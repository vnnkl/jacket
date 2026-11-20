// Keeps the payment websocket open and calls onPayment(sats) for each incoming payment.
#pragma once
#include <stdint.h>

namespace payment_socket {
using Callback = void (*)(int64_t sats);
bool begin(const char* url, Callback onPayment);  // false if the URL is invalid
void loop();
}  // namespace payment_socket
