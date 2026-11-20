#include "payment_socket.h"

#include <Arduino.h>
#include <WebSocketsClient.h>

#include "app_state.h"
#include "config.h"
#include "jacket_core.h"
#include "root_ca.h"

namespace {
WebSocketsClient ws;
payment_socket::Callback callback = nullptr;
jacket::WsUrl url;

void onEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            app_state::status().websocket = true;
            Serial.println("[ws] listening for payments");
            break;
        case WStype_DISCONNECTED:
            if (app_state::status().websocket) Serial.println("[ws] disconnected, will reconnect");
            app_state::status().websocket = false;
            break;
        case WStype_TEXT: {
            const int64_t sats = jacket::incomingSats(reinterpret_cast<const char*>(payload), length);
            if (sats > 0) {
                Serial.printf("[ws] payment received: %lld sats\n", static_cast<long long>(sats));
                callback(sats);
            }
            break;
        }
        case WStype_ERROR:
            Serial.println("[ws] error");
            break;
        default:
            break;
    }
}
}  // namespace

namespace payment_socket {

bool begin(const char* rawUrl, Callback onPayment) {
    if (!jacket::parseWsUrl(rawUrl, url)) return false;
    callback = onPayment;
    if (url.secure) {
        ws.beginSslWithCA(url.host, url.port, url.path, kRootCa);
    } else {
        ws.begin(url.host, url.port, url.path);
    }
    ws.onEvent(onEvent);
    ws.setReconnectInterval(kWsReconnectMs);
    ws.enableHeartbeat(kWsPingMs, 5000, 2);
    return true;
}

void loop() { ws.loop(); }

}  // namespace payment_socket
