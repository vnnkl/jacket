#include "app_state.h"

#include <Arduino.h>

#include "config.h"

namespace {
SemaphoreHandle_t lock;
QueueHandle_t shows;
jacket::Settings current;
jacket::PaymentLog paymentLog;
app_state::Status flags;

struct QueuedShow {
    int64_t sats;
    bool test;
};

struct Guard {
    Guard() { xSemaphoreTake(lock, portMAX_DELAY); }
    ~Guard() { xSemaphoreGive(lock); }
};
}  // namespace

namespace app_state {

void begin() {
    lock = xSemaphoreCreateMutex();
    shows = xQueueCreate(kPaymentQueue, sizeof(QueuedShow));
    current = jacket::defaultSettings();
}

jacket::Settings settings() {
    Guard g;
    return current;
}

void setSettings(const jacket::Settings& s) {
    Guard g;
    current = s;
}

Status& status() { return flags; }

void logPayment(int64_t sats, jacket::Show show) {
    Guard g;
    paymentLog.add(sats, show, millis());
}

jacket::PaymentLog payments() {
    Guard g;
    return paymentLog;
}

bool queueShow(int64_t sats, bool test) {
    const QueuedShow item{sats, test};
    return xQueueSend(shows, &item, 0) == pdTRUE;
}

bool nextShow(int64_t& sats, bool& test, uint32_t waitMs) {
    QueuedShow item;
    if (xQueueReceive(shows, &item, pdMS_TO_TICKS(waitMs)) != pdTRUE) return false;
    sats = item.sats;
    test = item.test;
    return true;
}

}  // namespace app_state
