#include "ble_jacket.h"

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "app_state.h"
#include "config.h"

namespace {
const NimBLEUUID kService("49535343-fe7d-4ae5-8fa9-9fafd205e455");
const NimBLEUUID kWrite("49535343-8841-43f4-a8d4-ecbe34729bb3");

NimBLEClient* client = nullptr;
NimBLERemoteCharacteristic* writeChar = nullptr;

bool findJacket(NimBLEAddress& out) {
    const jacket::Settings s = app_state::settings();
    if (s.jacketAddress[0]) {
        out = NimBLEAddress(std::string(s.jacketAddress), BLE_ADDR_PUBLIC);
        return true;
    }
    Serial.println("[ble] scanning for the jacket...");
    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->setActiveScan(true);
    NimBLEScanResults results = scan->start(kBleScanSeconds, false);
    for (int i = 0; i < results.getCount(); i++) {
        NimBLEAdvertisedDevice dev = results.getDevice(i);
        if (dev.getName().rfind(kJacketNamePrefix, 0) == 0) {
            out = dev.getAddress();
            Serial.printf("[ble] found %s (%s)\n", dev.getName().c_str(), out.toString().c_str());
            scan->clearResults();
            return true;
        }
    }
    scan->clearResults();
    Serial.println("[ble] no jacket found (switched on? Hello Fairy app closed? Mac listener stopped?)");
    return false;
}
}  // namespace

namespace ble_jacket {

void begin() {
    NimBLEDevice::init("");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    client = NimBLEDevice::createClient();
    client->setConnectTimeout(10);
}

bool ensureConnected() {
    const bool up = client->isConnected() && writeChar;
    app_state::status().jacket = up;
    if (up) return true;
    writeChar = nullptr;
    NimBLEAddress addr;
    if (!findJacket(addr)) return false;
    if (!client->connect(addr)) {
        Serial.println("[ble] connect failed");
        return false;
    }
    NimBLERemoteService* svc = client->getService(kService);
    writeChar = svc ? svc->getCharacteristic(kWrite) : nullptr;
    if (!writeChar) {
        Serial.println("[ble] jacket service missing - unexpected controller?");
        client->disconnect();
        return false;
    }
    Serial.printf("[ble] connected to jacket %s\n", addr.toString().c_str());
    app_state::status().jacket = true;
    return true;
}

bool send(const jacket::Packet& p) {
    return p.len > 0 && ensureConnected() && writeChar->writeValue(p.data, p.len, false);
}

bool play(const jacket::Step* steps, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (!send(steps[i].packet)) return false;
        if (steps[i].delayMs) delay(steps[i].delayMs);
    }
    return true;
}

}  // namespace ble_jacket
