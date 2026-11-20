#include <unity.h>
#include <string.h>
#include "jacket_core.h"
#include "golden_shows.h"

using namespace jacket;

static void assert_packet(const Packet& p, const uint8_t* bytes, size_t n) {
    TEST_ASSERT_EQUAL_UINT(n, p.len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(bytes, p.data, n);
}

void test_power_matches_bytes_confirmed_on_device() {
    const uint8_t off[] = {0xAA, 0x02, 0x01, 0x00, 0xAD};
    const uint8_t on[] = {0xAA, 0x02, 0x01, 0x01, 0xAE};
    assert_packet(power(false), off, 5);
    assert_packet(power(true), on, 5);
}

void test_color_matches_python_client() {
    // color(300, 1000, 500) from the Mac CLI
    const uint8_t want[] = {0xAA, 0x03, 0x07, 0x01, 0x01, 0x2C, 0x03, 0xE8, 0x01, 0xF4, 0xC2};
    assert_packet(color(300, 1000, 500), want, sizeof want);
}

void test_color_wraps_hue_and_clamps_levels() {
    Packet a = color(360), b = color(0);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(b.data, a.data, b.len);
    Packet c = color(0, 5000, 5000), d = color(0, 1000, 1000);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(d.data, c.data, d.len);
}

void test_disco_lasts_three_seconds_and_ends_on() {
    Step steps[kMaxDiscoSteps];
    size_t n = discoRainbow(3000, steps, kMaxDiscoSteps);
    TEST_ASSERT_TRUE(n > 20);
    uint32_t total = 0;
    for (size_t i = 0; i < n; i++) total += steps[i].delayMs;
    TEST_ASSERT_UINT32_WITHIN(20, 3000, total);
    const uint8_t on[] = {0xAA, 0x02, 0x01, 0x01, 0xAE};
    assert_packet(steps[n - 1].packet, on, 5);
}

void test_disco_respects_capacity() {
    Step steps[6];
    size_t n = discoRainbow(3000, steps, 6);
    TEST_ASSERT_TRUE(n <= 6);
}

static int64_t sats(const char* s) { return incomingSats(s, strlen(s)); }

void test_incoming_sats() {
    TEST_ASSERT_EQUAL_INT64(39431, sats("{\"wallet_balance\":1,\"payment\":{\"amount\":39431000,\"fee\":0,\"bolt11\":\"lnbc...\"}}"));
    TEST_ASSERT_EQUAL_INT64(10, sats("{\"payment\":{\"amount\":10000,\"status\":\"success\"}}"));
    TEST_ASSERT_EQUAL_INT64(1, sats("{\"payment\":{\"amount\":400}}"));
}

void test_test_triggers() {
    TEST_ASSERT_EQUAL_INT64(5000, sats("jacket-test"));
    TEST_ASSERT_EQUAL_INT64(100, sats("jacket-test:small"));
    TEST_ASSERT_EQUAL_INT64(5000, sats("jacket-test:medium"));
    TEST_ASSERT_EQUAL_INT64(21000, sats("jacket-test:big"));
    TEST_ASSERT_EQUAL_INT64(777, sats("jacket-test:777"));
    TEST_ASSERT_EQUAL_INT64(0, sats("jacket-test:nope"));
}

void test_non_payments_are_ignored() {
    TEST_ASSERT_EQUAL_INT64(0, sats("{\"payment\":{\"amount\":-1000,\"status\":\"success\"}}"));
    TEST_ASSERT_EQUAL_INT64(0, sats("{\"payment\":{\"amount\":1000,\"status\":\"pending\"}}"));
    TEST_ASSERT_EQUAL_INT64(0, sats("{\"payment\":{\"amount\":1000,\"status\":\"failed\"}}"));
    TEST_ASSERT_EQUAL_INT64(0, sats("{\"payment\":{\"amount\":\"lots\"}}"));
    TEST_ASSERT_EQUAL_INT64(0, sats("{\"payment\":{\"amount\":true}}"));
    TEST_ASSERT_EQUAL_INT64(0, sats("{\"payment\":\"oops\"}"));
    TEST_ASSERT_EQUAL_INT64(0, sats("{\"hello\":1}"));
    TEST_ASSERT_EQUAL_INT64(0, sats("not json"));
    TEST_ASSERT_EQUAL_INT64(0, sats(""));
}

static Step buf[kMaxShowSteps];

static void assert_matches_golden(const GoldenStep* golden, size_t goldenLen, size_t n) {
    TEST_ASSERT_EQUAL_UINT(goldenLen, n);
    for (size_t i = 0; i < n; i++) {
        TEST_ASSERT_EQUAL_UINT(golden[i].len, buf[i].packet.len);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(golden[i].data, buf[i].packet.data, golden[i].len);
        TEST_ASSERT_UINT32_WITHIN(1, golden[i].delayMs, buf[i].delayMs);
    }
}

void test_small_show_matches_mac() {
    size_t n = smallPulse(60, 2000, buf, kMaxShowSteps);
    assert_matches_golden(kGolden_small, sizeof kGolden_small / sizeof *kGolden_small, n);
}

void test_medium_show_matches_mac() {
    size_t n = discoRainbow(3000, buf, kMaxShowSteps);
    assert_matches_golden(kGolden_medium, sizeof kGolden_medium / sizeof *kGolden_medium, n);
}

void test_big_show_matches_mac() {
    size_t n = bigShow(10000, buf, kMaxShowSteps);
    assert_matches_golden(kGolden_big, sizeof kGolden_big / sizeof *kGolden_big, n);
}

void test_bands() {
    ShowConfig cfg;
    TEST_ASSERT_EQUAL(Show::None, pickShow(0, cfg));
    TEST_ASSERT_EQUAL(Show::Small, pickShow(1, cfg));
    TEST_ASSERT_EQUAL(Show::Small, pickShow(999, cfg));
    TEST_ASSERT_EQUAL(Show::Medium, pickShow(1000, cfg));
    TEST_ASSERT_EQUAL(Show::Medium, pickShow(9999, cfg));
    TEST_ASSERT_EQUAL(Show::Big, pickShow(10000, cfg));
    cfg.minSats = 100;
    TEST_ASSERT_EQUAL(Show::None, pickShow(99, cfg));
    TEST_ASSERT_EQUAL(Show::Small, pickShow(100, cfg));
}

void test_show_config_validation() {
    ShowConfig cfg;
    TEST_ASSERT_TRUE(cfg.valid());
    cfg.mediumFrom = 20000;
    TEST_ASSERT_FALSE(cfg.valid());
    cfg = ShowConfig{};
    cfg.smallMs = 0;
    TEST_ASSERT_FALSE(cfg.valid());
    cfg = ShowConfig{};
    cfg.bigMs = 120000;
    TEST_ASSERT_FALSE(cfg.valid());
}

void test_build_show_uses_config() {
    ShowConfig cfg;
    cfg.mediumMs = 2000;
    size_t n = buildShow(Show::Medium, cfg, buf, kMaxShowSteps);
    uint32_t total = 0;
    for (size_t i = 0; i < n; i++) total += buf[i].delayMs;
    TEST_ASSERT_UINT32_WITHIN(20, 2000, total);
    TEST_ASSERT_EQUAL_UINT(0, buildShow(Show::None, cfg, buf, kMaxShowSteps));
}

void test_parse_ws_url() {
    WsUrl u;
    TEST_ASSERT_TRUE(parseWsUrl("wss://bbits.io/api/v1/ws/abc", u));
    TEST_ASSERT_TRUE(u.secure);
    TEST_ASSERT_EQUAL_STRING("bbits.io", u.host);
    TEST_ASSERT_EQUAL_UINT16(443, u.port);
    TEST_ASSERT_EQUAL_STRING("/api/v1/ws/abc", u.path);

    TEST_ASSERT_TRUE(parseWsUrl("ws://10.0.0.2:8080", u));
    TEST_ASSERT_FALSE(u.secure);
    TEST_ASSERT_EQUAL_STRING("10.0.0.2", u.host);
    TEST_ASSERT_EQUAL_UINT16(8080, u.port);
    TEST_ASSERT_EQUAL_STRING("/", u.path);

    TEST_ASSERT_FALSE(parseWsUrl("https://bbits.io/x", u));
    TEST_ASSERT_FALSE(parseWsUrl("wss://", u));
    TEST_ASSERT_FALSE(parseWsUrl("wss://host:99999/x", u));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_power_matches_bytes_confirmed_on_device);
    RUN_TEST(test_color_matches_python_client);
    RUN_TEST(test_color_wraps_hue_and_clamps_levels);
    RUN_TEST(test_disco_lasts_three_seconds_and_ends_on);
    RUN_TEST(test_disco_respects_capacity);
    RUN_TEST(test_incoming_sats);
    RUN_TEST(test_test_triggers);
    RUN_TEST(test_non_payments_are_ignored);
    RUN_TEST(test_small_show_matches_mac);
    RUN_TEST(test_medium_show_matches_mac);
    RUN_TEST(test_big_show_matches_mac);
    RUN_TEST(test_bands);
    RUN_TEST(test_show_config_validation);
    RUN_TEST(test_build_show_uses_config);
    RUN_TEST(test_parse_ws_url);
    return UNITY_END();
}
