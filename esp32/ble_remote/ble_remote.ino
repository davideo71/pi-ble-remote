/*
 * BLE Remote - ESP32-S3 SuperMini GATT Server
 *
 * Test 45: Always-on advertising, no sleep, heartbeat only.
 * Designed for testing BLE connection with Pi receiver v2.
 *
 * LED patterns (NeoPixel on GPIO 48):
 *   - Slow pulse blue: Advertising, waiting for connection
 *   - Solid green: Connected
 *   - Red flash on boot: Startup indicator
 *
 * No buttons wired yet — heartbeat only.
 * No sleep mode — always advertising after power on.
 */

#include <NimBLEDevice.h>

// Custom UUIDs for our remote service
#define SERVICE_UUID        "4e520001-7354-4288-9a71-81a9bf56c4a8"
#define BUTTON_CHAR_UUID    "4e520002-7354-4288-9a71-81a9bf56c4a8"

// S3 SuperMini has a NeoPixel on GPIO 48 — but we'll use simple digital out
// for the built-in LED. If your board has a regular LED, adjust the pin.
#define LED_PIN 48  // NeoPixel data pin on S3 SuperMini

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

        // CRITICAL: restart advertising after disconnect!
        // Without this, the device goes silent and is never found again.
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
    digitalWrite(LED_PIN, HIGH);  // LED on during boot

    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n");
    Serial.println("============================================");
    Serial.println("  BLE Remote - ESP32-S3 SuperMini");
    Serial.println("  Test 45: Always-on, heartbeat only");
    Serial.println("============================================");
    printTimestamp();
    Serial.printf("Chip: %s Rev %d | Cores: %d | CPU: %dMHz\n",
        ESP.getChipModel(), ESP.getChipRevision(),
        ESP.getChipCores(), ESP.getCpuFreqMHz());
    printTimestamp();
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());

    // Initialize NimBLE
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

    // Configure advertising with name in both adv data and scan response
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();

    // Adv data: flags + service UUID
    NimBLEAdvertisementData advData;
    advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
    advData.addServiceUUID(SERVICE_UUID);
    pAdvertising->setAdvertisementData(advData);

    // Scan response: device name
    NimBLEAdvertisementData scanData;
    scanData.setName("BLE-Remote");
    pAdvertising->setScanResponseData(scanData);

    pAdvertising->setMinInterval(0x20);  // 20ms
    pAdvertising->setMaxInterval(0x40);  // 40ms — faster for testing
    pAdvertising->start();

    printTimestamp();
    Serial.println("Advertising started — always on, no sleep");
    printTimestamp();
    Serial.printf("Free heap after init: %d bytes\n", ESP.getFreeHeap());
    Serial.println("--------------------------------------------\n");

    digitalWrite(LED_PIN, LOW);  // Boot indicator off
}

void loop() {
    unsigned long now = millis();

    // Heartbeat every 2s when connected
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

    // Re-start advertising every 30 seconds when not connected (safety net)
    if (!deviceConnected && (now - lastAdvRestart >= 30000)) {
        lastAdvRestart = now;
        NimBLEDevice::getAdvertising()->start();
        printTimestamp();
        Serial.println("ADV: Periodic advertising restart");
    }

    // LED status
    if (deviceConnected) {
        digitalWrite(LED_PIN, HIGH);  // Solid on when connected
    } else {
        // Slow blink when advertising
        if (now - lastLedToggle >= 1000) {
            lastLedToggle = now;
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);
        }
    }

    // Memory report every 30 seconds
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
