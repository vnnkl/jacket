#include "web_auth.h"

#include <mbedtls/base64.h>
#include <string.h>

#include "config.h"
#include "settings_rules.h"
#include "settings_store.h"

namespace {
jacket::LoginThrottle throttle;
constexpr size_t kMaxCredentials = 160;
}  // namespace

namespace web_auth {

Result check(WebServer& server) {
    const String header = server.header("Authorization");
    if (!header.startsWith("Basic ")) return Result::Missing;
    if (!throttle.allowed(millis())) return Result::Locked;

    unsigned char decoded[kMaxCredentials + 1];
    size_t len = 0;
    const String b64 = header.substring(6);
    if (mbedtls_base64_decode(decoded, kMaxCredentials, &len, reinterpret_cast<const unsigned char*>(b64.c_str()),
                              b64.length()) != 0) {
        throttle.failed(millis());
        return Result::Wrong;
    }
    decoded[len] = '\0';
    char* colon = strchr(reinterpret_cast<char*>(decoded), ':');
    if (!colon) {
        throttle.failed(millis());
        return Result::Wrong;
    }
    *colon = '\0';
    const bool ok = strcmp(reinterpret_cast<char*>(decoded), kAdminUser) == 0 &&
                    settings_store::checkAdminPassword(colon + 1);
    memset(decoded, 0, sizeof decoded);
    if (!ok) {
        throttle.failed(millis());
        return Result::Wrong;
    }
    throttle.succeeded();
    return Result::Ok;
}

}  // namespace web_auth
