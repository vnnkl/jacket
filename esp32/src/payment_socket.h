// Keeps the payment websocket open and calls onPayment() for each incoming payment.
#pragma once

namespace payment_socket {
using Callback = void (*)();
bool begin(const char* url, Callback onPayment);  // false if the URL is invalid
void loop();
}  // namespace payment_socket
