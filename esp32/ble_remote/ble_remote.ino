/*
 * BLE Remote - ESP32-C3 GATT Server
 *
 * Step 1: Basic BLE connection with verbose debug output.
 * Advertises as "BLE-Remote", sends heartbeat notifications every 2s.
 * Later this will carry button press events instead.
 */

#include <NimBLEDevice.h>

// Custom UUIDs for our remote service
#define SERVICE_UUID        "4e520001-7354-4288-9a71-81a9bf56c4a8"
#define BUTTON_CHAR_UUID    "4e520002-7354-4288-9a71-81a9bf56c4a8"

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pButtonChar = nullptr;

bool deviceConnected = false;
bool wasConnected = false;
uint32_t heartbeatCounter = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastMemReport = 0;
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
        Serial.printf("  Latency: %d\n", connInfo.getConnLatency());
        printTimestamp();
        Serial.printf("  Supervision timeout: %d (x10ms = %dms)\n",
            connInfo.getConnTimeout(),
            connInfo.getConnTimeout() * 10);
        printTimestamp();
        Serial.printf("  Free heap: %d bytes\n", ESP.getFreeHeap());
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        deviceConnected = false;
        printTimestamp();
        Serial.println("========== CLIENT DISCONNECTED ==========");
        printTimestamp();
        Serial.printf("  Reason: 0x%02X (%d)\n", reason, reason);
        printTimestamp();
        Serial.printf("  Free heap: %d bytes\n", ESP.getFreeHeap());

        // Critical: restart advertising so the Pi can reconnect
        printTimestamp();
        Serial.println("  Restarting advertising...");
        NimBLEDevice::startAdvertising();
        printTimestamp();
        Serial.println("  Advertising restarted - ready for reconnection");
    }

    void onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo) override {
        printTimestamp();
        Serial.printf("MTU changed to: %d\n", MTU);
    }
};

// Characteristic callbacks - log reads/writes
class ButtonCharCallbacks : public NimBLECharacteristicCallbacks {
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        printTimestamp();
        Serial.printf("Characteristic READ by %s\n", connInfo.getAddress().toString().c_str());
    }

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
    delay(1000);  // Give serial monitor time to connect

    bootTime = millis();

    Serial.println("\n\n");
    Serial.println("============================================");
    Serial.println("  BLE Remote - ESP32-C3 GATT Server");
    Serial.println("  Step 1: Connection + Heartbeat Debug");
    Serial.println("============================================");
    printTimestamp();
    Serial.printf("Boot reason: %d\n", esp_reset_reason());
    printTimestamp();
    Serial.printf("Free heap at boot: %d bytes\n", ESP.getFreeHeap());
    printTimestamp();
    Serial.printf("Chip model: %s rev %d\n", ESP.getChipModel(), ESP.getChipRevision());

    // Initialize NimBLE
    printTimestamp();
    Serial.println("Initializing NimBLE...");
    NimBLEDevice::init("BLE-Remote");
    NimBLEDevice::setPower(20);  // Max TX power: +20 dBm (NimBLE 2.x takes dBm directly)
    printTimestamp();
    Serial.printf("TX power set to: %d dBm\n", NimBLEDevice::getPower());
    printTimestamp();
    Serial.printf("BLE address: %s\n", NimBLEDevice::getAddress().toString().c_str());

    // Create server
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    printTimestamp();
    Serial.println("GATT server created");

    // Create service
    NimBLEService* pService = pServer->createService(SERVICE_UUID);
    printTimestamp();
    Serial.printf("Service created: %s\n", SERVICE_UUID);

    // Create button characteristic (notify only for now)
    pButtonChar = pService->createCharacteristic(
        BUTTON_CHAR_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    pButtonChar->setCallbacks(new ButtonCharCallbacks());
    printTimestamp();
    Serial.printf("Button characteristic created: %s\n", BUTTON_CHAR_UUID);

    // Start service
    pService->start();
    printTimestamp();
    Serial.println("Service started");

    // Configure and start advertising
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->enableScanResponse(true);
    pAdvertising->start();

    printTimestamp();
    Serial.println("Advertising started - waiting for connections...");
    printTimestamp();
    Serial.printf("Free heap after init: %d bytes\n", ESP.getFreeHeap());
    Serial.println("--------------------------------------------\n");
}

void loop() {
    unsigned long now = millis();

    // Send heartbeat notification every 2 seconds when connected
    if (deviceConnected && (now - lastHeartbeat >= 2000)) {
        lastHeartbeat = now;
        heartbeatCounter++;

        // Pack counter as 4 bytes (little-endian)
        pButtonChar->setValue(heartbeatCounter);
        pButtonChar->notify();

        printTimestamp();
        Serial.printf("HEARTBEAT #%d sent (value: %d)\n", heartbeatCounter, heartbeatCounter);
    }

    // Track connection state transitions
    if (wasConnected && !deviceConnected) {
        wasConnected = false;
        printTimestamp();
        Serial.println("State: CONNECTED -> DISCONNECTED");
    }
    if (!wasConnected && deviceConnected) {
        wasConnected = true;
        heartbeatCounter = 0;  // Reset on new connection
        printTimestamp();
        Serial.println("State: DISCONNECTED -> CONNECTED (counter reset)");
    }

    // Memory report every 10 seconds
    if (now - lastMemReport >= 10000) {
        lastMemReport = now;
        printTimestamp();
        Serial.printf("STATUS: heap=%d bytes | connected=%s | heartbeats=%d\n",
            ESP.getFreeHeap(),
            deviceConnected ? "YES" : "NO",
            heartbeatCounter);
    }

    delay(10);  // Small delay to prevent watchdog issues
}
