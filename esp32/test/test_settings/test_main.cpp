#include <ArduinoJson.h>
#include <string.h>
#include <unity.h>

#include "settings_rules.h"

using namespace jacket;

static Settings base() {
    Settings s = defaultSettings();
    strcpy(s.wifiSsid, "Home");
    strcpy(s.wifiPass, "secret-wifi");
    strcpy(s.wsUrl, "wss://bbits.io/api/v1/ws/abc");
    return s;
}

static bool apply(const char* json, Settings& s, char* err) {
    return applySettingsJson(json, strlen(json), s, err, 128);
}

void test_defaults() {
    Settings s = defaultSettings();
    TEST_ASSERT_EQUAL_STRING("jacketsetup", s.hotspotPass);
    TEST_ASSERT_EQUAL_STRING("", s.wifiSsid);
    TEST_ASSERT_EQUAL_UINT32(1000, s.shows.mediumFrom);
    TEST_ASSERT_EQUAL(IdleMode::Off, s.idleMode);
    TEST_ASSERT_FALSE(isConfigured(s));
    TEST_ASSERT_TRUE(isConfigured(base()));
}

void test_apply_updates_only_given_fields() {
    Settings s = base();
    char err[128];
    TEST_ASSERT_TRUE(apply("{\"minSats\":21,\"smallColor\":320,\"bigSeconds\":12.5}", s, err));
    TEST_ASSERT_EQUAL_UINT32(21, s.shows.minSats);
    TEST_ASSERT_EQUAL_UINT16(320, s.shows.smallHue);
    TEST_ASSERT_EQUAL_UINT32(12500, s.shows.bigMs);
    TEST_ASSERT_EQUAL_STRING("Home", s.wifiSsid);
    TEST_ASSERT_EQUAL_STRING("secret-wifi", s.wifiPass);
}

void test_empty_secret_keeps_old_value() {
    Settings s = base();
    char err[128];
    TEST_ASSERT_TRUE(apply("{\"wifiPass\":\"\",\"wifiSsid\":\"Venue\"}", s, err));
    TEST_ASSERT_EQUAL_STRING("secret-wifi", s.wifiPass);
    TEST_ASSERT_EQUAL_STRING("Venue", s.wifiSsid);
    TEST_ASSERT_TRUE(apply("{\"wifiPass\":\"new-password\"}", s, err));
    TEST_ASSERT_EQUAL_STRING("new-password", s.wifiPass);
}

void test_open_wifi_network_allowed() {
    Settings s = base();
    char err[128];
    TEST_ASSERT_TRUE(apply("{\"wifiSsid\":\"Cafe\",\"wifiOpen\":true}", s, err));
    TEST_ASSERT_EQUAL_STRING("", s.wifiPass);
}

void test_invalid_input_rejected_and_nothing_changes() {
    const char* bad[] = {
        "{\"wifiSsid\":\"\"}",
        "{\"wifiSsid\":\"123456789012345678901234567890123\"}",
        "{\"wifiPass\":\"short\"}",
        "{\"wsUrl\":\"https://bbits.io/x\"}",
        "{\"jacketAddress\":\"zz:11\"}",
        "{\"hotspotPass\":\"1234567\"}",
        "{\"mediumFrom\":50000}",
        "{\"smallColor\":400}",
        "{\"bigSeconds\":0}",
        "{\"bigSeconds\":600}",
        "{\"minSats\":-5}",
        "{\"idle\":\"sparkle\"}",
        "{\"idlePreset\":300}",
        "{\"minSats\":\"ten\"}",
        "not json",
        "[1,2]",
    };
    for (const char* json : bad) {
        Settings s = base();
        const Settings before = s;
        char err[128] = "";
        TEST_ASSERT_FALSE_MESSAGE(apply(json, s, err), json);
        TEST_ASSERT_TRUE_MESSAGE(strlen(err) > 0, json);
        TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&before, &s, sizeof s, json);
    }
}

void test_idle_modes() {
    Settings s = base();
    char err[128];
    TEST_ASSERT_TRUE(apply("{\"idle\":\"color\",\"idleColor\":240}", s, err));
    TEST_ASSERT_EQUAL(IdleMode::Color, s.idleMode);
    TEST_ASSERT_EQUAL_UINT16(240, s.idleHue);
    TEST_ASSERT_TRUE(apply("{\"idle\":\"preset\",\"idlePreset\":7}", s, err));
    TEST_ASSERT_EQUAL(IdleMode::Preset, s.idleMode);
    TEST_ASSERT_EQUAL_UINT8(7, s.idlePreset);
    TEST_ASSERT_TRUE(apply("{\"idle\":\"off\"}", s, err));
    TEST_ASSERT_EQUAL(IdleMode::Off, s.idleMode);
}

void test_settings_json_never_contains_secrets() {
    Settings s = base();
    strcpy(s.hotspotPass, "hotspot-secret");
    char out[1024];
    size_t n = settingsToJson(s, out, sizeof out);
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_NULL(strstr(out, "secret-wifi"));
    TEST_ASSERT_NULL(strstr(out, "hotspot-secret"));
    TEST_ASSERT_NULL(strstr(out, "/api/v1/ws/abc"));  // websocket id is a secret too
    JsonDocument doc;
    TEST_ASSERT_FALSE(deserializeJson(doc, out));
    TEST_ASSERT_EQUAL_STRING("Home", doc["wifiSsid"]);
    TEST_ASSERT_TRUE(doc["wifiPassSet"].as<bool>());
    TEST_ASSERT_TRUE(doc["wsUrlSet"].as<bool>());
    TEST_ASSERT_EQUAL_STRING("bbits.io", doc["wsHost"]);
    TEST_ASSERT_EQUAL(1000, doc["mediumFrom"].as<int>());
    TEST_ASSERT_EQUAL_FLOAT(3.0, doc["mediumSeconds"].as<float>());
    TEST_ASSERT_EQUAL_STRING("off", doc["idle"]);
}

void test_payment_log_keeps_last_ten_newest_first() {
    PaymentLog log;
    for (int i = 1; i <= 12; i++) log.add(i * 100, Show::Small, i * 1000);
    TEST_ASSERT_EQUAL_UINT(10, log.size());
    TEST_ASSERT_EQUAL_INT64(1200, log.at(0).sats);
    TEST_ASSERT_EQUAL_UINT32(12000, log.at(0).uptimeMs);
    TEST_ASSERT_EQUAL_INT64(300, log.at(9).sats);
}

void test_button_hold_actions() {
    TEST_ASSERT_EQUAL(ButtonAction::None, buttonAction(500));
    TEST_ASSERT_EQUAL(ButtonAction::Hotspot, buttonAction(3000));
    TEST_ASSERT_EQUAL(ButtonAction::Hotspot, buttonAction(9999));
    TEST_ASSERT_EQUAL(ButtonAction::FactoryReset, buttonAction(10000));
}

void test_login_throttle() {
    LoginThrottle t;
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_TRUE(t.allowed(1000));
        t.failed(1000);
    }
    TEST_ASSERT_FALSE(t.allowed(1000));
    TEST_ASSERT_FALSE(t.allowed(30999));
    TEST_ASSERT_TRUE(t.allowed(31000));
    t.failed(31000);
    t.succeeded();
    TEST_ASSERT_TRUE(t.allowed(31001));
}

void test_constant_time_equal() {
    TEST_ASSERT_TRUE(constantTimeEqual("abc", "abc"));
    TEST_ASSERT_FALSE(constantTimeEqual("abc", "abd"));
    TEST_ASSERT_FALSE(constantTimeEqual("abc", "abcd"));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_defaults);
    RUN_TEST(test_apply_updates_only_given_fields);
    RUN_TEST(test_empty_secret_keeps_old_value);
    RUN_TEST(test_open_wifi_network_allowed);
    RUN_TEST(test_invalid_input_rejected_and_nothing_changes);
    RUN_TEST(test_idle_modes);
    RUN_TEST(test_settings_json_never_contains_secrets);
    RUN_TEST(test_payment_log_keeps_last_ten_newest_first);
    RUN_TEST(test_button_hold_actions);
    RUN_TEST(test_login_throttle);
    RUN_TEST(test_constant_time_equal);
    return UNITY_END();
}
