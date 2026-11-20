// Pure logic shared by the firmware and the host unit tests (no Arduino / BLE / Wi-Fi here).
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace jacket {

constexpr size_t kMaxPacket = 16;
constexpr uint16_t kMaxLevel = 1000;  // saturation / brightness scale used by the controller

struct Packet {
    uint8_t data[kMaxPacket];
    size_t len;
};

struct Step {
    Packet packet;
    uint32_t delayMs;  // wait after sending
};

// Hello Fairy frame: AA TYPE LEN payload... SUM (SUM = low byte of the sum of all earlier bytes).
Packet frame(uint8_t kind, const uint8_t* payload, size_t len);
Packet power(bool on);
Packet color(int hue, uint16_t saturation = kMaxLevel, uint16_t brightness = kMaxLevel);
Packet white();

// Strobing jumps around the rainbow for `durationMs`, ending lit. Returns steps written.
constexpr size_t kMaxDiscoSteps = 128;
size_t discoRainbow(uint32_t durationMs, Step* out, size_t capacity);

// Incoming amount in sats (rounded up) from an LNbits wallet update, or 0 if the message is not an
// incoming settled payment. Test triggers: "jacket-test", "jacket-test:small|medium|big|<sats>".
int64_t incomingSats(const char* message, size_t len);

// ---- payment shows (must match the Mac implementation byte for byte; see tools/gen_golden.py) ----
constexpr size_t kMaxShowSteps = 192;
constexpr uint32_t kMaxShowMs = 60000;

enum class Show { None, Small, Medium, Big };

struct ShowConfig {
    uint32_t minSats = 0;
    uint32_t mediumFrom = 1000;
    uint32_t bigFrom = 10000;
    uint16_t smallHue = 60;  // yellow
    uint32_t smallMs = 2000;
    uint32_t mediumMs = 3000;
    uint32_t bigMs = 10000;
    bool valid() const;
};

Show pickShow(int64_t sats, const ShowConfig& cfg);
size_t buildShow(Show show, const ShowConfig& cfg, Step* out, size_t capacity);
size_t smallPulse(uint16_t hue, uint32_t durationMs, Step* out, size_t capacity);
size_t bigShow(uint32_t durationMs, Step* out, size_t capacity);
const char* showName(Show show);

struct WsUrl {
    bool secure;
    char host[128];
    uint16_t port;
    char path[256];
};
bool parseWsUrl(const char* url, WsUrl& out);

}  // namespace jacket
