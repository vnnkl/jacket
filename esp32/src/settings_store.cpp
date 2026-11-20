#include "settings_store.h"

#include <Arduino.h>
#include <Preferences.h>
#include <mbedtls/sha256.h>

namespace {
constexpr const char* kNamespace = "jacket";
constexpr const char* kSettingsKey = "settings";
constexpr const char* kLayoutKey = "layout";
constexpr const char* kAdminKey = "admin";        // "<salt hex>:<sha256 hex>"
constexpr uint32_t kLayout = 1000 + sizeof(jacket::Settings);  // bump the base when fields change meaning
constexpr size_t kSaltBytes = 16;

void toHex(const uint8_t* in, size_t n, char* out) {
    static const char* d = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) {
        out[2 * i] = d[in[i] >> 4];
        out[2 * i + 1] = d[in[i] & 15];
    }
    out[2 * n] = '\0';
}

void hashPassword(const char* saltHex, const char* password, char* outHex) {
    uint8_t digest[32];
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, reinterpret_cast<const uint8_t*>(saltHex), strlen(saltHex));
    mbedtls_sha256_update(&ctx, reinterpret_cast<const uint8_t*>(password), strlen(password));
    mbedtls_sha256_finish(&ctx, digest);
    mbedtls_sha256_free(&ctx);
    toHex(digest, sizeof digest, outHex);
}
}  // namespace

namespace settings_store {

jacket::Settings load() {
    Preferences p;
    jacket::Settings s = jacket::defaultSettings();
    if (!p.begin(kNamespace, true)) return s;
    if (p.getUInt(kLayoutKey, 0) == kLayout && p.getBytesLength(kSettingsKey) == sizeof s) {
        p.getBytes(kSettingsKey, &s, sizeof s);
    } else if (p.isKey(kSettingsKey)) {
        Serial.println("[settings] stored settings are from another firmware layout; using defaults");
    }
    p.end();
    return s;
}

bool save(const jacket::Settings& s) {
    Preferences p;
    if (!p.begin(kNamespace, false)) return false;
    const bool ok = p.putBytes(kSettingsKey, &s, sizeof s) == sizeof s && p.putUInt(kLayoutKey, kLayout) > 0;
    p.end();
    return ok;
}

bool hasAdminPassword() {
    Preferences p;
    if (!p.begin(kNamespace, true)) return false;
    const bool has = p.isKey(kAdminKey);
    p.end();
    return has;
}

bool setAdminPassword(const char* password) {
    uint8_t salt[kSaltBytes];
    esp_fill_random(salt, sizeof salt);
    char record[2 * kSaltBytes + 1 + 64 + 1];
    toHex(salt, sizeof salt, record);
    record[2 * kSaltBytes] = '\0';
    char hash[65];
    hashPassword(record, password, hash);
    record[2 * kSaltBytes] = ':';
    memcpy(record + 2 * kSaltBytes + 1, hash, sizeof hash);
    Preferences p;
    if (!p.begin(kNamespace, false)) return false;
    const bool ok = p.putString(kAdminKey, record) > 0;
    p.end();
    return ok;
}

bool checkAdminPassword(const char* password) {
    Preferences p;
    if (!p.begin(kNamespace, true)) return false;
    String record = p.getString(kAdminKey, "");
    p.end();
    const int colon = record.indexOf(':');
    if (colon <= 0) return false;
    char hash[65];
    hashPassword(record.substring(0, colon).c_str(), password, hash);
    return jacket::constantTimeEqual(hash, record.c_str() + colon + 1);
}

void factoryReset() {
    Preferences p;
    if (p.begin(kNamespace, false)) {
        p.clear();
        p.end();
    }
}

}  // namespace settings_store
