// Payment shows. Mirrors src/jacket/shows.py; tests compare against its exact output (golden_shows.h).
#include <math.h>

#include "jacket_core.h"

namespace jacket {

namespace {
constexpr int kFadeLevels = 12;
constexpr uint16_t kMinLevel = 10;  // the controller ignores brightness below this
constexpr int kSweepHueStep = 15;
constexpr int kSweepHues = 720 / kSweepHueStep;  // two laps around the wheel
constexpr int kStrobeFlashes = 24;
constexpr int kStrobeHueJump = 135;
constexpr int kFinaleFlashes = 6;
constexpr double kSlowToFast = 3.0;

// Appends steps while there is room; `n` tracks the count.
struct Writer {
    Writer(Step* o, size_t c) : out(o), capacity(c) {}
    Step* out;
    size_t capacity;
    size_t n = 0;
    void add(const Packet& p, double seconds) {
        if (n < capacity) out[n++] = {p, static_cast<uint32_t>(lround(seconds * 1000.0))};
    }
};

void fade(Writer& w, uint16_t hue, uint16_t sat, double seconds, bool up) {
    for (int i = 0; i < kFadeLevels; i++) {
        const int k = up ? i : kFadeLevels - 1 - i;
        const uint16_t v = kMinLevel + (kMaxLevel - kMinLevel) * k / (kFadeLevels - 1);
        w.add(color(hue, sat, v), seconds / kFadeLevels);
    }
}

// Delay i of `count` accelerating delays summing to `seconds` (first is kSlowToFast x the last).
double accelerating(int i, int count, double seconds) {
    double sum = 0;
    for (int j = 0; j < count; j++) sum += kSlowToFast - (kSlowToFast - 1) * j / double(count > 1 ? count - 1 : 1);
    return (kSlowToFast - (kSlowToFast - 1) * i / double(count > 1 ? count - 1 : 1)) * seconds / sum;
}
}  // namespace

bool ShowConfig::valid() const {
    const bool bands = minSats <= mediumFrom && mediumFrom < bigFrom;
    const bool lengths = smallMs > 0 && mediumMs > 0 && bigMs > 0 &&
                         smallMs <= kMaxShowMs && mediumMs <= kMaxShowMs && bigMs <= kMaxShowMs;
    return bands && lengths && smallHue < 360;
}

size_t smallPulse(uint16_t hue, uint32_t durationMs, Step* out, size_t capacity) {
    Writer w{out, capacity};
    const double seconds = durationMs / 1000.0;
    const double fadeS = seconds * 0.25, hold = seconds * 0.5;
    w.add(power(true), 0);
    fade(w, hue, kMaxLevel, fadeS, true);
    if (w.n) w.out[w.n - 1].delayMs = static_cast<uint32_t>(lround((fadeS / kFadeLevels + hold) * 1000.0));
    fade(w, hue, kMaxLevel, fadeS, false);
    w.add(power(false), 0);
    return w.n;
}

size_t bigShow(uint32_t durationMs, Step* out, size_t capacity) {
    Writer w{out, capacity};
    const double seconds = durationMs / 1000.0;
    w.add(power(true), 0);

    const double sweep = seconds * 0.3;  // accelerating rainbow sweep
    for (int i = 0; i < kSweepHues; i++) w.add(color(i * kSweepHueStep), accelerating(i, kSweepHues, sweep));

    const double strobe = seconds * 0.4;  // accelerating disco strobe
    for (int i = 0; i < kStrobeFlashes; i++) {
        const double period = accelerating(i, kStrobeFlashes, strobe);
        w.add(color(i * kStrobeHueJump), 0);
        w.add(power(true), period * 0.6);
        w.add(power(false), period * 0.4);
    }

    const double finale = seconds * 0.3;  // white flashes, then fade to off
    const double period = finale * 0.6 / kFinaleFlashes;
    w.add(white(), 0);
    for (int i = 0; i < kFinaleFlashes; i++) {
        w.add(power(true), period * 0.4);
        w.add(power(false), period * 0.6);
    }
    w.add(power(true), 0);
    fade(w, 0, 0, finale * 0.4, false);
    w.add(power(false), 0);
    return w.n;
}

Show pickShow(int64_t sats, const ShowConfig& cfg) {
    const int64_t minimum = cfg.minSats > 0 ? cfg.minSats : 1;
    if (sats < minimum) return Show::None;
    if (sats >= cfg.bigFrom) return Show::Big;
    if (sats >= cfg.mediumFrom) return Show::Medium;
    return Show::Small;
}

size_t buildShow(Show show, const ShowConfig& cfg, Step* out, size_t capacity) {
    switch (show) {
        case Show::Small: return smallPulse(cfg.smallHue, cfg.smallMs, out, capacity);
        case Show::Medium: return discoRainbow(cfg.mediumMs, out, capacity);
        case Show::Big: return bigShow(cfg.bigMs, out, capacity);
        default: return 0;
    }
}

const char* showName(Show show) {
    switch (show) {
        case Show::Small: return "small";
        case Show::Medium: return "medium";
        case Show::Big: return "big";
        default: return "none";
    }
}

}  // namespace jacket
