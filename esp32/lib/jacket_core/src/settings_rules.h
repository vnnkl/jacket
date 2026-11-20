// Admin settings: defaults, validation of form input, JSON for the page (never secrets),
// plus small helpers for the admin (payment log, BOOT button, login throttle). No Arduino here.
#pragma once
#include <stddef.h>
#include <stdint.h>

#include "jacket_core.h"

namespace jacket {

enum class IdleMode : uint8_t { Off, Color, Preset };

// Plain data so it can be stored as one blob; always start from defaultSettings().
struct Settings {
    char wifiSsid[33];
    char wifiPass[64];
    char wsUrl[256];
    char jacketAddress[18];  // "aa:bb:cc:dd:ee:ff" or empty = scan for "Hello Fairy"
    char hotspotPass[64];
    ShowConfig shows;
    IdleMode idleMode;
    uint16_t idleHue;
    uint8_t idlePreset;
};

constexpr const char* kDefaultHotspotPass = "jacketsetup";
constexpr uint32_t kMaxShowSeconds = kMaxShowMs / 1000;

Settings defaultSettings();
bool isConfigured(const Settings& s);  // has Wi-Fi and a websocket URL

// Applies the fields present in a JSON object to `s`. Empty secret fields keep the stored value.
// On any invalid field nothing changes, false is returned and `err` explains it.
bool applySettingsJson(const char* json, size_t len, Settings& s, char* err, size_t errLen);

// Settings for the admin page. Secrets (passwords, websocket path) are reported only as "...Set".
size_t settingsToJson(const Settings& s, char* out, size_t outLen);

struct PaymentEntry {
    int64_t sats;
    Show show;
    uint32_t uptimeMs;
};

class PaymentLog {
   public:
    static constexpr size_t kCapacity = 10;
    void add(int64_t sats, Show show, uint32_t uptimeMs);
    size_t size() const { return count_; }
    const PaymentEntry& at(size_t i) const;  // 0 = newest

   private:
    PaymentEntry entries_[kCapacity] = {};
    size_t next_ = 0;
    size_t count_ = 0;
};

enum class ButtonAction { None, Hotspot, FactoryReset };
constexpr uint32_t kHoldForHotspotMs = 3000;
constexpr uint32_t kHoldForResetMs = 10000;
ButtonAction buttonAction(uint32_t heldMs);

class LoginThrottle {
   public:
    static constexpr int kMaxFailures = 5;
    static constexpr uint32_t kLockMs = 30000;
    bool allowed(uint32_t nowMs) const { return static_cast<int32_t>(nowMs - lockedUntil_) >= 0; }
    void failed(uint32_t nowMs);
    void succeeded() { failures_ = 0; }

   private:
    int failures_ = 0;
    uint32_t lockedUntil_ = 0;
};

bool constantTimeEqual(const char* a, const char* b);

}  // namespace jacket
