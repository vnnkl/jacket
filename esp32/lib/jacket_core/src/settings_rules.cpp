#include "settings_rules.h"

#include <ArduinoJson.h>
#include <stdio.h>
#include <string.h>

namespace jacket {

namespace {
constexpr size_t kMinPass = 8, kMaxPass = 63, kMaxSsid = 32;
constexpr uint32_t kMaxSats = 1000000000;

struct Ctx {
    Settings& s;
    char* err;
    size_t errLen;
    bool fail(const char* msg) {
        snprintf(err, errLen, "%s", msg);
        return false;
    }
};

bool isMac(const char* v) {
    if (strlen(v) != 17) return false;
    for (int i = 0; i < 17; i++) {
        const char c = v[i];
        const bool hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (i % 3 == 2 ? c != ':' : !hex) return false;
    }
    return true;
}

void copyStr(char* dst, size_t cap, const char* src) { snprintf(dst, cap, "%s", src); }

// A password field: absent or "" keeps the stored value; otherwise 8-63 characters.
bool applyPassword(JsonVariantConst v, char* dst, size_t cap, const char* what, Ctx& c) {
    if (v.isNull()) return true;
    if (!v.is<const char*>()) return c.fail(what);
    const char* p = v.as<const char*>();
    if (!*p) return true;
    const size_t n = strlen(p);
    if (n < kMinPass || n > kMaxPass) return c.fail(what);
    copyStr(dst, cap, p);
    return true;
}

bool applyUint(JsonVariantConst v, uint32_t max, uint32_t& dst, const char* what, Ctx& c) {
    if (v.isNull()) return true;
    if (!v.is<uint32_t>() || v.as<uint32_t>() > max) return c.fail(what);
    dst = v.as<uint32_t>();
    return true;
}

bool applyHue(JsonVariantConst v, uint16_t& dst, const char* what, Ctx& c) {
    uint32_t hue = dst;
    if (!applyUint(v, 359, hue, what, c)) return false;
    dst = static_cast<uint16_t>(hue);
    return true;
}

bool applySeconds(JsonVariantConst v, uint32_t& dstMs, const char* what, Ctx& c) {
    if (v.isNull()) return true;
    if (!v.is<double>() && !v.is<long>()) return c.fail(what);
    const double sec = v.as<double>();
    if (!(sec >= 0.1 && sec <= kMaxShowSeconds)) return c.fail(what);
    dstMs = static_cast<uint32_t>(sec * 1000 + 0.5);
    return true;
}

bool applyWifi(JsonObjectConst o, Ctx& c) {
    JsonVariantConst ssid = o["wifiSsid"];
    if (!ssid.isNull()) {
        if (!ssid.is<const char*>()) return c.fail("Wi-Fi name must be text");
        const size_t n = strlen(ssid.as<const char*>());
        if (n == 0 || n > kMaxSsid) return c.fail("Wi-Fi name must be 1-32 characters");
        copyStr(c.s.wifiSsid, sizeof c.s.wifiSsid, ssid.as<const char*>());
    }
    if (o["wifiOpen"].as<bool>()) {
        c.s.wifiPass[0] = '\0';
        return true;
    }
    return applyPassword(o["wifiPass"], c.s.wifiPass, sizeof c.s.wifiPass,
                         "Wi-Fi password must be 8-63 characters (or tick 'open network')", c);
}

bool applyConnections(JsonObjectConst o, Ctx& c) {
    JsonVariantConst url = o["wsUrl"];
    if (!url.isNull()) {
        WsUrl parsed;
        if (!url.is<const char*>()) return c.fail("Websocket URL must be text");
        const char* u = url.as<const char*>();
        if (*u && (strlen(u) >= sizeof c.s.wsUrl || !parseWsUrl(u, parsed)))
            return c.fail("Websocket URL must start with ws:// or wss://");
        if (*u) copyStr(c.s.wsUrl, sizeof c.s.wsUrl, u);
    }
    JsonVariantConst mac = o["jacketAddress"];
    if (!mac.isNull()) {
        if (!mac.is<const char*>() || (*mac.as<const char*>() && !isMac(mac.as<const char*>())))
            return c.fail("Jacket address must look like aa:bb:cc:dd:ee:ff, or be empty to scan");
        copyStr(c.s.jacketAddress, sizeof c.s.jacketAddress, mac.as<const char*>());
    }
    return applyPassword(o["hotspotPass"], c.s.hotspotPass, sizeof c.s.hotspotPass,
                         "Hotspot password must be 8-63 characters", c);
}

bool applyShows(JsonObjectConst o, Ctx& c) {
    ShowConfig& sh = c.s.shows;
    return applyUint(o["minSats"], kMaxSats, sh.minSats, "Minimum must be a whole number of sats", c) &&
           applyUint(o["mediumFrom"], kMaxSats, sh.mediumFrom, "Medium-from must be a whole number of sats", c) &&
           applyUint(o["bigFrom"], kMaxSats, sh.bigFrom, "Big-from must be a whole number of sats", c) &&
           applyHue(o["smallColor"], sh.smallHue, "Small-show colour must be a hue 0-359", c) &&
           applySeconds(o["smallSeconds"], sh.smallMs, "Small-show length must be 0.1-60 s", c) &&
           applySeconds(o["mediumSeconds"], sh.mediumMs, "Medium-show length must be 0.1-60 s", c) &&
           applySeconds(o["bigSeconds"], sh.bigMs, "Big-show length must be 0.1-60 s", c) &&
           (sh.valid() || c.fail("Need minimum <= medium-from < big-from"));
}

bool applyIdle(JsonObjectConst o, Ctx& c) {
    JsonVariantConst mode = o["idle"];
    if (!mode.isNull()) {
        const char* m = mode.is<const char*>() ? mode.as<const char*>() : "";
        if (!strcmp(m, "off")) c.s.idleMode = IdleMode::Off;
        else if (!strcmp(m, "color")) c.s.idleMode = IdleMode::Color;
        else if (!strcmp(m, "preset")) c.s.idleMode = IdleMode::Preset;
        else return c.fail("Between payments must be off, color or preset");
    }
    uint32_t preset = c.s.idlePreset;
    if (!applyUint(o["idlePreset"], 255, preset, "Preset must be 0-255", c)) return false;
    c.s.idlePreset = static_cast<uint8_t>(preset);
    return applyHue(o["idleColor"], c.s.idleHue, "Idle colour must be a hue 0-359", c);
}

const char* idleName(IdleMode m) {
    switch (m) {
        case IdleMode::Color: return "color";
        case IdleMode::Preset: return "preset";
        default: return "off";
    }
}
}  // namespace

Settings defaultSettings() {
    Settings s;
    memset(&s, 0, sizeof s);  // deterministic padding: settings are stored and compared as a blob
    copyStr(s.hotspotPass, sizeof s.hotspotPass, kDefaultHotspotPass);
    s.shows = ShowConfig{};
    s.idleMode = IdleMode::Off;
    s.idleHue = 0;
    s.idlePreset = 0;
    return s;
}

bool isConfigured(const Settings& s) { return s.wifiSsid[0] && s.wsUrl[0]; }

bool applySettingsJson(const char* json, size_t len, Settings& s, char* err, size_t errLen) {
    JsonDocument doc;
    if (deserializeJson(doc, json, len) || !doc.is<JsonObjectConst>()) {
        snprintf(err, errLen, "Settings must be a JSON object");
        return false;
    }
    Settings next = s;
    Ctx c{next, err, errLen};
    JsonObjectConst o = doc.as<JsonObjectConst>();
    if (!applyWifi(o, c) || !applyConnections(o, c) || !applyShows(o, c) || !applyIdle(o, c)) return false;
    s = next;
    return true;
}

size_t settingsToJson(const Settings& s, char* out, size_t outLen) {
    JsonDocument doc;
    WsUrl url;
    doc["wifiSsid"] = s.wifiSsid;
    doc["wifiPassSet"] = s.wifiPass[0] != '\0';
    doc["wsUrlSet"] = s.wsUrl[0] != '\0';
    doc["wsHost"] = parseWsUrl(s.wsUrl, url) ? url.host : "";
    doc["jacketAddress"] = s.jacketAddress;
    doc["hotspotPassIsDefault"] = strcmp(s.hotspotPass, kDefaultHotspotPass) == 0;
    doc["minSats"] = s.shows.minSats;
    doc["mediumFrom"] = s.shows.mediumFrom;
    doc["bigFrom"] = s.shows.bigFrom;
    doc["smallColor"] = s.shows.smallHue;
    doc["smallSeconds"] = s.shows.smallMs / 1000.0;
    doc["mediumSeconds"] = s.shows.mediumMs / 1000.0;
    doc["bigSeconds"] = s.shows.bigMs / 1000.0;
    doc["idle"] = idleName(s.idleMode);
    doc["idleColor"] = s.idleHue;
    doc["idlePreset"] = s.idlePreset;
    return serializeJson(doc, out, outLen);
}

void PaymentLog::add(int64_t sats, Show show, uint32_t uptimeMs) {
    entries_[next_] = {sats, show, uptimeMs};
    next_ = (next_ + 1) % kCapacity;
    if (count_ < kCapacity) count_++;
}

const PaymentEntry& PaymentLog::at(size_t i) const {
    return entries_[(next_ + kCapacity - 1 - (i % kCapacity)) % kCapacity];
}

ButtonAction buttonAction(uint32_t heldMs) {
    if (heldMs >= kHoldForResetMs) return ButtonAction::FactoryReset;
    if (heldMs >= kHoldForHotspotMs) return ButtonAction::Hotspot;
    return ButtonAction::None;
}

void LoginThrottle::failed(uint32_t nowMs) {
    if (++failures_ >= kMaxFailures) {
        failures_ = 0;
        lockedUntil_ = nowMs + kLockMs;
    }
}

bool constantTimeEqual(const char* a, const char* b) {
    const size_t la = strlen(a), lb = strlen(b);
    unsigned char diff = la != lb;
    for (size_t i = 0; i < la; i++) diff |= static_cast<unsigned char>(a[i] ^ b[i % (lb ? lb : 1)]);
    return diff == 0;
}

}  // namespace jacket
