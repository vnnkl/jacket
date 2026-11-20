"""Serve the firmware's admin page with a fake API, to try the page in a browser without a board.
Usage: uv run python tools/mock_admin.py [port] [--fresh]   (--fresh = no admin password, on hotspot)"""
import base64, json, pathlib, re, sys, time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

PAGE = re.search(r'R"HTML\((.*)\)HTML"', (pathlib.Path(__file__).resolve().parent.parent /
                 "esp32/include/admin_page.h").read_text(), re.S).group(1)
STATE = {"admin": None if "--fresh" in sys.argv else "testpass1", "start": time.time(), "tests": [],
         "settings": {"wifiSsid": "Home", "wifiPassSet": True, "wsUrlSet": True, "wsHost": "bbits.io",
                      "jacketAddress": "", "hotspotPassIsDefault": True, "minSats": 0, "mediumFrom": 1000,
                      "bigFrom": 10000, "smallColor": 60, "smallSeconds": 2, "mediumSeconds": 3, "bigSeconds": 10,
                      "idle": "off", "idleColor": 0, "idlePreset": 0}}


class H(BaseHTTPRequestHandler):
    def log_message(self, *a): pass

    def send(self, code, body, ctype="application/json"):
        data = (json.dumps(body) if ctype == "application/json" else body).encode()
        self.send_response(code); self.send_header("Content-Type", ctype); self.end_headers(); self.wfile.write(data)

    def authed(self):
        h = self.headers.get("Authorization", "")
        ok = STATE["admin"] and h == "Basic " + base64.b64encode(f"admin:{STATE['admin']}".encode()).decode()
        if not ok: self.send(401, {"error": "Wrong password." if h else "Log in first."})
        return ok

    def body(self):
        n = int(self.headers.get("Content-Length", 0)); raw = self.rfile.read(n)
        try: return json.loads(raw)
        except ValueError: return {}

    def do_GET(self):
        if self.path == "/": return self.send(200, PAGE, "text/html")
        if self.path == "/api/state":
            return self.send(200, {"adminSet": bool(STATE["admin"]), "onHotspot": True, "version": "mock"})
        if not self.authed(): return
        if self.path == "/api/settings": return self.send(200, STATE["settings"])
        if self.path == "/api/status":
            return self.send(200, {"wifi": True, "wifiIp": "192.168.1.50", "wifiRssi": -58, "hotspot": False,
                                   "websocket": True, "jacket": True, "uptimeMs": int((time.time() - STATE["start"]) * 1000),
                                   "version": "mock", "payments": [{"sats": 72510, "show": "big", "agoMs": 65000},
                                                                   {"sats": 300, "show": "small", "agoMs": 4000000}]})
        self.send(404, {"error": "Not found."})

    def do_POST(self):
        path = self.path.split("?")[0]
        if path == "/api/setup":
            d = self.body()
            if len(d.get("adminPassword", "")) < 8: return self.send(400, {"error": "Admin password must be 8-64 characters."})
            STATE["admin"] = d["adminPassword"]; return self.send(200, {"ok": True, "restarting": True})
        if not self.authed(): return
        if path == "/api/settings":
            d = self.body()
            if d.get("bigFrom", 10000) <= d.get("mediumFrom", 1000): return self.send(400, {"error": "Need minimum <= medium-from < big-from"})
            restart = any(k in d for k in ("wifiPass", "wsUrl", "hotspotPass")) or d.get("wifiSsid", STATE["settings"]["wifiSsid"]) != STATE["settings"]["wifiSsid"]
            STATE["settings"].update({k: v for k, v in d.items() if k in STATE["settings"]})
            return self.send(200, {"ok": True, "restarting": restart})
        if path == "/api/test":
            STATE["tests"].append(self.path.split("size=")[-1]); return self.send(200, {"ok": True})
        if path == "/api/password":
            STATE["admin"] = self.body()["password"]; return self.send(200, {"ok": True})
        if path in ("/api/reboot", "/api/factory-reset", "/update"): return self.send(200, {"ok": True, "restarting": True})
        self.send(404, {"error": "Not found."})


if __name__ == "__main__":
    port = int(next((a for a in sys.argv[1:] if a.isdigit()), 8765))
    print(f"mock admin on http://127.0.0.1:{port}  (admin password: testpass1)", flush=True)
    ThreadingHTTPServer(("127.0.0.1", port), H).serve_forever()
