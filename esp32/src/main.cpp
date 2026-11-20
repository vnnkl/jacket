// Wi-Fi -> payment websocket -> Bluetooth LE -> jacket off; a small/medium/big show per payment.
#include <Arduino.h>
#include <WiFi.h>

#include "ble_jacket.h"
#include "config.h"
#include "jacket_core.h"
#include "payment_socket.h"

namespace {
QueueHandle_t payments;
jacket::ShowConfig showConfig;  // defaults; the admin page makes these editable
jacket::Step showSteps[jacket::kMaxShowSteps];

void queuePayment(int64_t sats) {
    if (xQueueSend(payments, &sats, 0) != pdTRUE) Serial.println("[main] payment queue full; skipping one");
}

void playPayment(int64_t sats) {
    const jacket::Show show = jacket::pickShow(sats, showConfig);
    const size_t n = jacket::buildShow(show, showConfig, showSteps, jacket::kMaxShowSteps);
    Serial.printf("[main] %s show for %lld sats\n", jacket::showName(show), static_cast<long long>(sats));
    if (n == 0) return;
    if (!ble_jacket::play(showSteps, n) || !ble_jacket::send(jacket::power(false))) {
        Serial.println("[ble] lost the jacket mid-show; will reconnect");
    }
}

// Owns the Bluetooth link: jacket stays off; one show per queued payment, then off again.
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
        int64_t sats;
        if (xQueueReceive(payments, &sats, pdMS_TO_TICKS(1000)) == pdTRUE) playPayment(sats);
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
    payments = xQueueCreate(kPaymentQueue, sizeof(int64_t));
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
