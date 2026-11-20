#include <ArduinoJson.h>
#include <string.h>

#include "jacket_core.h"

namespace jacket {

namespace {
constexpr const char* kTestTrigger = "jacket-test";
constexpr const char* kIgnoredStatuses[] = {"pending", "failed", "expired", "cancelled"};
constexpr double kMsatPerSat = 1000;

bool ignoredStatus(const char* status) {
    if (!status) return false;  // LNbits omits status on settled payments
    for (const char* s : kIgnoredStatuses)
        if (strcasecmp(status, s) == 0) return true;
    return false;
}

int64_t testAmount(const char* message, size_t len) {
    const size_t triggerLen = strlen(kTestTrigger);
    if (len == triggerLen) return 5000;
    if (message[triggerLen] != ':') return 0;
    const char* arg = message + triggerLen + 1;
    const size_t argLen = len - triggerLen - 1;
    struct Band { const char* name; int64_t sats; };
    static const Band kBands[] = {{"small", 100}, {"medium", 5000}, {"big", 21000}};
    for (const Band& b : kBands)
        if (argLen == strlen(b.name) && memcmp(arg, b.name, argLen) == 0) return b.sats;
    int64_t n = 0;
    for (size_t i = 0; i < argLen; i++) {
        if (arg[i] < '0' || arg[i] > '9' || n > 1000000000000LL) return 0;
        n = n * 10 + (arg[i] - '0');
    }
    return n;
}

}  // namespace

int64_t incomingSats(const char* message, size_t len) {
    if (!message || len == 0) return 0;
    const size_t triggerLen = strlen(kTestTrigger);
    if (len >= triggerLen && memcmp(message, kTestTrigger, triggerLen) == 0) return testAmount(message, len);

    JsonDocument filter;  // only keep the fields we need; bolt11 etc. are large
    filter["payment"]["amount"] = true;
    filter["payment"]["status"] = true;
    JsonDocument doc;
    if (deserializeJson(doc, message, len, DeserializationOption::Filter(filter))) return 0;

    JsonVariantConst amount = doc["payment"]["amount"];
    if (!amount.is<long long>() && !amount.is<double>()) return 0;
    const double msat = amount.as<double>();
    if (msat <= 0 || ignoredStatus(doc["payment"]["status"].as<const char*>())) return 0;
    const int64_t sats = static_cast<int64_t>((msat + kMsatPerSat - 1) / kMsatPerSat);
    return sats < 1 ? 1 : sats;
}

}  // namespace jacket
