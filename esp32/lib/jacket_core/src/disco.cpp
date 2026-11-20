#include "jacket_core.h"

namespace jacket {

namespace {
constexpr int kHueJump = 135;  // big jumps around the wheel read as "disco", not a smooth fade
constexpr uint32_t kOnMs = 120;
constexpr uint32_t kOffMs = 50;
constexpr size_t kStepsPerFlash = 3;
}  // namespace

size_t discoRainbow(uint32_t durationMs, Step* out, size_t capacity) {
    if (durationMs == 0 || capacity < kStepsPerFlash) return 0;
    const uint32_t period = kOnMs + kOffMs;
    size_t flashes = (durationMs + period / 2) / period;
    if (flashes == 0) flashes = 1;
    if (flashes * kStepsPerFlash > capacity) flashes = capacity / kStepsPerFlash;
    const uint32_t slot = durationMs / flashes;
    const uint32_t on = slot * kOnMs / period;
    const uint32_t off = slot - on;
    size_t n = 0;
    for (size_t i = 0; i < flashes; i++) {
        out[n++] = {color(static_cast<int>(i) * kHueJump), 0};
        out[n++] = {power(true), on};
        out[n++] = {power(false), off};
    }
    out[n - 1] = {power(true), off};  // finish lit
    return n;
}

}  // namespace jacket
