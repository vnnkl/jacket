#include <stdlib.h>
#include <string.h>

#include "jacket_core.h"

namespace jacket {

namespace {
bool copyBounded(char* dst, size_t cap, const char* src, size_t len) {
    if (len == 0 || len >= cap) return false;
    memcpy(dst, src, len);
    dst[len] = '\0';
    return true;
}
}  // namespace

bool parseWsUrl(const char* url, WsUrl& out) {
    if (!url) return false;
    const char* rest;
    if (strncmp(url, "wss://", 6) == 0) {
        out.secure = true;
        out.port = 443;
        rest = url + 6;
    } else if (strncmp(url, "ws://", 5) == 0) {
        out.secure = false;
        out.port = 80;
        rest = url + 5;
    } else {
        return false;
    }
    const char* slash = strchr(rest, '/');
    const char* hostEnd = slash ? slash : rest + strlen(rest);
    const char* colon = static_cast<const char*>(memchr(rest, ':', hostEnd - rest));
    if (!copyBounded(out.host, sizeof out.host, rest, (colon ? colon : hostEnd) - rest)) return false;
    if (colon) {
        char* end = nullptr;
        long port = strtol(colon + 1, &end, 10);
        if (end != hostEnd || port <= 0 || port > 65535) return false;
        out.port = static_cast<uint16_t>(port);
    }
    return slash ? copyBounded(out.path, sizeof out.path, slash, strlen(slash))
                 : copyBounded(out.path, sizeof out.path, "/", 1);
}

}  // namespace jacket
