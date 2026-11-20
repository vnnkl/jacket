// Wi-Fi -> payment websocket -> Bluetooth LE -> jacket: off between payments, a small/medium/big show
// for each one. Settings come from the admin page (http://jacket.local or the Jacket-Setup hotspot).
#include <Arduino.h>

#include "app_state.h"
#include "ble_jacket.h"
#include "config.h"
#include "jacket_core.h"
#include "payment_socket.h"
#include "settings_store.h"
#include "web_admin.h"
#include "wifi_setup.h"

namespace {
jacket::Step showSteps[jacket::kMaxShowSteps];

bool sendIdle(const jacket::Settings& s) {
    switch (s.idleMode) {
        case jacket::IdleMode::Color:
            return ble_jacket::send(jacket::power(true)) && ble_jacket::send(jacket::color(s.idleHue));
        case jacket::IdleMode::Preset: {
            const uint8_t payload[] = {2, s.idlePreset, uint8_t(jacket::kMaxLevel >> 8), uint8_t(jacket::kMaxLevel)};
            return ble_jacket::send(jacket::power(true)) && ble_jacket::send(jacket::frame(0x03, payload, sizeof payload));
        }
        default:
            return ble_jacket::send(jacket::power(false));
    }
}

void playShow(int64_t sats, bool test) {
    const jacket::Settings s = app_state::settings();
    const jacket::Show show = jacket::pickShow(sats, s.shows);
    if (!test) app_state::logPayment(sats, show);
    Serial.printf("[main] %s%s show for %lld sats\n", test ? "test " : "", jacket::showName(show),
                  static_cast<long long>(sats));
    const size_t n = jacket::buildShow(show, s.shows, showSteps, jacket::kMaxShowSteps);
    if (n && !(ble_jacket::play(showSteps, n) && sendIdle(s))) Serial.println("[ble] lost the jacket mid-show");
}

// Owns the Bluetooth link: one show per queued payment, then back to the idle look.
void jacketTask(void*) {
    ble_jacket::begin();
    bool idleSent = false;
    for (;;) {
        if (!ble_jacket::ensureConnected()) {
            idleSent = false;
            delay(kBleRetryMs);
            continue;
        }
        if (!idleSent) idleSent = sendIdle(app_state::settings());
        int64_t sats;
        bool test;
        if (app_state::nextShow(sats, test, 1000)) playShow(sats, test);
    }
}

void onPayment(int64_t sats) {
    if (!app_state::queueShow(sats, false)) Serial.println("[main] show queue full; skipping a payment");
}
}  // namespace

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.printf("\n[main] jacket payment light %s starting\n", FIRMWARE_VERSION);
    app_state::begin();
    app_state::setSettings(settings_store::load());
    xTaskCreate(jacketTask, "jacket", 8192, nullptr, 1, nullptr);
    wifi_setup::begin();
    web_admin::begin();
    const jacket::Settings s = app_state::settings();
    if (s.wsUrl[0] && !payment_socket::begin(s.wsUrl, onPayment))
        Serial.println("[main] saved websocket URL is invalid; fix it on the admin page");
}

void loop() {
    wifi_setup::loop();
    web_admin::loop();
    payment_socket::loop();
    delay(2);
}
