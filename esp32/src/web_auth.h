// HTTP Basic auth against the stored admin password, with a lockout after repeated failures.
#pragma once
#include <WebServer.h>

namespace web_auth {
enum class Result { Ok, Missing, Wrong, Locked };
Result check(WebServer& server);
}  // namespace web_auth
