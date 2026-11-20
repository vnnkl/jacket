#include <ArduinoJson.h>
#include <string.h>

#include "jacket_core.h"

namespace jacket {

namespace {
constexpr const char* kTestTrigger = "jacket-test";
constexpr const char* kIgnoredStatuses[] = {"pending", "failed", "expired", "cancelled"};

bool ignoredStatus(const char* status) {
    if (!status) return false;  // LNbits omits status on settled payments
    for (const char* s : kIgnoredStatuses)
        if (strcasecmp(status, s) == 0) return true;
    return false;
}
}  // namespace

bool isIncomingPayment(const char* message, size_t len) {
    if (!message || len == 0) return false;
    const size_t triggerLen = strlen(kTestTrigger);
    if (len == triggerLen && memcmp(message, kTestTrigger, len) == 0) return true;

    JsonDocument filter;  // only keep the fields we need; bolt11 etc. are large
    filter["payment"]["amount"] = true;
    filter["payment"]["status"] = true;
    JsonDocument doc;
    if (deserializeJson(doc, message, len, DeserializationOption::Filter(filter))) return false;

    JsonVariantConst amount = doc["payment"]["amount"];
    if (!amount.is<long long>() && !amount.is<double>()) return false;
    if (amount.as<double>() <= 0) return false;
    return !ignoredStatus(doc["payment"]["status"].as<const char*>());
}

}  // namespace jacket
