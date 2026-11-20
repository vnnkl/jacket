// Wi-Fi -> payment websocket -> Bluetooth LE -> jacket off, 3 s disco rainbow for every payment.
#include <Arduino.h>
#include <WiFi.h>

#include "ble_jacket.h"
#include "config.h"
#include "jacket_core.h"
#include "payment_socket.h"

namespace {
QueueHandle_t payments;
jacket::Step disco[jacket::kMaxDiscoSteps];
size_t discoSteps = 0;

void queuePayment() {
    uint8_t token = 1;
    if (xQueueSend(payments, &token, 0) != pdTRUE) Serial.println("[main] payment queue full; skipping one");
}

// Owns the Bluetooth link: jacket stays off; one disco per queued payment, then off again.
void jacketTask(void*) {
    ble_jacket::begin();
    bool wasConnected = false;
    for (;;) {
        if (!ble_jacket::ensureConnected()) {
            wasConnected = false;
            delay(kBleRetryMs);
            continue;
        }
        if (!wasConnected) wasConnected = ble_jacket::send(jacket::power(false));
        uint8_t token;
        if (xQueueReceive(payments, &token, pdMS_TO_TICKS(1000)) != pdTRUE) continue;
        if (!ble_jacket::play(disco, discoSteps) || !ble_jacket::send(jacket::power(false))) {
            Serial.println("[ble] lost the jacket mid-effect; will reconnect");
        }
    }
}

void connectWifi() {
    if (WiFi.status() == WL_CONNECTED) return;
    Serial.printf("[wifi] connecting to %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < kWifiRetryMs) {
        delay(250);
        Serial.print('.');
    }
    Serial.println(WiFi.status() == WL_CONNECTED ? " connected" : " failed, retrying");
}
}  // namespace

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n[main] jacket payment light starting");
    discoSteps = jacket::discoRainbow(kDiscoMs, disco, jacket::kMaxDiscoSteps);
    payments = xQueueCreate(kPaymentQueue, sizeof(uint8_t));
    xTaskCreate(jacketTask, "jacket", 8192, nullptr, 1, nullptr);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    connectWifi();
    if (!payment_socket::begin(PAYMENT_WS_URL, queuePayment)) {
        Serial.println("[main] PAYMENT_WS_URL is not a ws:// or wss:// URL - fix secrets.h and reflash");
    }
}

void loop() {
    connectWifi();
    payment_socket::loop();
    delay(5);
}
