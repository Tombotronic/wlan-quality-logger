// Web UI demo for M5Stack Cardputer Adv
// The Cardputer creates its own WiFi hotspot and serves a page showing
// live accelerometer/gyro readings from the onboard BMI270.
//
// Connect your phone/laptop to the WiFi network shown on screen,
// then open the IP address shown (usually http://192.168.4.1) in a browser.
//
// Libraries needed: M5Unified (Arduino Library Manager)
// WiFi/WebServer are built into the ESP32 Arduino core, no install needed.

#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>

const char* AP_SSID = "CardputerAdv";
const char* AP_PASSWORD = "cardputer123"; // must be 8+ chars for WPA2

WebServer server(80);

// Page shell: loads once, then polls /data every 200ms via JS fetch()
// so values update live without reloading the whole page.
const char* INDEX_HTML = R"HTML(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Cardputer Adv - IMU</title>
  <style>
    body { font-family: monospace; background: #111; color: #eee; padding: 20px; }
    h1 { font-size: 1.2em; }
    table { border-collapse: collapse; width: 100%; max-width: 400px; }
    td { padding: 6px 10px; border-bottom: 1px solid #333; }
    .label { color: #888; }
    .val { text-align: right; font-weight: bold; }
  </style>
</head>
<body>
  <h1>Cardputer Adv - Live IMU</h1>
  <table>
    <tr><td class="label">Accel X</td><td class="val" id="ax">-</td></tr>
    <tr><td class="label">Accel Y</td><td class="val" id="ay">-</td></tr>
    <tr><td class="label">Accel Z</td><td class="val" id="az">-</td></tr>
    <tr><td class="label">Gyro X</td><td class="val" id="gx">-</td></tr>
    <tr><td class="label">Gyro Y</td><td class="val" id="gy">-</td></tr>
    <tr><td class="label">Gyro Z</td><td class="val" id="gz">-</td></tr>
  </table>
  <script>
    async function poll() {
      try {
        const r = await fetch('/data');
        const d = await r.json();
        for (const k in d) document.getElementById(k).textContent = d[k].toFixed(3);
      } catch (e) {}
      setTimeout(poll, 200);
    }
    poll();
  </script>
</body>
</html>
)HTML";

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleData() {
  M5.Imu.update();
  auto data = M5.Imu.getImuData();
  char json[200];
  snprintf(json, sizeof(json),
    "{\"ax\":%.3f,\"ay\":%.3f,\"az\":%.3f,\"gx\":%.3f,\"gy\":%.3f,\"gz\":%.3f}",
    data.accel.x, data.accel.y, data.accel.z,
    data.gyro.x, data.gyro.y, data.gyro.z);
  server.send(200, "application/json", json);
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Display.setRotation(1);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.println("Starting WiFi AP...");

  if (M5.Imu.getType() == m5::imu_none) {
    M5.Display.println("No IMU found!");
    while (true) { delay(1000); }
  }

  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress ip = WiFi.softAPIP();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.println("WiFi AP ready:");
  M5.Display.print("SSID: ");
  M5.Display.println(AP_SSID);
  M5.Display.print("Pass: ");
  M5.Display.println(AP_PASSWORD);
  M5.Display.print("URL:  http://");
  M5.Display.println(ip);
}

void loop() {
  M5.update();
  server.handleClient();
}
