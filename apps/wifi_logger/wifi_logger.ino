// WLAN quality logger for M5Stack Cardputer Adv
// Joins your home WiFi (station mode), logs signal strength (RSSI) to the
// SD card once a minute with a real timestamp (via NTP), and serves a
// live + historical chart over the same network.
//
// WiFi credentials are entered once on the device itself - using its
// built-in physical keyboard - and saved to flash (NVS), not hardcoded.
// First boot (or after "Forget WiFi"): the screen prompts for SSID and
// password right there, no second device or separate network needed.
//
// Open http://<ip-shown-on-screen>/ from any device on your home WiFi.
//
// Libraries needed: M5Cardputer (Arduino Library Manager, pulls in M5Unified)
// WiFi/WebServer/SD/SPI/time/Preferences are built into the ESP32 Arduino core.

#include "M5Cardputer.h"
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <SPI.h>
#include <time.h>
#include <Preferences.h>
#include <vector>

// Germany: automatic CET/CEST switching
const char* TZ_INFO = "CET-1CEST,M3.5.0,M10.5.0/3";

#define SD_CS   12
#define SD_MOSI 14
#define SD_CLK  40
#define SD_MISO 39

const char* LOG_PATH = "/wifi_log.csv";
const unsigned long LOG_INTERVAL_MS = 60UL * 1000UL; // 1 minute

WebServer server(80);
Preferences wifiPrefs;
bool sdAvailable = false;
unsigned long lastLogMs = 0;
int lastRssi = 0;
int lastBatteryPct = -1;
char lastTimestamp[32] = "-";

// Saved WiFi credentials live in NVS flash (namespace "wifi"), not in code.
bool loadWifiCreds(String &ssid, String &pass) {
  wifiPrefs.begin("wifi", true); // read-only
  ssid = wifiPrefs.getString("ssid", "");
  pass = wifiPrefs.getString("pass", "");
  wifiPrefs.end();
  return ssid.length() > 0;
}

void saveWifiCreds(const String &ssid, const String &pass) {
  wifiPrefs.begin("wifi", false);
  wifiPrefs.putString("ssid", ssid);
  wifiPrefs.putString("pass", pass);
  wifiPrefs.end();
}

void clearWifiCreds() {
  wifiPrefs.begin("wifi", false);
  wifiPrefs.clear();
  wifiPrefs.end();
}

// Prompts for one line of text on-device, using the physical keyboard.
// Enter confirms (only once the line has content, unless allowEmpty),
// backspace edits. When mask is true (password entry) typed characters
// are echoed as '*' so nothing legible shows on screen. maxLen caps input
// length (0 = unlimited) so it can't exceed what WiFi.begin() accepts
// (32 bytes for an SSID, 63 for a WPA passphrase).
String promptKeyboardInput(const char* label, bool mask, bool allowEmpty, size_t maxLen = 0) {
  String value = "";

  auto redraw = [&]() {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(0, 0);
    M5.Display.println(label);
    M5.Display.println();
    M5.Display.print("> ");
    for (size_t i = 0; i < value.length(); i++) {
      M5.Display.print(mask ? '*' : value[i]);
    }
  };
  redraw();

  while (true) {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
      auto status = M5Cardputer.Keyboard.keysState();
      bool changed = false;

      for (char c : status.word) {
        if (maxLen > 0 && value.length() >= maxLen) continue;
        value += c;
        changed = true;
      }
      if (status.del && value.length() > 0) {
        value.remove(value.length() - 1);
        changed = true;
      }
      if (status.enter && (allowEmpty || value.length() > 0)) {
        return value;
      }
      if (changed) redraw();
    }
    delay(10);
  }
}

// Scans for nearby WiFi networks and lets the user pick one with a single
// number key (strongest signal first), or press 0 to type an SSID by hand
// (needed for hidden networks). Returns the chosen/typed SSID.
String selectSSIDFromScan() {
  M5.Display.setTextSize(2);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.println("Scanning WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();

  // Same network often shows up once per access point/mesh node - dedupe
  // by name, keeping the strongest RSSI seen for each.
  std::vector<String> names;
  std::vector<int32_t> rssis;
  for (int i = 0; i < n; i++) {
    String s = WiFi.SSID(i);
    if (s.length() == 0) continue; // hidden network - only reachable via manual entry
    int32_t r = WiFi.RSSI(i);
    int existing = -1;
    for (size_t j = 0; j < names.size(); j++) {
      if (names[j] == s) { existing = (int)j; break; }
    }
    if (existing >= 0) {
      if (r > rssis[existing]) rssis[existing] = r;
    } else {
      names.push_back(s);
      rssis.push_back(r);
    }
  }
  WiFi.scanDelete();

  // Strongest signal first (selection sort - list is tiny).
  for (size_t i = 0; i < names.size(); i++) {
    size_t best = i;
    for (size_t j = i + 1; j < names.size(); j++) {
      if (rssis[j] > rssis[best]) best = j;
    }
    if (best != i) {
      String tn = names[i]; names[i] = names[best]; names[best] = tn;
      int32_t tr = rssis[i]; rssis[i] = rssis[best]; rssis[best] = tr;
    }
  }

  // At a readable text size, the 135px-tall screen only fits ~8 lines -
  // keep the list short enough to leave room for the header and "Manual".
  const int MAX_SHOWN = 6; // single digit keys 1-6, plus 0 for manual entry
  int shown = names.size() < (size_t)MAX_SHOWN ? (int)names.size() : MAX_SHOWN;

  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setCursor(0, 0);
  if (shown == 0) {
    M5.Display.println("No networks found.");
  } else {
    M5.Display.println("Select WiFi:");
    for (int i = 0; i < shown; i++) {
      // ~20 chars fit per line at this text size - "N:" takes 2, leaving
      // 18 for the name.
      String name = names[i];
      if (name.length() > 18) name = name.substring(0, 17) + ".";
      M5.Display.printf("%d:%s\n", i + 1, name.c_str());
    }
  }
  M5.Display.println("0:Manual entry");

  while (true) {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
      auto status = M5Cardputer.Keyboard.keysState();
      for (char c : status.word) {
        if (c == '0') {
          return promptKeyboardInput("WiFi SSID:", false, false, 32);
        }
        if (c >= '1' && c <= '9') {
          int idx = c - '1';
          if (idx < shown) {
            return names[idx];
          }
        }
      }
    }
    delay(10);
  }
}

// Runs the on-device SSID/password entry screens and saves the result.
void promptAndSaveWifiCreds(String &ssid, String &pass) {
  ssid = selectSSIDFromScan();
  pass = promptKeyboardInput("WiFi password:", true, true, 63);
  saveWifiCreds(ssid, pass);
}

bool getTimestamp(char* buf, size_t len) {
  struct tm t;
  if (!getLocalTime(&t, 100)) return false;
  // Before NTP actually syncs, the ESP32 (no battery-backed RTC) can report
  // a clock stuck around the Unix epoch instead of failing outright; skip
  // logging rather than write an obviously-bogus timestamp.
  if (t.tm_year + 1900 < 2020) return false;
  strftime(buf, len, "%Y-%m-%d %H:%M:%S", &t);
  return true;
}

void logReading() {
  if (!sdAvailable || WiFi.status() != WL_CONNECTED) return;

  char ts[32];
  if (!getTimestamp(ts, sizeof(ts))) return;

  lastRssi = WiFi.RSSI();
  strncpy(lastTimestamp, ts, sizeof(lastTimestamp));

  File f = SD.open(LOG_PATH, FILE_APPEND);
  if (!f) return;
  f.printf("%s,%d\n", ts, lastRssi);
  f.close();
}

#include "page.h"
#include "icon_png.h"

void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/html", INDEX_HTML);
}

void handleForget() {
  clearWifiCreds();
  server.send(200, "text/html",
    "<html><body style='font-family:-apple-system,sans-serif;background:#f2f2f6;color:#1c1c1e;padding:40px'>"
    "<h2>WiFi credentials cleared.</h2><p>Restarting into setup mode...</p></body></html>");
  delay(1500);
  ESP.restart();
}

void handleIcon() {
  server.send_P(200, "image/png", (const char*)ICON_PNG, ICON_PNG_LEN);
}

// Escapes a string for safe embedding inside a JSON string literal.
// Only SSID needs this (timestamps/IPs are generated in known-safe formats),
// but it's applied generically since it's cheap and SSIDs are free-form
// user/router-chosen text that may contain quotes or backslashes.
void appendJsonEscaped(String &out, const String &in) {
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == '"' || c == '\\') out += '\\';
    if ((uint8_t)c < 0x20) continue; // drop control chars rather than escape
    out += c;
  }
}

void handleData() {
  lastBatteryPct = M5.Power.getBatteryLevel();

  String ssidEscaped;
  appendJsonEscaped(ssidEscaped, WiFi.SSID());

  String json = "{\"rssi\":";
  json += lastRssi;
  json += ",\"batt\":";
  json += lastBatteryPct;
  json += ",\"ts\":\"";
  json += lastTimestamp;
  json += "\",\"ip\":\"";
  json += WiFi.localIP().toString();
  json += "\",\"ssid\":\"";
  json += ssidEscaped;
  json += "\"}";

  server.send(200, "application/json", json);
}

void handleHistory() {
  if (!sdAvailable) {
    server.send(200, "text/csv", "");
    return;
  }
  File f = SD.open(LOG_PATH, FILE_READ);
  if (!f) {
    server.send(200, "text/csv", "");
    return;
  }
  server.streamFile(f, "text/csv");
  f.close();
}

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true); // true = enable keyboard

  M5.Display.setRotation(1);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.println("Mounting SD...");

  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  sdAvailable = SD.begin(SD_CS, SPI);
  if (!sdAvailable) {
    M5.Display.println("SD mount FAILED");
    M5.Display.println("Continuing without logging.");
    delay(2000);
  } else if (!SD.exists(LOG_PATH)) {
    File f = SD.open(LOG_PATH, FILE_WRITE);
    if (f) {
      f.println("timestamp,rssi");
      f.close();
    }
  }

  String ssid, pass;
  bool haveCreds = loadWifiCreds(ssid, pass);

  while (true) {
    if (!haveCreds) {
      promptAndSaveWifiCreds(ssid, pass);
    }

    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(0, 0);
    M5.Display.println("Connecting WiFi...");
    M5.Display.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid.c_str(), pass.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
      delay(300);
    }

    if (WiFi.status() == WL_CONNECTED) {
      break;
    }

    // Could be a wrong password, or just the router being briefly down/out
    // of range during boot - don't wipe otherwise-good saved credentials
    // for a transient failure. ENTER retries the same credentials; typing
    // R first re-enters them (e.g. after an actual password change).
    String choice = promptKeyboardInput(
      "Connect failed.\nCheck pass/signal.\n\nENTER: retry\nR+ENTER: re-enter WiFi",
      false, true);
    if (choice.length() > 0 && (choice[0] == 'r' || choice[0] == 'R')) {
      clearWifiCreds();
      haveCreds = false;
    }
  }

  configTzTime(TZ_INFO, "pool.ntp.org", "time.nist.gov");
  struct tm t;
  getLocalTime(&t, 10000); // wait up to 10s for NTP sync

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/history", handleHistory);
  server.on("/icon.png", handleIcon);
  server.on("/forget", HTTP_POST, handleForget);
  server.begin();

  logReading(); // first reading immediately
  lastBatteryPct = M5.Power.getBatteryLevel();
  lastLogMs = millis();

  M5.Display.setTextDatum(middle_left);
  drawStatus();
}

// Same green/yellow/red cutoffs used on the web UI.
uint16_t colorForRssi(int rssi) {
  if (rssi >= -60) return M5.Display.color565(46, 204, 64);   // green
  if (rssi >= -75) return M5.Display.color565(255, 220, 0);   // yellow
  return M5.Display.color565(255, 65, 54);                    // red
}

uint16_t colorForBattery(int pct) {
  if (pct >= 50) return M5.Display.color565(46, 204, 64);   // green
  if (pct >= 20) return M5.Display.color565(255, 220, 0);   // yellow
  return M5.Display.color565(255, 65, 54);                  // red
}

// One single font everywhere (M5GFX-bundled DejaVu Sans) - only the scale
// changes between elements, so every glyph shares the exact same shape.
#define FONT (&fonts::DejaVu56)

const float NUM_SCALE = 1.0f;
const float UNIT_SCALE = 0.35f;
const float BATT_SCALE = UNIT_SCALE;

// Draws "<numStr><unitStr>" (e.g. "-77dBm") centered on cx, unit smaller,
// in light gray.
void drawValue(int cx, int cy, const char* numStr, const char* unitStr, uint16_t color) {
  const int GAP = 6;

  M5.Display.setFont(FONT);

  M5.Display.setTextSize(NUM_SCALE);
  int numW = M5.Display.textWidth(numStr);
  M5.Display.setTextSize(UNIT_SCALE);
  int unitW = M5.Display.textWidth(unitStr);

  int startX = cx - (numW + GAP + unitW) / 2;

  M5.Display.setTextColor(color, TFT_BLACK);
  M5.Display.setTextSize(NUM_SCALE);
  M5.Display.drawString(numStr, startX, cy);

  M5.Display.setTextColor(M5.Display.color565(150, 150, 150), TFT_BLACK);
  M5.Display.setTextSize(UNIT_SCALE);
  M5.Display.drawString(unitStr, startX + numW + GAP, cy);
}

// Only the top row position jumps between updates (screen wear protection);
// the battery line always sits a fixed, tight offset below the RSSI line.
const int SLOT_Y[] = {40, 50};
const int NUM_SLOTS = 2;
const int BATTERY_GAP = 38;
int statusSlot = 0;

void drawStatus() {
  M5.Display.fillScreen(TFT_BLACK);

  int cx = M5.Display.width() / 2;
  int cy1 = SLOT_Y[statusSlot];
  int cy2 = cy1 + BATTERY_GAP;
  statusSlot = (statusSlot + 1) % NUM_SLOTS;

  char rssiBuf[8];
  snprintf(rssiBuf, sizeof(rssiBuf), "%d", lastRssi);
  drawValue(cx, cy1, rssiBuf, "dBm", colorForRssi(lastRssi));

  char battBuf[24];
  if (lastBatteryPct < 0) {
    snprintf(battBuf, sizeof(battBuf), "BAT --%%");
  } else {
    snprintf(battBuf, sizeof(battBuf), "BAT %d %%", lastBatteryPct);
  }
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(FONT);
  M5.Display.setTextSize(BATT_SCALE);
  M5.Display.setTextColor(colorForBattery(lastBatteryPct), TFT_BLACK);
  M5.Display.drawString(battBuf, cx, cy2);

  M5.Display.setTextSize(NUM_SCALE);
  M5.Display.setTextDatum(middle_left);
}

void loop() {
  M5Cardputer.update();
  server.handleClient();

  if (millis() - lastLogMs >= LOG_INTERVAL_MS) {
    logReading();
    lastBatteryPct = M5.Power.getBatteryLevel();
    lastLogMs = millis();
    drawStatus();
  }
}
