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

// LNbits wallet update with a positive, non-failed payment.amount, or the text "jacket-test".
bool isIncomingPayment(const char* message, size_t len);

struct WsUrl {
    bool secure;
    char host[128];
    uint16_t port;
    char path[256];
};
bool parseWsUrl(const char* url, WsUrl& out);

}  // namespace jacket
