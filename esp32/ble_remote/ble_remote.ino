/*
 * BLE Remote - ESP32-C3 GATT Server
 *
 * Step 2: Button handling + BLE notifications.
 * 5 buttons on GPIO 0-4, wired to GND with internal pull-ups.
 * Sends single ASCII chars: uppercase=press, lowercase=release.
 * Still sends heartbeat every 2s when no buttons active.
 */

#include <NimBLEDevice.h>

// Custom UUIDs for our remote service
#define SERVICE_UUID        "4e520001-7354-4288-9a71-81a9bf56c4a8"
#define BUTTON_CHAR_UUID    "4e520002-7354-4288-9a71-81a9bf56c4a8"

// Button configuration: GPIO pin → ASCII char
// Wired to GND with internal pull-ups (LOW = pressed)
struct Button {
    uint8_t pin;
    char pressChar;    // Uppercase = KEYDOWN
    char releaseChar;  // Lowercase = KEYUP
    bool pressed;      // Current debounced state
    bool lastReading;  // Last raw reading (for debounce)
    unsigned long lastDebounceTime;
};

#define NUM_BUTTONS 5
#define DEBOUNCE_MS 50

Button buttons[NUM_BUTTONS] = {
    {0, 'L', 'l', false, true, 0},  // GPIO 0 = Left
    {1, 'R', 'r', false, true, 0},  // GPIO 1 = Right
    {2, 'U', 'u', false, true, 0},  // GPIO 2 = Up
    {3, 'D', 'd', false, true, 0},  // GPIO 3 = Down
    {4, 'O', 'o', false, true, 0},  // GPIO 4 = On/Off
};

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

// Send a single char as BLE notification
void sendButton(char c) {
    uint8_t data = (uint8_t)c;
    pButtonChar->setValue(&data, 1);
    bool sent = pButtonChar->notify();
    printTimestamp();
    Serial.printf("BUTTON: '%c' (0x%02X) notify=%s\n", c, c, sent ? "true" : "false");
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
    Serial.println("  Step 2: Buttons + BLE Notifications");
    Serial.println("============================================");
    printTimestamp();
    Serial.printf("Boot reason: %d\n", esp_reset_reason());
    printTimestamp();
    Serial.printf("Free heap at boot: %d bytes\n", ESP.getFreeHeap());

    // Initialize button pins
    printTimestamp();
    Serial.println("Initializing buttons (GPIO 0-4, INPUT_PULLUP)...");
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(buttons[i].pin, INPUT_PULLUP);
        printTimestamp();
        Serial.printf("  GPIO %d = '%c'/'%c' (read: %d)\n",
            buttons[i].pin, buttons[i].pressChar, buttons[i].releaseChar,
            digitalRead(buttons[i].pin));
    }

    // Initialize NimBLE
    printTimestamp();
    Serial.println("Initializing NimBLE...");
    NimBLEDevice::init("BLE-Remote");
    NimBLEDevice::setPower(9);  // +9 dBm — cheap C3 clones distort at max power
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
    pAdvertising->setMinInterval(0x20);  // 20ms
    pAdvertising->setMaxInterval(0x60);  // 60ms
    pAdvertising->start();

    printTimestamp();
    Serial.println("Advertising started — waiting for connections...");
    printTimestamp();
    Serial.printf("Free heap after init: %d bytes\n", ESP.getFreeHeap());
    Serial.println("--------------------------------------------\n");
}

void loop() {
    unsigned long now = millis();

    // ---- Button scanning with debounce ----
    for (int i = 0; i < NUM_BUTTONS; i++) {
        bool reading = !digitalRead(buttons[i].pin);  // LOW = pressed (inverted)

        // Reset debounce timer on state change
        if (reading != buttons[i].lastReading) {
            buttons[i].lastDebounceTime = now;
            buttons[i].lastReading = reading;
        }

        // Accept new state after debounce period
        if ((now - buttons[i].lastDebounceTime) >= DEBOUNCE_MS) {
            if (reading != buttons[i].pressed) {
                buttons[i].pressed = reading;

                if (deviceConnected) {
                    sendButton(reading ? buttons[i].pressChar : buttons[i].releaseChar);
                } else {
                    printTimestamp();
                    Serial.printf("BUTTON: '%c' (not connected, dropped)\n",
                        reading ? buttons[i].pressChar : buttons[i].releaseChar);
                }
            }
        }
    }

    // ---- Simulated button presses for testing (cycle all 5 buttons every 5s) ----
    static unsigned long lastSimButton = 0;
    static int simButtonIndex = 0;
    static bool simPressed = false;
    if (deviceConnected && (now - lastSimButton >= 500)) {
        lastSimButton = now;
        if (!simPressed) {
            // Send press
            sendButton(buttons[simButtonIndex].pressChar);
            simPressed = true;
        } else {
            // Send release, advance to next button
            sendButton(buttons[simButtonIndex].releaseChar);
            simPressed = false;
            simButtonIndex = (simButtonIndex + 1) % NUM_BUTTONS;
        }
    }

    // ---- Heartbeat every 2s (keeps connection alive, useful for monitoring) ----
    if (deviceConnected && (now - lastHeartbeat >= 2000)) {
        lastHeartbeat = now;
        heartbeatCounter++;

        pButtonChar->setValue(heartbeatCounter);
        bool sent = pButtonChar->notify();

        // Only log every 10th heartbeat to reduce serial spam now that buttons are the focus
        if (heartbeatCounter % 10 == 0) {
            printTimestamp();
            Serial.printf("HEARTBEAT #%d (every 10th logged)\n", heartbeatCounter);
        }
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
        Serial.println("State: DISCONNECTED -> CONNECTED (counter reset)");
    }

    // Re-start advertising every 30 seconds when not connected
    if (!deviceConnected && (now - lastAdvRestart >= 30000)) {
        lastAdvRestart = now;
        NimBLEDevice::getAdvertising()->start();
        printTimestamp();
        Serial.println("ADV: Re-started advertising (periodic refresh)");
    }

    // Memory report every 30 seconds (reduced from 10s)
    if (now - lastMemReport >= 30000) {
        lastMemReport = now;
        printTimestamp();
        Serial.printf("STATUS: heap=%d | connected=%s | heartbeats=%d\n",
            ESP.getFreeHeap(),
            deviceConnected ? "YES" : "NO",
            heartbeatCounter);
    }

    delay(5);  // 5ms loop = 200Hz button polling (was 10ms)
}
