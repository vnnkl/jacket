#include "web_admin.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>

#include "admin_page.h"
#include "app_state.h"
#include "config.h"
#include "settings_store.h"
#include "web_auth.h"
#include "wifi_setup.h"

namespace {
WebServer server(80);
bool restartPending = false;
uint32_t restartAtMs = 0;
bool uploadAuthorized = false;
String uploadError;
constexpr size_t kMinAdminPassword = 8;
constexpr size_t kMaxAdminPassword = 64;

void sendJson(int code, const JsonDocument& doc) {
    String body;
    serializeJson(doc, body);
    server.sendHeader("Cache-Control", "no-store");
    server.send(code, "application/json", body);
}

void sendError(int code, const char* message) {
    JsonDocument doc;
    doc["error"] = message;
    sendJson(code, doc);
}

void scheduleRestart() {
    restartPending = true;
    restartAtMs = millis() + kRestartDelayMs;
}

// No WWW-Authenticate header: the page shows its own login, so browsers never cache credentials.
bool authorized() {
    switch (web_auth::check(server)) {
        case web_auth::Result::Ok: return true;
        case web_auth::Result::Locked: sendError(429, "Too many wrong passwords. Wait 30 seconds."); return false;
        case web_auth::Result::Wrong: sendError(401, "Wrong password."); return false;
        default: sendError(401, "Log in first."); return false;
    }
}

bool validAdminPassword(const char* p) {
    const size_t n = p ? strlen(p) : 0;
    return n >= kMinAdminPassword && n <= kMaxAdminPassword;
}

void handleState() {
    JsonDocument doc;
    doc["adminSet"] = settings_store::hasAdminPassword();
    doc["onHotspot"] = wifi_setup::onHotspot(server.client().localIP());
    doc["version"] = FIRMWARE_VERSION;
    sendJson(200, doc);
}

// Settings changes that need a fresh connection restart the board after replying.
bool needsRestart(const jacket::Settings& a, const jacket::Settings& b) {
    return strcmp(a.wifiSsid, b.wifiSsid) || strcmp(a.wifiPass, b.wifiPass) || strcmp(a.wsUrl, b.wsUrl) ||
           strcmp(a.jacketAddress, b.jacketAddress) || strcmp(a.hotspotPass, b.hotspotPass);
}

void saveAndReply(const jacket::Settings& before, const jacket::Settings& after) {
    if (!settings_store::save(after)) return sendError(500, "Could not save settings to flash.");
    app_state::setSettings(after);
    JsonDocument doc;
    doc["ok"] = true;
    doc["restarting"] = needsRestart(before, after);
    sendJson(200, doc);
    if (doc["restarting"]) scheduleRestart();
}

void handleSetup() {  // first-time setup: only through the hotspot, only while no admin password exists
    if (settings_store::hasAdminPassword()) return sendError(403, "Already set up. Log in instead.");
    if (!wifi_setup::onHotspot(server.client().localIP()))
        return sendError(403, "First-time setup only works when connected to the Jacket-Setup hotspot.");
    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain"))) return sendError(400, "Expected JSON.");
    const char* admin = doc["adminPassword"] | "";
    if (!validAdminPassword(admin)) return sendError(400, "Admin password must be 8-64 characters.");
    const jacket::Settings before = app_state::settings();
    jacket::Settings next = before;
    char err[128];
    doc.remove("adminPassword");
    String rest;
    serializeJson(doc, rest);
    if (!jacket::applySettingsJson(rest.c_str(), rest.length(), next, err, sizeof err)) return sendError(400, err);
    if (!jacket::isConfigured(next)) return sendError(400, "Wi-Fi name and websocket URL are required.");
    if (!settings_store::setAdminPassword(admin)) return sendError(500, "Could not save the admin password.");
    saveAndReply(before, next);
}

void handleStatus() {
    if (!authorized()) return;
    JsonDocument doc;
    app_state::Status& st = app_state::status();
    doc["wifi"] = st.wifi.load();
    doc["wifiIp"] = WiFi.localIP().toString();
    doc["wifiRssi"] = WiFi.RSSI();
    doc["hotspot"] = st.hotspot.load();
    doc["websocket"] = st.websocket.load();
    doc["jacket"] = st.jacket.load();
    doc["uptimeMs"] = millis();
    doc["version"] = FIRMWARE_VERSION;
    JsonArray list = doc["payments"].to<JsonArray>();
    const jacket::PaymentLog log = app_state::payments();
    for (size_t i = 0; i < log.size(); i++) {
        JsonObject p = list.add<JsonObject>();
        p["sats"] = log.at(i).sats;
        p["show"] = jacket::showName(log.at(i).show);
        p["agoMs"] = millis() - log.at(i).uptimeMs;
    }
    sendJson(200, doc);
}

void handleGetSettings() {
    if (!authorized()) return;
    char out[1024];
    const size_t n = jacket::settingsToJson(app_state::settings(), out, sizeof out);
    server.sendHeader("Cache-Control", "no-store");
    server.send(n ? 200 : 500, "application/json", n ? out : "{\"error\":\"settings too large\"}");
}

void handlePostSettings() {
    if (!authorized()) return;
    const jacket::Settings before = app_state::settings();
    jacket::Settings next = before;
    char err[128];
    const String& body = server.arg("plain");
    if (!jacket::applySettingsJson(body.c_str(), body.length(), next, err, sizeof err)) return sendError(400, err);
    saveAndReply(before, next);
}

void handlePassword() {
    if (!authorized()) return;
    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain"))) return sendError(400, "Expected JSON.");
    const char* pw = doc["password"] | "";
    if (!validAdminPassword(pw)) return sendError(400, "Admin password must be 8-64 characters.");
    if (!settings_store::setAdminPassword(pw)) return sendError(500, "Could not save the password.");
    JsonDocument ok;
    ok["ok"] = true;
    sendJson(200, ok);
}

void handleTest() {
    if (!authorized()) return;
    const jacket::ShowConfig cfg = app_state::settings().shows;
    const String size = server.arg("size");
    const int64_t sats = size == "big" ? cfg.bigFrom : size == "medium" ? cfg.mediumFrom
                         : size == "small" ? (cfg.minSats > 0 ? cfg.minSats : 1) : 0;
    if (!sats) return sendError(400, "size must be small, medium or big");
    if (!app_state::queueShow(sats, true)) return sendError(503, "Show queue is full; try again shortly.");
    JsonDocument doc;
    doc["ok"] = true;
    sendJson(200, doc);
}

void handleReboot() {
    if (!authorized()) return;
    JsonDocument doc;
    doc["ok"] = true;
    doc["restarting"] = true;
    sendJson(200, doc);
    scheduleRestart();
}

void handleFactoryReset() {
    if (!authorized()) return;
    settings_store::factoryReset();
    JsonDocument doc;
    doc["ok"] = true;
    doc["restarting"] = true;
    sendJson(200, doc);
    scheduleRestart();
}

void handleUploadChunk() {
    HTTPUpload& up = server.upload();
    if (up.status == UPLOAD_FILE_START) {
        uploadError = "";
        uploadAuthorized = web_auth::check(server) == web_auth::Result::Ok;
        if (!uploadAuthorized) return void(uploadError = "Wrong or missing admin password.");
        Serial.printf("[ota] receiving %s\n", up.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) uploadError = Update.errorString();
    } else if (up.status == UPLOAD_FILE_WRITE && uploadAuthorized && uploadError.isEmpty()) {
        if (Update.write(up.buf, up.currentSize) != up.currentSize) uploadError = Update.errorString();
    } else if (up.status == UPLOAD_FILE_END && uploadAuthorized && uploadError.isEmpty()) {
        if (!Update.end(true)) uploadError = Update.errorString();
    } else if (up.status == UPLOAD_FILE_ABORTED) {
        Update.abort();
        uploadError = "Upload aborted.";
    }
}

void handleUploadDone() {
    if (!uploadAuthorized) return sendError(401, "Wrong or missing admin password.");
    if (!uploadError.isEmpty()) return sendError(400, ("Update failed: " + uploadError).c_str());
    JsonDocument doc;
    doc["ok"] = true;
    doc["restarting"] = true;
    sendJson(200, doc);
    Serial.println("[ota] update written, restarting");
    scheduleRestart();
}

void handlePage() {
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("Content-Security-Policy", "default-src 'self' 'unsafe-inline'; img-src data:");
    server.send_P(200, "text/html", kAdminPage);
}

void handleNotFound() {  // captive portal: send hotspot visitors to the page
    if (wifi_setup::onHotspot(server.client().localIP())) {
        server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/");
        return server.send(302, "text/plain", "");
    }
    sendError(404, "Not found.");
}
}  // namespace

namespace web_admin {

void begin() {
    server.on("/", HTTP_GET, handlePage);
    server.on("/api/state", HTTP_GET, handleState);
    server.on("/api/setup", HTTP_POST, handleSetup);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/settings", HTTP_GET, handleGetSettings);
    server.on("/api/settings", HTTP_POST, handlePostSettings);
    server.on("/api/password", HTTP_POST, handlePassword);
    server.on("/api/test", HTTP_POST, handleTest);
    server.on("/api/reboot", HTTP_POST, handleReboot);
    server.on("/api/factory-reset", HTTP_POST, handleFactoryReset);
    server.on("/update", HTTP_POST, handleUploadDone, handleUploadChunk);
    server.onNotFound(handleNotFound);
    server.begin();
}

void loop() {
    server.handleClient();
    if (restartPending && static_cast<int32_t>(millis() - restartAtMs) >= 0) ESP.restart();
}

}  // namespace web_admin
