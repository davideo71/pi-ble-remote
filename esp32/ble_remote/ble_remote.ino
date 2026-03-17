/*
 * BLE Remote - ESP32-C3 GATT Server
 *
 * Test 34: Interrupt-driven buttons — no digitalRead in loop.
 * Tests 32-33 proved that polling digitalRead() on 5 pins disrupts NimBLE
 * on the single-core C3. This version uses attachInterrupt() so GPIO is
 * only accessed on actual pin changes, not every loop iteration.
 */

#include <NimBLEDevice.h>

// Custom UUIDs for our remote service
#define SERVICE_UUID        "4e520001-7354-4288-9a71-81a9bf56c4a8"
#define BUTTON_CHAR_UUID    "4e520002-7354-4288-9a71-81a9bf56c4a8"

// Button configuration
struct Button {
    uint8_t pin;
    char pressChar;
    char releaseChar;
    volatile bool changed;      // set by ISR
    bool pressed;               // debounced state
    unsigned long lastChangeTime; // for debounce
};

#define NUM_BUTTONS 5
#define DEBOUNCE_MS 50

Button buttons[NUM_BUTTONS] = {
    {0, 'L', 'l', false, false, 0},
    {1, 'R', 'r', false, false, 0},
    {2, 'U', 'u', false, false, 0},
    {3, 'D', 'd', false, false, 0},
    {4, 'O', 'o', false, false, 0},
};

// ISR handlers — one per button, just sets the changed flag
void IRAM_ATTR isr_btn0() { buttons[0].changed = true; }
void IRAM_ATTR isr_btn1() { buttons[1].changed = true; }
void IRAM_ATTR isr_btn2() { buttons[2].changed = true; }
void IRAM_ATTR isr_btn3() { buttons[3].changed = true; }
void IRAM_ATTR isr_btn4() { buttons[4].changed = true; }

typedef void (*isr_func_t)();
isr_func_t isrHandlers[NUM_BUTTONS] = { isr_btn0, isr_btn1, isr_btn2, isr_btn3, isr_btn4 };

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pButtonChar = nullptr;

bool deviceConnected = false;
bool wasConnected = false;
uint32_t heartbeatCounter = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastMemReport = 0;
unsigned long lastAdvRestart = 0;
unsigned long bootTime = 0;

// Helper: milliseconds since boot as readable timestamp
void printTimestamp() {
    unsigned long ms = millis();
    unsigned long secs = ms / 1000;
    unsigned long mins = secs / 60;
    Serial.printf("[%02lu:%02lu.%03lu] ", mins, secs % 60, ms % 1000);
}

// Server callbacks - log every connection event
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
        Serial.printf("  Conn interval: %d (x1.25ms = %.1fms)\n",
            connInfo.getConnInterval(),
            connInfo.getConnInterval() * 1.25);
        printTimestamp();
        Serial.printf("  Free heap: %d bytes\n", ESP.getFreeHeap());

        // Request relaxed connection parameters from the central (Pi)
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

        // Critical: restart advertising so the Pi can reconnect
        NimBLEDevice::startAdvertising();
        printTimestamp();
        Serial.println("  Advertising restarted - ready for reconnection");
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
    Serial.begin(115200);
    delay(1000);

    bootTime = millis();

    Serial.println("\n\n");
    Serial.println("============================================");
    Serial.println("  BLE Remote - ESP32-C3 GATT Server");
    Serial.println("  Test 34: Interrupt-driven buttons, 10ms");
    Serial.println("============================================");
    printTimestamp();
    Serial.printf("Boot reason: %d\n", esp_reset_reason());
    printTimestamp();
    Serial.printf("Free heap at boot: %d bytes\n", ESP.getFreeHeap());

    // Initialize button GPIOs with interrupts
    printTimestamp();
    Serial.println("Initializing buttons (GPIO 0-4, INPUT_PULLUP, interrupt on CHANGE)...");
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(buttons[i].pin, INPUT_PULLUP);
        attachInterrupt(buttons[i].pin, isrHandlers[i], CHANGE);
        printTimestamp();
        Serial.printf("  GPIO %d = '%c'/'%c' — interrupt attached\n",
            buttons[i].pin, buttons[i].pressChar, buttons[i].releaseChar);
    }

    // Initialize NimBLE
    printTimestamp();
    Serial.println("Initializing NimBLE...");
    NimBLEDevice::init("BLE-Remote");
    NimBLEDevice::setPower(9);
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

    // Configure and start advertising
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->enableScanResponse(true);
    pAdvertising->setMinInterval(0x20);
    pAdvertising->setMaxInterval(0x60);
    pAdvertising->start();

    printTimestamp();
    Serial.println("Advertising started — waiting for connections...");
    printTimestamp();
    Serial.printf("Free heap after init: %d bytes\n", ESP.getFreeHeap());
    Serial.println("--------------------------------------------\n");
}

void loop() {
    unsigned long now = millis();

    // ---- Process button interrupts (only when a pin actually changed) ----
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (!buttons[i].changed) continue;

        // Debounce: ignore changes within DEBOUNCE_MS of last change
        if ((now - buttons[i].lastChangeTime) < DEBOUNCE_MS) {
            buttons[i].changed = false;
            continue;
        }

        buttons[i].changed = false;
        buttons[i].lastChangeTime = now;

        // Read current state — single digitalRead only when ISR fired
        bool pressed = !digitalRead(buttons[i].pin);  // LOW = pressed

        if (pressed != buttons[i].pressed) {
            buttons[i].pressed = pressed;
            printTimestamp();
            Serial.printf("BUTTON: '%c' [GPIO %d]\n",
                pressed ? buttons[i].pressChar : buttons[i].releaseChar,
                buttons[i].pin);
        }
    }

    // ---- Heartbeat every 2s ----
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
        Serial.println("State: DISCONNECTED -> CONNECTED (interrupt-driven buttons)");
    }

    // Re-start advertising every 30 seconds when not connected
    if (!deviceConnected && (now - lastAdvRestart >= 30000)) {
        lastAdvRestart = now;
        NimBLEDevice::getAdvertising()->start();
        printTimestamp();
        Serial.println("ADV: Re-started advertising (periodic refresh)");
    }

    // Memory report every 30 seconds
    if (now - lastMemReport >= 30000) {
        lastMemReport = now;
        printTimestamp();
        Serial.printf("STATUS: heap=%d | connected=%s | heartbeats=%d\n",
            ESP.getFreeHeap(),
            deviceConnected ? "YES" : "NO",
            heartbeatCounter);
    }

    delay(10);  // 10ms loop — but no GPIO access unless ISR fired
}
