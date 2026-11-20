// Settings and the admin password in flash (NVS). Survive firmware updates; wiped by factoryReset().
#pragma once
#include "settings_rules.h"

namespace settings_store {
jacket::Settings load();                 // defaults if nothing (or an older layout) is stored
bool save(const jacket::Settings& s);
bool hasAdminPassword();
bool setAdminPassword(const char* password);
bool checkAdminPassword(const char* password);
void factoryReset();
}  // namespace settings_store
