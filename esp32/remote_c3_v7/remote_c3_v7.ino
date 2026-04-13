/*
 * EasyPlay BLE UART Remote v7 (Arduino / ESP32-C3 SuperMini)
 *
 * v6 + deep sleep. Sleeps after idle timeout, wakes on any button press.
 *
 * GPIO (active LOW, INPUT_PULLUP — buttons wired to GND):
 *   GPIO 0 = Right
 *   GPIO 1 = On/Off
 *   GPIO 2 = Up
 *   GPIO 4 = Left
 *   GPIO 5 = Down
 *
 * NeoPixel: GPIO 8
 *
 * Button codes: uppercase = press, lowercase = release
 *   L/l  R/r  U/u  D/d  O/o
 *
 * Sleep behavior:
 *   - Sleeps after IDLE_TIMEOUT_MS with no button press (default 3 min)
 *   - Red flash before sleep
 *   - Any button GPIO going LOW wakes from deep sleep
 *   - On wake: purple flash, re-inits BLE, starts advertising
 *   - Wake reason printed to serial
 *
 * LED modes (hold On/Off 3s to toggle, persists across sleep):
 *   Debug:      blue breathe (advertising), green breathe (connected),
 *               red breathe (last 30s before sleep), white flash on button
 *   Efficiency: LED off normally, white flash on button press when connected,
 *               double-speed white flash when not connected
 */

#include <NimBLEDevice.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include "esp_sleep.h"
#include "driver/gpio.h"

// ── NeoPixel ─────────────────────────────────────────────────────────────────
#define NEOPIXEL_PIN 8
Adafruit_NeoPixel pixel(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// ── Buttons ──────────────────────────────────────────────────────────────────
struct Button {
  uint8_t pin;
  char    downChar;
  char    upChar;
  const char* label;
  bool    prevPressed;
  unsigned long lastChangeMs;
};

Button buttons[] = {
  { 0, 'R', 'r', "RIGHT",  false, 0 },
  { 1, 'O', 'o', "ON/OFF", false, 0 },
  { 2, 'U', 'u', "UP",     false, 0 },
  { 4, 'L', 'l', "LEFT",   false, 0 },
  { 5, 'D', 'd', "DOWN",   false, 0 },
};
const int NUM_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);
const unsigned long DEBOUNCE_MS = 50;

// ── Sleep ────────────────────────────────────────────────────────────────────
#define IDLE_TIMEOUT_MS (2 * 60 * 1000)  // 2 minutes
unsigned long lastActivityMs = 0;

// ── LED mode ────────────────────────────────────────────────────────────────
// 0 = debug (breathing colors), 1 = efficiency (flash on button only)
#define MODE_DEBUG      0
#define MODE_EFFICIENCY 1
RTC_DATA_ATTR uint8_t ledMode = MODE_EFFICIENCY;
#define MODE_TOGGLE_HOLD_MS 10000  // hold On/Off 10s to toggle

unsigned long onOffPressedAt = 0;  // when On/Off was pressed (0 = not held)
bool modeToggled = false;          // prevent re-toggling while still held

// ── BLE NUS UUIDs (Nordic UART Service) ──────────────────────────────────────
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define TX_UUID      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // notify to Pi
#define RX_UUID      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // write from Pi

// ── Globals ──────────────────────────────────────────────────────────────────
NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pTxChar = nullptr;
bool deviceConnected = false;
bool wasConnected = false;
unsigned long connectedAt = 0;

// Deferred wake button — send once Pi connects
char deferredWakeChar = 0;   // 0 = nothing to send
bool piReady = false;        // set by RX callback when Pi sends "R"

// Forward declaration
bool bleSend(char c);

// ── BLE Callbacks ────────────────────────────────────────────────────────────
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    deviceConnected = true;
    connectedAt = millis();
    lastActivityMs = millis();  // connection counts as activity
    Serial.println("Pi connected!");
    pixel.setPixelColor(0, pixel.Color(0, 25, 0));
    pixel.show();
  }

  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    deviceConnected = false;
    Serial.printf("Disconnected (reason=%d)\n", reason);
    pixel.setPixelColor(0, 0);
    pixel.show();
    NimBLEDevice::startAdvertising();
    Serial.println("Advertising restarted");
  }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
    std::string val = pChar->getValue();
    if (val.length() > 0) {
      Serial.printf("RX from Pi: %s\n", val.c_str());
      lastActivityMs = millis();

      // Pi sends "R" (ready) after subscribing — flag it, main loop will send
      if (val[0] == 'R') {
        piReady = true;
        Serial.println("Pi ready signal received");
      }
    }
  }
};

// ── Send a character via BLE notify ──────────────────────────────────────────
bool bleSend(char c) {
  if (!deviceConnected || pTxChar == nullptr) return false;
  uint8_t data = (uint8_t)c;
  pTxChar->setValue(&data, 1);
  pTxChar->notify();
  return true;
}

// ── LED helpers ──────────────────────────────────────────────────────────────
// GPIO 8 has both a WS2812 NeoPixel and a basic blue LED (active LOW).
// After pixel.show(), the pin is left LOW = blue LED on.
// To kill both: send NeoPixel off, then drive pin HIGH.
void ledsOff() {
  pixel.setPixelColor(0, 0);
  pixel.show();
  delay(1);
  pinMode(NEOPIXEL_PIN, OUTPUT);
  digitalWrite(NEOPIXEL_PIN, LOW);  // blue LED off (active HIGH)
}

void flash(uint8_t r, uint8_t g, uint8_t b, int ms) {
  pixel.begin();  // re-init NeoPixel after plain GPIO use
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
  delay(ms);
  ledsOff();
}

// ── Deep sleep ───────────────────────────────────────────────────────────────
void enterDeepSleep() {
  Serial.printf(">>> DEEP SLEEP after %lu ms idle <<<\n", millis() - lastActivityMs);
  Serial.flush();

  // Stop advertising first so no new connections come in
  NimBLEDevice::getAdvertising()->stop();

  // Disconnect cleanly if connected — give Pi time to process
  if (deviceConnected && pServer != nullptr) {
    pServer->disconnect(0);
    delay(500);  // let disconnect propagate before deinit
  }

  // Shut down BLE stack
  NimBLEDevice::deinit(true);
  delay(100);

  // Red flash (only in debug mode)
  if (ledMode == MODE_DEBUG) {
    flash(25, 0, 0, 300);
  }

  // GPIO 8 has TWO LEDs: basic blue LED (active LOW) + WS2812 NeoPixel
  // Step 1: send NeoPixel "all off" via WS2812 protocol
  pixel.setPixelColor(0, 0);
  pixel.show();
  // Step 2: WS2812 protocol ends with pin LOW after latch.
  //         Switch to plain GPIO LOW = blue LED off (it's active HIGH)
  delay(1);
  pinMode(NEOPIXEL_PIN, OUTPUT);
  digitalWrite(NEOPIXEL_PIN, LOW);
  delay(1);
  // Step 3: latch this state through deep sleep
  gpio_hold_en((gpio_num_t)NEOPIXEL_PIN);
  gpio_deep_sleep_hold_en();

  // Wake on any button GPIO going LOW
  esp_deep_sleep_enable_gpio_wakeup(
    (1ULL << 0) | (1ULL << 1) | (1ULL << 2) | (1ULL << 4) | (1ULL << 5),
    ESP_GPIO_WAKEUP_GPIO_LOW
  );

  esp_deep_sleep_start();
}

// ── Setup ────────────────────────────────────────────────────────────────────
void setup() {
  // FIRST: capture wake reason and GPIO status before anything else
  esp_sleep_wakeup_cause_t wakeReason = esp_sleep_get_wakeup_cause();
  uint64_t wakeGPIOs = esp_sleep_get_gpio_wakeup_status();

  // SECOND: configure button pins and read them immediately (wire may still be held)
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
  }

  // Detect which button woke us (3 methods, first match wins)
  if (wakeReason == ESP_SLEEP_WAKEUP_GPIO) {
    // Method 1: hardware latch
    for (int i = 0; i < NUM_BUTTONS; i++) {
      if (!deferredWakeChar && (wakeGPIOs & (1ULL << buttons[i].pin))) {
        deferredWakeChar = buttons[i].downChar;
      }
    }
    // Method 2: read GPIOs directly (wire might still be touching)
    for (int i = 0; i < NUM_BUTTONS; i++) {
      if (!deferredWakeChar && digitalRead(buttons[i].pin) == LOW) {
        deferredWakeChar = buttons[i].downChar;
      }
      // Initialize prevPressed to actual state so we don't get
      // phantom press/release events in the loop after wake
      buttons[i].prevPressed = (digitalRead(buttons[i].pin) == LOW);
    }
  }

  // Now do the slower init stuff
  Serial.begin(115200);
  delay(1000);

  // Release GPIO hold from deep sleep so NeoPixel works again
  gpio_hold_dis((gpio_num_t)NEOPIXEL_PIN);

  // NeoPixel init
  pixel.begin();
  pixel.setBrightness(255);
  pixel.setPixelColor(0, 0);
  pixel.show();

  // ── Wake reason ──
  Serial.println("\n============================================");
  Serial.println("  EasyPlay Remote C3 v7 (BLE + sleep)");
  Serial.println("============================================");

  if (wakeReason == ESP_SLEEP_WAKEUP_GPIO) {
    Serial.printf("WAKE: GPIO latch=0x%llX\n", wakeGPIOs);
    if (deferredWakeChar) {
      Serial.printf("Deferred wake button: '%c' — will send when Pi says ready\n", deferredWakeChar);
    } else {
      Serial.println("WARNING: GPIO wake but no button detected!");
    }
    flash(25, 0, 25, 300);  // purple = wake from sleep
  } else {
    Serial.printf("BOOT: cold start (reason=%d)\n", wakeReason);
    flash(0, 0, 25, 300);   // blue = cold boot
  }

  Serial.printf("MAC: ");

  // ── BLE init ──
  NimBLEDevice::init("EasyPlay");
  NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_PUBLIC);
  Serial.println(NimBLEDevice::getAddress().toString().c_str());

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  pTxChar = pService->createCharacteristic(
    TX_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );

  NimBLECharacteristic* pRxChar = pService->createCharacteristic(
    RX_UUID,
    NIMBLE_PROPERTY::WRITE_NR
  );
  pRxChar->setCallbacks(new RxCallbacks());

  pService->start();

  // ── Advertising ──
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setName("EasyPlay");
  pAdvertising->setMinInterval(400);  // 250ms
  pAdvertising->setMaxInterval(400);
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.printf("Advertising. Sleep after %d min idle.\n", IDLE_TIMEOUT_MS / 60000);
  Serial.printf("LED mode: %s (hold On/Off 3s to toggle)\n", ledMode == MODE_DEBUG ? "DEBUG" : "EFFICIENCY");
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.println("--------------------------------------------");

  lastActivityMs = millis();
}

// ── Main loop ────────────────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  if (deviceConnected && !wasConnected) {
    wasConnected = true;
  }
  if (!deviceConnected && wasConnected) {
    wasConnected = false;
    piReady = false;
  }

  // ── Deferred wake button: send from main loop once Pi is ready ──
  if (piReady && deferredWakeChar && deviceConnected) {
    Serial.printf("DEFERRED SEND: press='%c' (0x%02X)\n", deferredWakeChar, deferredWakeChar);
    uint8_t pressData = (uint8_t)deferredWakeChar;
    pTxChar->setValue(&pressData, 1);
    pTxChar->notify();
    deferredWakeChar = 0;
    piReady = false;
  }

  // ── Button scanning ──
  bool buttonEvent = false;
  bool buttonPress = false;  // true only for press (not release)
  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool pressed = digitalRead(buttons[i].pin) == LOW;

    if (pressed != buttons[i].prevPressed) {
      if ((now - buttons[i].lastChangeMs) > DEBOUNCE_MS) {
        lastActivityMs = now;
        buttonEvent = true;

        if (pressed) {
          buttonPress = true;
          Serial.printf("BTN DOWN: %s\n", buttons[i].label);
          if (deviceConnected) bleSend(buttons[i].downChar);
        } else {
          Serial.printf("BTN UP:   %s\n", buttons[i].label);
          if (deviceConnected) bleSend(buttons[i].upChar);
        }
        buttons[i].lastChangeMs = now;
        buttons[i].prevPressed = pressed;
      }
    }
  }

  // ── Mode toggle: hold On/Off for 3 seconds ──
  bool onOffHeld = (digitalRead(buttons[1].pin) == LOW);  // buttons[1] = On/Off
  if (onOffHeld && onOffPressedAt == 0) {
    onOffPressedAt = now;
    modeToggled = false;
  } else if (!onOffHeld) {
    onOffPressedAt = 0;
    modeToggled = false;
  } else if (onOffHeld && !modeToggled && onOffPressedAt > 0
             && (now - onOffPressedAt >= MODE_TOGGLE_HOLD_MS)) {
    modeToggled = true;
    ledMode = (ledMode == MODE_DEBUG) ? MODE_EFFICIENCY : MODE_DEBUG;
    Serial.printf("LED mode: %s\n", ledMode == MODE_DEBUG ? "DEBUG" : "EFFICIENCY");
    // Confirm flash: green = debug, cyan = efficiency
    if (ledMode == MODE_DEBUG) {
      flash(0, 25, 0, 200); flash(0, 25, 0, 200);
    } else {
      flash(0, 25, 25, 200); flash(0, 25, 25, 200);
    }
  }

  // ── LED status ──
  unsigned long idleTime = now - lastActivityMs;
  unsigned long timeToSleep = (idleTime < IDLE_TIMEOUT_MS) ? (IDLE_TIMEOUT_MS - idleTime) : 0;

  if (ledMode == MODE_EFFICIENCY) {
    // Efficiency: NeoPixel flash on button, GPIO LOW otherwise (blue LED off)
    // Use built-in neopixelWrite() — handles RMT setup each call,
    // works even after plain GPIO override
    if (buttonPress) {
      if (deviceConnected) {
        // Single white flash
        neopixelWrite(NEOPIXEL_PIN, 25, 25, 25);
        delay(80);
      } else {
        // Double flash = not connected
        neopixelWrite(NEOPIXEL_PIN, 25, 25, 25);
        delay(50);
        neopixelWrite(NEOPIXEL_PIN, 0, 0, 0);
        delay(50);
        neopixelWrite(NEOPIXEL_PIN, 25, 25, 25);
        delay(50);
      }
      neopixelWrite(NEOPIXEL_PIN, 0, 0, 0);
      delay(1);
      pinMode(NEOPIXEL_PIN, OUTPUT);
      digitalWrite(NEOPIXEL_PIN, LOW);
    } else {
      pinMode(NEOPIXEL_PIN, OUTPUT);
      digitalWrite(NEOPIXEL_PIN, LOW);
    }
  } else {
    // Debug mode: breathing LEDs
    pixel.begin();  // re-init in case efficiency mode took over GPIO 8
    if (buttonEvent) {
      pixel.setPixelColor(0, pixel.Color(25, 25, 25));
      pixel.show();
    } else {
      // Sine-ish breathe: 3s period, brightness 1-20
      float phase = (float)(now % 3000) / 3000.0;
      float breath = (sin(phase * 2.0 * PI) + 1.0) / 2.0;  // 0.0 to 1.0
      uint8_t b = 1 + (uint8_t)(breath * 19);

      if (timeToSleep < 30000 && timeToSleep > 0) {
        // Last 30 seconds: red breathe = about to sleep
        pixel.setPixelColor(0, pixel.Color(b, 0, 0));
      } else if (deviceConnected) {
        // Connected: green breathe
        pixel.setPixelColor(0, pixel.Color(0, b, 0));
      } else {
        // Advertising, waiting: blue breathe
        pixel.setPixelColor(0, pixel.Color(0, 0, b));
      }
      pixel.show();
    }
  }

  // ── Idle timeout → deep sleep ──
  if (timeToSleep == 0) {
    enterDeepSleep();
  }

  delay(10);
}
