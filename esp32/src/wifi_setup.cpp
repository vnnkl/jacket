#include "wifi_setup.h"

#include <Arduino.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>

#include "app_state.h"
#include "config.h"
#include "settings_store.h"

namespace {
DNSServer dns;
bool hotspotOn = false;
bool mdnsOn = false;
uint32_t staStartedMs = 0;
uint32_t buttonDownMs = 0;

void startHotspot(const char* why) {
    if (hotspotOn) return;
    const jacket::Settings s = app_state::settings();
    WiFi.mode(s.wifiSsid[0] ? WIFI_AP_STA : WIFI_AP);
    if (!WiFi.softAP(kHotspotName, s.hotspotPass)) {
        Serial.println("[wifi] could not start the hotspot");
        return;
    }
    dns.start(53, "*", WiFi.softAPIP());  // every name -> us, so phones open the setup page
    hotspotOn = true;
    app_state::status().hotspot = true;
    Serial.printf("[wifi] hotspot '%s' open (%s); setup page http://%s\n", kHotspotName, why,
                  WiFi.softAPIP().toString().c_str());
}

void startStation() {
    const jacket::Settings s = app_state::settings();
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.setHostname(kHostname);
    WiFi.begin(s.wifiSsid, s.wifiPass[0] ? s.wifiPass : nullptr);
    staStartedMs = millis();
    Serial.printf("[wifi] joining '%s'\n", s.wifiSsid);
}

void trackStation() {
    const bool up = WiFi.status() == WL_CONNECTED;
    if (up && !app_state::status().wifi) {
        Serial.printf("[wifi] connected, admin page: http://%s.local or http://%s\n", kHostname,
                      WiFi.localIP().toString().c_str());
        if (!mdnsOn && MDNS.begin(kHostname)) {
            MDNS.addService("http", "tcp", 80);
            mdnsOn = true;
        }
    }
    if (!up && app_state::status().wifi) Serial.println("[wifi] connection lost, retrying");
    app_state::status().wifi = up;
    if (!up && !hotspotOn && millis() - staStartedMs > kWifiJoinTimeoutMs) startHotspot("could not join Wi-Fi");
}

void watchButton() {
    const bool down = digitalRead(kBootPin) == LOW;
    if (down && !buttonDownMs) buttonDownMs = millis();
    if (down || !buttonDownMs) return;
    const uint32_t held = millis() - buttonDownMs;
    buttonDownMs = 0;
    switch (jacket::buttonAction(held)) {
        case jacket::ButtonAction::Hotspot:
            startHotspot("BOOT button");
            break;
        case jacket::ButtonAction::FactoryReset:
            Serial.println("[wifi] BOOT held 10 s: factory reset");
            settings_store::factoryReset();
            delay(200);
            ESP.restart();
            break;
        default:
            break;
    }
}
}  // namespace

namespace wifi_setup {

void begin() {
    pinMode(kBootPin, INPUT_PULLUP);
    if (jacket::isConfigured(app_state::settings())) {
        startStation();
    } else {
        startHotspot("not set up yet");
    }
}

void loop() {
    if (hotspotOn) dns.processNextRequest();
    if (app_state::settings().wifiSsid[0]) trackStation();
    watchButton();
}

bool onHotspot(const IPAddress& localIp) { return hotspotOn && localIp == WiFi.softAPIP(); }

}  // namespace wifi_setup
