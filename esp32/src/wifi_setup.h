// Joins the saved Wi-Fi; opens the "Jacket-Setup" hotspot (with a captive page) when there is no
// Wi-Fi yet, when joining fails, or when BOOT is held 3 s. Holding BOOT 10 s wipes all settings.
#pragma once
#include <IPAddress.h>

namespace wifi_setup {
void begin();
void loop();
bool onHotspot(const IPAddress& localIp);  // was this request made through the hotspot?
}  // namespace wifi_setup
