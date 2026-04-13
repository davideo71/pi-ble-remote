/*
 * BLE Remote - ESP32-C3 SuperMini GATT Server
 *
 * 5 buttons on GPIO 1,2,4,5,7 with internal pull-ups.
 * Touch a patch wire from any GPIO to GND to simulate a press.
 *
 * Button protocol: single ASCII char per event
 *   Press:   L R U D O  (uppercase = KEYDOWN)
 *   Release: l r u d o  (lowercase = KEYUP)
 *
 * LED patterns (onboard LED on GPIO 8):
 *   - Slow blink: Advertising, waiting for connection
 *   - Solid on: Connected
 *   - Fast blink: Button event sent
 */

#include <NimBLEDevice.h>

// Custom UUIDs for our remote service
#define SERVICE_UUID        "4e520001-7354-4288-9a71-81a9bf56c4a8"
#define BUTTON_CHAR_UUID    "4e520002-7354-4288-9a71-81a9bf56c4a8"

#define LED_PIN 8       // Onboard LED on C3 SuperMini
#define LED_ON  LOW     // C3 SuperMini LED is active-LOW
#define LED_OFF HIGH

// ── Button configuration ──
struct Button {
    uint8_t pin;
    char pressChar;    // uppercase = KEYDOWN
    char releaseChar;  // lowercase = KEYUP
    const char* name;
    volatile bool lastState;      // true = released (HIGH), false = pressed (LOW)
    volatile unsigned long lastChange;  // debounce timestamp
};

#define NUM_BUTTONS 5
#define DEBOUNCE_MS 50

Button buttons[NUM_BUTTONS] = {
    { 1, 'L', 'l', "LEFT",   true, 0 },
    { 2, 'R', 'r', "RIGHT",  true, 0 },
    { 4, 'U', 'u', "UP",     true, 0 },
    { 5, 'D', 'd', "DOWN",   true, 0 },
    { 7, 'O', 'o', "ON/OFF", true, 0 },
};

// ISR flag: which buttons need processing
volatile uint8_t buttonFlags = 0;

// ISR handler — one per button, sets a flag
void IRAM_ATTR buttonISR0() { buttonFlags |= (1 << 0); }
void IRAM_ATTR buttonISR1() { buttonFlags |= (1 << 1); }
void IRAM_ATTR buttonISR2() { buttonFlags |= (1 << 2); }
void IRAM_ATTR buttonISR3() { buttonFlags |= (1 << 3); }
void IRAM_ATTR buttonISR4() { buttonFlags |= (1 << 4); }

void (*buttonISRs[NUM_BUTTONS])() = {
    buttonISR0, buttonISR1, buttonISR2, buttonISR3, buttonISR4
};

// ── BLE globals ──
NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pButtonChar = nullptr;

bool deviceConnected = false;
bool wasConnected = false;
uint32_t heartbeatCounter = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastMemReport = 0;
unsigned long lastAdvRestart = 0;
unsigned long lastLedToggle = 0;
bool ledState = false;

// Helper: milliseconds since boot as readable timestamp
void printTimestamp() {
    unsigned long ms = millis();
    unsigned long secs = ms / 1000;
    unsigned long mins = secs / 60;
    Serial.printf("[%02lu:%02lu.%03lu] ", mins, secs % 60, ms % 1000);
}

// Server callbacks
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        deviceConnected = true;
        printTimestamp();
        Serial.println("========== CLIENT CONNECTED ==========");
        printTimestamp();
        Serial.printf("  Address: %s\n", connInfo.getAddress().toString().c_str());
        printTimestamp();
        Serial.printf("  Conn handle: %d\n", connInfo.getConnHandle());
        printTimestamp();
        Serial.printf("  Free heap: %d bytes\n", ESP.getFreeHeap());

        pServer->updateConnParams(connInfo.getConnHandle(), 24, 48, 0, 600);
        printTimestamp();
        Serial.println("  Requested conn params: interval=30-60ms, latency=0, timeout=6000ms");
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        deviceConnected = false;
        printTimestamp();
        Serial.println("========== CLIENT DISCONNECTED ==========");
        printTimestamp();
        Serial.printf("  Reason: 0x%02X (%d)\n", reason, reason);

        NimBLEDevice::startAdvertising();
        printTimestamp();
        Serial.println("  Advertising restarted after disconnect");
    }

    void onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo) override {
        printTimestamp();
        Serial.printf("MTU changed to: %d\n", MTU);
    }
};

// Characteristic callbacks
class ButtonCharCallbacks : public NimBLECharacteristicCallbacks {
    void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override {
        printTimestamp();
        const char* subType = "UNKNOWN";
        if (subValue == 0) subType = "UNSUBSCRIBED";
        else if (subValue == 1) subType = "NOTIFICATIONS";
        else if (subValue == 2) subType = "INDICATIONS";
        else if (subValue == 3) subType = "NOTIFICATIONS+INDICATIONS";
        Serial.printf("Subscription changed: %s (value: %d) by %s\n",
            subType, subValue, connInfo.getAddress().toString().c_str());
    }
};

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LED_ON);  // LED on during boot

    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n");
    Serial.println("============================================");
    Serial.println("  BLE Remote - ESP32-S3 SuperMini");
    Serial.println("  Test 46: Buttons + heartbeat");
    Serial.println("============================================");
    printTimestamp();
    Serial.printf("Chip: %s Rev %d | Cores: %d | CPU: %dMHz\n",
        ESP.getChipModel(), ESP.getChipRevision(),
        ESP.getChipCores(), ESP.getCpuFreqMHz());
    printTimestamp();
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());

    // ── Initialize buttons ──
    printTimestamp();
    Serial.println("Configuring buttons:");
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(buttons[i].pin, INPUT_PULLUP);
        buttons[i].lastState = digitalRead(buttons[i].pin);
        attachInterrupt(digitalPinToInterrupt(buttons[i].pin), buttonISRs[i], CHANGE);
        printTimestamp();
        Serial.printf("  GPIO %d = %s (%c/%c) — state: %s\n",
            buttons[i].pin, buttons[i].name,
            buttons[i].pressChar, buttons[i].releaseChar,
            buttons[i].lastState ? "RELEASED" : "PRESSED");
    }

    // ── Initialize NimBLE ──
    printTimestamp();
    Serial.println("Initializing NimBLE...");
    NimBLEDevice::init("BLE-Remote");
    NimBLEDevice::setPower(9);  // Max TX power (+9 dBm)
    printTimestamp();
    Serial.printf("TX power: %d dBm | BLE address: %s\n",
        NimBLEDevice::getPower(),
        NimBLEDevice::getAddress().toString().c_str());

    // Create server
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // Create service + characteristic
    NimBLEService* pService = pServer->createService(SERVICE_UUID);
    pButtonChar = pService->createCharacteristic(
        BUTTON_CHAR_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    pButtonChar->setCallbacks(new ButtonCharCallbacks());
    pService->start();

    // Configure advertising
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();

    NimBLEAdvertisementData advData;
    advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
    advData.addServiceUUID(SERVICE_UUID);
    pAdvertising->setAdvertisementData(advData);

    NimBLEAdvertisementData scanData;
    scanData.setName("BLE-Remote");
    pAdvertising->setScanResponseData(scanData);

    pAdvertising->setMinInterval(0x20);  // 20ms
    pAdvertising->setMaxInterval(0x40);  // 40ms
    pAdvertising->start();

    printTimestamp();
    Serial.println("Advertising started — always on, no sleep");
    printTimestamp();
    Serial.printf("Free heap after init: %d bytes\n", ESP.getFreeHeap());
    Serial.println("--------------------------------------------");
    Serial.println("READY — touch GPIO 1/2/4/5/7 to GND to test buttons");
    Serial.println("--------------------------------------------\n");

    digitalWrite(LED_PIN, LED_OFF);  // Boot indicator off
}

void processButtons() {
    uint8_t flags = buttonFlags;
    buttonFlags = 0;  // Clear flags

    if (flags == 0) return;

    unsigned long now = millis();

    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (!(flags & (1 << i))) continue;

        // Debounce
        if (now - buttons[i].lastChange < DEBOUNCE_MS) continue;

        bool currentState = digitalRead(buttons[i].pin);  // HIGH = released, LOW = pressed
        if (currentState == buttons[i].lastState) continue;  // No real change

        buttons[i].lastState = currentState;
        buttons[i].lastChange = now;

        bool pressed = !currentState;  // LOW = pressed (pull-up to GND)
        char eventChar = pressed ? buttons[i].pressChar : buttons[i].releaseChar;

        printTimestamp();
        Serial.printf("BUTTON: %s %s (GPIO %d -> '%c')\n",
            buttons[i].name,
            pressed ? "PRESSED" : "RELEASED",
            buttons[i].pin,
            eventChar);

        // Send over BLE if connected
        if (deviceConnected) {
            uint8_t data = (uint8_t)eventChar;
            pButtonChar->setValue(&data, 1);
            bool sent = pButtonChar->notify();
            printTimestamp();
            Serial.printf("  BLE notify '%c' -> %s\n", eventChar, sent ? "OK" : "FAILED");

            // Quick LED flash on button event
            digitalWrite(LED_PIN, LED_OFF);
            delay(30);
            digitalWrite(LED_PIN, LED_ON);
        } else {
            printTimestamp();
            Serial.println("  (not connected — event logged only)");
        }
    }
}

void loop() {
    unsigned long now = millis();

    // ── Process button events (interrupt-driven) ──
    processButtons();

    // ── Heartbeat every 2s when connected ──
    if (deviceConnected && (now - lastHeartbeat >= 2000)) {
        lastHeartbeat = now;
        heartbeatCounter++;

        pButtonChar->setValue(heartbeatCounter);
        bool sent = pButtonChar->notify();

        printTimestamp();
        Serial.printf("HEARTBEAT #%d notify=%s\n", heartbeatCounter, sent ? "true" : "false");
    }

    // Track connection state transitions
    if (wasConnected && !deviceConnected) {
        wasConnected = false;
        printTimestamp();
        Serial.println("State: CONNECTED -> DISCONNECTED");
    }
    if (!wasConnected && deviceConnected) {
        wasConnected = true;
        heartbeatCounter = 0;
        printTimestamp();
        Serial.println("State: DISCONNECTED -> CONNECTED");
    }

    // Re-start advertising every 30s when not connected (safety net)
    if (!deviceConnected && (now - lastAdvRestart >= 30000)) {
        lastAdvRestart = now;
        NimBLEDevice::getAdvertising()->start();
        printTimestamp();
        Serial.println("ADV: Periodic advertising restart");
    }

    // LED status
    if (deviceConnected) {
        digitalWrite(LED_PIN, LED_ON);  // Solid on when connected
    } else {
        if (now - lastLedToggle >= 1000) {
            lastLedToggle = now;
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState ? LED_ON : LED_OFF);
        }
    }

    // Memory report every 30s
    if (now - lastMemReport >= 30000) {
        lastMemReport = now;
        printTimestamp();
        Serial.printf("STATUS: heap=%d connected=%s heartbeats=%d uptime=%lus\n",
            ESP.getFreeHeap(),
            deviceConnected ? "YES" : "NO",
            heartbeatCounter,
            now / 1000);
    }

    delay(10);
}
