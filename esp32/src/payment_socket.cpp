#include "payment_socket.h"

#include <Arduino.h>
#include <WebSocketsClient.h>

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
            Serial.println("[ws] listening for payments");
            break;
        case WStype_DISCONNECTED:
            Serial.println("[ws] disconnected, will reconnect");
            break;
        case WStype_TEXT:
            if (jacket::isIncomingPayment(reinterpret_cast<const char*>(payload), length)) {
                Serial.println("[ws] payment received");
                callback();
            }
            break;
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
