// Copy to secrets.h (flash.sh writes it for you). secrets.h is git-ignored.
#pragma once
#define WIFI_SSID "your-wifi-name"
#define WIFI_PASSWORD "your-wifi-password"
#define PAYMENT_WS_URL "wss://bbits.io/api/v1/ws/<id>"
// Optional: Bluetooth MAC of the jacket controller, e.g. "aa:bb:cc:dd:ee:ff". Empty = scan for "Hello Fairy".
#define JACKET_ADDRESS ""
