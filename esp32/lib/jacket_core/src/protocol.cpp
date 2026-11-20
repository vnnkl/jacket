#include "jacket_core.h"

#include <string.h>

namespace jacket {

Packet frame(uint8_t kind, const uint8_t* payload, size_t len) {
    Packet p{};
    if (len + 4 > kMaxPacket) return p;  // len 0 signals an invalid packet
    p.data[0] = 0xAA;
    p.data[1] = kind;
    p.data[2] = static_cast<uint8_t>(len);
    memcpy(p.data + 3, payload, len);
    uint8_t sum = 0;
    for (size_t i = 0; i < len + 3; i++) sum += p.data[i];
    p.data[len + 3] = sum;
    p.len = len + 4;
    return p;
}

Packet power(bool on) {
    const uint8_t payload[] = {static_cast<uint8_t>(on ? 1 : 0)};
    return frame(0x02, payload, sizeof payload);
}

Packet color(int hue, uint16_t saturation, uint16_t brightness) {
    const uint16_t h = static_cast<uint16_t>(((hue % 360) + 360) % 360);
    const uint16_t s = saturation > kMaxLevel ? kMaxLevel : saturation;
    const uint16_t v = brightness > kMaxLevel ? kMaxLevel : brightness;
    const uint8_t payload[] = {1, uint8_t(h >> 8), uint8_t(h), uint8_t(s >> 8), uint8_t(s), uint8_t(v >> 8), uint8_t(v)};
    return frame(0x03, payload, sizeof payload);
}

Packet white() { return color(0, 0, kMaxLevel); }

}  // namespace jacket
