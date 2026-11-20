// Bluetooth LE link to the Hello Fairy jacket controller. Use from one task only.
#pragma once
#include "jacket_core.h"

namespace ble_jacket {
void begin();
bool ensureConnected();  // scans + connects if needed; false if the jacket isn't reachable
bool send(const jacket::Packet& p);
bool play(const jacket::Step* steps, size_t n);
}  // namespace ble_jacket
