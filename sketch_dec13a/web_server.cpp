#include "web_server.h"

// --- Defaults ---
#define CFG_FILE "/server.cfg"
#define DEFAULT_AP_SSID     "M5Stick_AP"
#define DEFAULT_AP_PASS     "password123"
#define DEFAULT_STA_SSID    ""
#define DEFAULT_STA_PASS    ""
#define DEFAULT_MODE        0   // 0 = AP, 1 = STA

WebServer server(80);

// --- Runtime config ---
String cfg_ap_ssid   = DEFAULT_AP_SSID;
String cfg_ap_pass   = DEFAULT_AP_PASS;
String cfg_sta_ssid  = DEFAULT_STA_SSID;
String cfg_sta_pass  = DEFAULT_STA_PASS;
bool   cfg_mode      = DEFAULT_MODE;   // current active mode

// ─────────────────────────────────────────
//  Config persistence
// ─────────────────────────────────────────
void loadConfig() {
    File f = LittleFS.open(CFG_FILE, FILE_READ);
    if (!f) return;

    // Format: one "key=value\n" per line
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        int sep = line.indexOf('=');
        if (sep < 0) continue;
        String key = line.substring(0, sep);
        String val = line.substring(sep + 1);

        if (key == "ap_ssid")  cfg_ap_ssid  = val;
        else if (key == "ap_pass")  cfg_ap_pass  = val;
        else if (key == "sta_ssid") cfg_sta_ssid = val;
        else if (key == "sta_pass") cfg_sta_pass = val;
        else if (key == "mode")     cfg_mode     = val.toInt();
    }
    f.close();
    Serial.println("Config loaded.");
}

void saveConfig() {
    File f = LittleFS.open(CFG_FILE, FILE_WRITE);
    if (!f) { Serial.println("Failed to write config"); return; }
    f.printf("ap_ssid=%s\n",  cfg_ap_ssid.c_str());
    f.printf("ap_pass=%s\n",  cfg_ap_pass.c_str());
    f.printf("sta_ssid=%s\n", cfg_sta_ssid.c_str());
    f.printf("sta_pass=%s\n", cfg_sta_pass.c_str());
    f.printf("mode=%d\n",     cfg_mode);
    f.close();
    Serial.println("Config saved.");
}

// ─────────────────────────────────────────
//  Network startup
// ─────────────────────────────────────────
void startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(cfg_ap_ssid.c_str(), cfg_ap_pass.c_str());
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    displayText("Mode: AP\n" + cfg_ap_ssid + "\nIP: " + WiFi.softAPIP().toString());
}

bool startSTA() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg_sta_ssid.c_str(), cfg_sta_pass.c_str());

    Serial.print("Connecting to ");
    Serial.print(cfg_sta_ssid);
    for (int i = 0; i < 20; i++) {   // 10-second timeout
        if (WiFi.status() == WL_CONNECTED) break;
        delay(500);
        Serial.print('.');
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nSTA connect failed, falling back to AP");
        return false;
    }
    Serial.print("\nSTA IP: ");
    Serial.println(WiFi.localIP());

    // Sync time via NTP
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.print("Syncing NTP");
    struct tm ti;
    for (int i = 0; i < 20; i++) {
        delay(500);
        if (getLocalTime(&ti)) { Serial.println(" OK"); break; }
        Serial.print('.');
    }

    displayText("Mode: STA\n" + cfg_sta_ssid + "\nIP: " + WiFi.localIP().toString());
    return true;
}

// ─────────────────────────────────────────
//  Helper: current date string YYYY-MM-DD
// ─────────────────────────────────────────
String currentDateString() {
    struct tm ti;
    if (getLocalTime(&ti)) {
        char buf[16];
        strftime(buf, sizeof(buf), "%Y-%m-%d", &ti);
        return String(buf);
    }
    return "nodate";
}

// ─────────────────────────────────────────
//  File save
// ─────────────────────────────────────────
void saveTextToFile(const String &filename, const String &text) {
    String path = filename.startsWith("/") ? filename : "/" + filename;
    File file = LittleFS.open(path, FILE_WRITE);
    if (!file) { Serial.println("Failed to open file for writing"); return; }
    file.print(text);
    file.close();
    Serial.printf("Saved: %s\n", path.c_str());
}

// ─────────────────────────────────────────
//  HTML page
// ─────────────────────────────────────────
void handleRoot() {
    String dateStr = currentDateString();

    String html = R"=====(<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>M5Stick</title>
<style>
  body{font-family:Arial,sans-serif;margin:0;background:#f0f2f5}
  .wrap{max-width:540px;margin:30px auto;display:flex;flex-direction:column;gap:16px}
  .card{background:#fff;border-radius:10px;box-shadow:0 2px 8px rgba(0,0,0,.1);padding:20px}
  h1{margin:0 0 4px;font-size:22px}
  h2{margin:0 0 12px;font-size:16px;color:#555;border-bottom:1px solid #eee;padding-bottom:8px}
  label{display:block;margin-bottom:4px;font-size:13px;font-weight:bold;color:#333}
  input,textarea{width:100%;padding:9px 10px;box-sizing:border-box;border:1px solid #ccc;
    border-radius:6px;font-size:14px;margin-bottom:10px}
  textarea{height:110px;resize:vertical}
  .row{display:grid;grid-template-columns:1fr 1fr;gap:10px}
  button{width:100%;padding:11px;font-size:15px;cursor:pointer;border:none;border-radius:6px;color:#fff}
  .btn-save{background:#28a745}.btn-save:hover{background:#218838}
  .btn-cfg{background:#0d6efd}.btn-cfg:hover{background:#0b5ed7}
  .status{font-size:13px;color:#666;margin-top:4px;min-height:18px}
  .mode-badge{display:inline-block;padding:2px 10px;border-radius:12px;font-size:12px;
    font-weight:bold;color:#fff;background:#6c757d}
  .mode-ap{background:#fd7e14}.mode-sta{background:#0d6efd}
</style>
</head>
<body>
<div class="wrap">
  <div class="card">
    <h1>M5Stick &nbsp;
      <span class="mode-badge )=====";

    html += (cfg_mode == 0 ? "mode-ap\">AP" : "mode-sta\">STA");
    html += R"=====("</span></h1>
    <p style="margin:0;font-size:13px;color:#888">Текущее время устройства: <span id="devtime">—</span></p>
  </div>

  <!-- ── Save file ── -->
  <div class="card">
    <h2>Сохранить файл</h2>
    <label>Имя файла</label>
    <input type="text" id="fn" placeholder="YYYY-MM-DD" value=")=====";
    html += dateStr;
    html += R"=====(">
    <label>Текст</label>
    <textarea id="t" placeholder="Введите текст…"></textarea>
    <button class="btn-save" onclick="saveFile()">Сохранить на M5Stick</button>
    <div class="status" id="save-status"></div>
  </div>

  <!-- ── AP settings ── -->
  <div class="card">
    <h2>Точка доступа (AP)</h2>
    <div class="row">
      <div>
        <label>Имя сети (SSID)</label>
        <input type="text" id="ap_ssid" value=")=====";
    html += cfg_ap_ssid;
    html += R"=====(">
      </div>
      <div>
        <label>Пароль</label>
        <input type="password" id="ap_pass" value=")=====";
    html += cfg_ap_pass;
    html += R"=====(">
      </div>
    </div>
  </div>

  <!-- ── STA settings ── -->
  <div class="card">
    <h2>Клиент Wi-Fi (STA)</h2>
    <div class="row">
      <div>
        <label>SSID сети</label>
        <input type="text" id="sta_ssid" value=")=====";
    html += cfg_sta_ssid;
    html += R"=====(">
      </div>
      <div>
        <label>Пароль</label>
        <input type="password" id="sta_pass" value=")=====";
    html += cfg_sta_pass;
    html += R"=====(">
      </div>
    </div>
    <button class="btn-cfg" onclick="saveConfig()">Сохранить конфигурацию</button>
    <div class="status" id="cfg-status"></div>
  </div>
</div>

<script>
  // ── Auto-sync browser time to device ──
  function syncTime(){
    const now=new Date();
    const pad=n=>String(n).padStart(2,'0');
    const ts=`${now.getFullYear()}-${pad(now.getMonth()+1)}-${pad(now.getDate())} `+
             `${pad(now.getHours())}:${pad(now.getMinutes())}:${pad(now.getSeconds())}`;
    document.getElementById('devtime').textContent=ts;

    fetch('/setTime?ts='+encodeURIComponent(ts)).catch(()=>{});

    // Also keep filename up to date with today's date
    const datePart=`${now.getFullYear()}-${pad(now.getMonth()+1)}-${pad(now.getDate())}`;
    const fn=document.getElementById('fn');
    // Only auto-update if user hasn't manually changed it
    if(!fn.dataset.edited) fn.value=datePart;
  }
  document.getElementById('fn').addEventListener('input',function(){this.dataset.edited='1'});
  syncTime();
  setInterval(syncTime,10000);

  // ── Save file ──
  function saveFile(){
    const p=new URLSearchParams();
    p.append('filename',document.getElementById('fn').value);
    p.append('text',document.getElementById('t').value);
    document.getElementById('save-status').textContent='Сохранение…';
    fetch('/setText',{method:'POST',body:p})
      .then(r=>r.text())
      .then(m=>{document.getElementById('save-status').textContent=m})
      .catch(e=>{document.getElementById('save-status').textContent='Ошибка: '+e});
  }

  // ── Save config ──
  function saveConfig(){
    const p=new URLSearchParams();
    p.append('ap_ssid', document.getElementById('ap_ssid').value);
    p.append('ap_pass', document.getElementById('ap_pass').value);
    p.append('sta_ssid',document.getElementById('sta_ssid').value);
    p.append('sta_pass',document.getElementById('sta_pass').value);
    document.getElementById('cfg-status').textContent='Сохранение…';
    fetch('/saveConfig',{method:'POST',body:p})
      .then(r=>r.text())
      .then(m=>{document.getElementById('cfg-status').textContent=m})
      .catch(e=>{document.getElementById('cfg-status').textContent='Ошибка: '+e});
  }
</script>
</body>
</html>)=====";

    server.send(200, "text/html; charset=utf-8", html);
}

// ─────────────────────────────────────────
//  Handlers
// ─────────────────────────────────────────
void handleSetText() {
    if (server.method() != HTTP_POST) { server.send(405, "text/plain", "Method Not Allowed"); return; }

    String filename = server.arg("filename");
    String text     = server.arg("text");

    if (filename.length() == 0) filename = currentDateString();
    if (!filename.endsWith(".txt") && !filename.endsWith(".script") && !filename.endsWith(".html"))
        filename += ".txt";

    saveTextToFile(filename, text);
    displayText("File: " + filename + "\nSaved!");
    server.send(200, "text/plain", "Saved as: " + filename);
}

void handleSetTime() {
    // Browser sends ?ts=YYYY-MM-DD HH:MM:SS
    if (!server.hasArg("ts")) { server.send(400, "text/plain", "Missing ts"); return; }
    String ts = server.arg("ts");  // "2024-06-15 14:32:00"

    struct tm ti = {};
    // Parse manually (no sscanf in some builds)
    ti.tm_year = ts.substring(0,4).toInt() - 1900;
    ti.tm_mon  = ts.substring(5,7).toInt() - 1;
    ti.tm_mday = ts.substring(8,10).toInt();
    ti.tm_hour = ts.substring(11,13).toInt();
    ti.tm_min  = ts.substring(14,16).toInt();
    ti.tm_sec  = ts.substring(17,19).toInt();

    time_t t = mktime(&ti);
    if (t != -1) {
        struct timeval tv = { t, 0 };
        settimeofday(&tv, nullptr);
    }
    server.send(200, "text/plain", "OK");
}

void handleSaveConfig() {
    if (server.method() != HTTP_POST) { server.send(405, "text/plain", "Method Not Allowed"); return; }

    if (server.hasArg("ap_ssid") && server.arg("ap_ssid").length() > 0)
        cfg_ap_ssid = server.arg("ap_ssid");
    if (server.hasArg("ap_pass") && server.arg("ap_pass").length() >= 8)
        cfg_ap_pass = server.arg("ap_pass");
    if (server.hasArg("sta_ssid"))
        cfg_sta_ssid = server.arg("sta_ssid");
    if (server.hasArg("sta_pass"))
        cfg_sta_pass = server.arg("sta_pass");

    saveConfig();
    displayText("Config saved!\nRestart to\napply network");
    server.send(200, "text/plain", "Config saved. Press BtnA to switch mode or restart.");
}

// ─────────────────────────────────────────
//  Init
// ─────────────────────────────────────────
void initWebServer() {
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
    }

    loadConfig();

    bool staOk = false;
    if (cfg_mode == 1) {
        staOk = startSTA();
    }
    if (!staOk) {
        cfg_mode = 0;
        startAP();
    }

    server.on("/",           handleRoot);
    server.on("/setText",    handleSetText);
    server.on("/setTime",    handleSetTime);
    server.on("/saveConfig", handleSaveConfig);
    server.onNotFound([]() { server.send(404, "text/plain", "Not Found"); });

    server.begin();
    Serial.println("Web server started!");
}

// ─────────────────────────────────────────
//  Loop
// ─────────────────────────────────────────
void loopWebServer() {
    server.handleClient();

    // BtnA: toggle AP ↔ STA and restart
    if (StickCP2.BtnA.wasPressed()) {
        cfg_mode = (cfg_mode == 0) ? 1 : 0;
        saveConfig();
        displayText("Mode -> " + String(cfg_mode == 0 ? "AP" : "STA") + "\nRestarting…");
        delay(1500);
        ESP.restart();
    }

    // BtnB: just restart
    if (StickCP2.BtnB.wasPressed()) {
        ESP.restart();
    }
}