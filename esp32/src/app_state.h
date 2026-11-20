// State shared between the web/Wi-Fi loop and the jacket task. All access goes through here.
#pragma once
#include <atomic>

#include "settings_rules.h"

namespace app_state {

void begin();

jacket::Settings settings();                     // copy of the current settings
void setSettings(const jacket::Settings& s);     // in memory only; persist with settings_store::save

struct Status {
    std::atomic<bool> wifi{false};
    std::atomic<bool> hotspot{false};
    std::atomic<bool> websocket{false};
    std::atomic<bool> jacket{false};
};
Status& status();

void logPayment(int64_t sats, jacket::Show show);
jacket::PaymentLog payments();                   // copy

// Queue a show: real payments are logged when played, tests are not.
bool queueShow(int64_t sats, bool test);
bool nextShow(int64_t& sats, bool& test, uint32_t waitMs);

}  // namespace app_state
