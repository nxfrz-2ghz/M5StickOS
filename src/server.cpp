#include "M5StickCPlus2.h"
#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include "preferences.h"
#include "gui.h"
#include <esp_sntp.h>
#include "server.h"
#include "tvbGone.h"

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
    // Читаем все параметры из NVS. 
    // Если параметр еще ни разу не сохранялся, запишется дефолтное значение из правого аргумента.
    cfg_ap_ssid  = pref_get("ap_ssid",  "M5Stick_AP"); 
    cfg_ap_pass  = pref_get("ap_pass",  "12345678");
    cfg_sta_ssid = pref_get("sta_ssid", ""); 
    cfg_sta_pass = pref_get("sta_pass", "");
    cfg_mode     = pref_get("mode",     0);

    Serial.println(">>> All config loaded from Preferences:");
    Serial.printf("  Mode: %d\n  AP SSID: %s\n  STA SSID: %s\n", cfg_mode, cfg_ap_ssid.c_str(), cfg_sta_ssid.c_str());
}

void saveConfig() {
    // Благодаря встроенной в менеджер защите, физически запишутся только измененные поля!
    pref_set("ap_ssid",  cfg_ap_ssid);
    pref_set("ap_pass",  cfg_ap_pass);
    pref_set("sta_ssid", cfg_sta_ssid);
    pref_set("sta_pass", cfg_sta_pass);
    pref_set("mode",     cfg_mode);

    Serial.println(">>> Config smart-save completed.");
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
    for (int i = 0; i < 20; i++) {
        if (WiFi.status() == WL_CONNECTED) break;
        delay(500); Serial.print('.');
    }
    if (WiFi.status() != WL_CONNECTED) return false;

    configTime(0, 0, "pool.ntp.org");  // UTC
    struct tm timeinfo;
    for (int i = 0; i < 20; i++) {
        delay(500);
        if (getLocalTime(&timeinfo)) {
            m5::rtc_date_t date;
            date.year  = timeinfo.tm_year + 1900;
            date.month = timeinfo.tm_mon + 1;
            date.date  = timeinfo.tm_mday;

            m5::rtc_time_t time;
            time.hours   = timeinfo.tm_hour;
            time.minutes = timeinfo.tm_min;
            time.seconds = timeinfo.tm_sec;

            StickCP2.Rtc.setDate(date);
            StickCP2.Rtc.setTime(time);
            Serial.printf("NTP→RTC: %04d-%02d-%02d %02d:%02d:%02d\n",
                date.year, date.month, date.date,
                time.hours, time.minutes, time.seconds);
            break;
        }
    }
    displayText("Mode: STA\n" + cfg_sta_ssid + "\nIP: " + WiFi.localIP().toString());
    return true;
}

// ─────────────────────────────────────────
//  Helper: current date string YYYY-MM-DD
// ─────────────────────────────────────────
String currentDateString() {
    auto dt = StickCP2.Rtc.getDateTime();
    if (dt.date.year < 2020) return "nodate";
    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
        dt.date.year, dt.date.month, dt.date.date);
    return String(buf);
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
    <p style="margin:4px 0 0;font-size:13px;color:#888">
    Время: <span id="devtime">—</span>
    </p>
    <div style="display:flex;align-items:center;gap:10px;margin-top:10px">
    <button class="btn-save" id="sync-btn"
          style="width:auto;padding:8px 18px;font-size:13px"
          onclick="syncTime()">⏱ Синхронизировать время</button>
    <span class="status" id="sync-status">Нажмите для синхронизации</span>
    </div>
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
  let timeSynced = false;

  function pad(n){ return String(n).padStart(2,'0'); }

  function browserTs(){
    const n = new Date();
    return `${n.getFullYear()}-${pad(n.getMonth()+1)}-${pad(n.getDate())} `+
           `${pad(n.getHours())}:${pad(n.getMinutes())}:${pad(n.getSeconds())}`;
  }
  function browserDate(){
    const n = new Date();
    return `${n.getFullYear()}-${pad(n.getMonth()+1)}-${pad(n.getDate())}`;
  }

  let displayedTime = '—';
  let lastFetch = 0;

  function tickClock(){
    const status = document.getElementById('sync-status');
    if (timeSynced) {
      // Инкрементируем локально каждую секунду между опросами ESP
      displayedTime = displayedTime; // обновляется из fetchEspTime
      document.getElementById('devtime').textContent = displayedTime + ' ✓';
    } else {
      document.getElementById('devtime').textContent = 'не синхронизировано';
    }
  }

  function fetchEspTime(){
    fetch('/getTime')
      .then(r => r.text())
      .then(t => {
        if (t === 'unsynced') {
          timeSynced = false;
          document.getElementById('devtime').textContent = 'не синхронизировано';
        } else {
          timeSynced = true;
          displayedTime = t;
          document.getElementById('devtime').textContent = t + ' ✓';
          const fn = document.getElementById('fn');
          if (!fn.dataset.edited) fn.value = t.substring(0, 10);
        }
      })
      .catch(() => {
        document.getElementById('devtime').textContent = 'ошибка чтения';
      });
  }

  fetchEspTime();
  setInterval(fetchEspTime, 5000);

  function syncTime(){
    const btn = document.getElementById('sync-btn');
    const status = document.getElementById('sync-status');
    btn.disabled = true;
    status.textContent = 'Отправка…';
    status.style.color = '#888';

    const now = new Date();
    const localEpoch = Math.floor(now.getTime() / 1000) - (now.getTimezoneOffset() * 60);

    const params = new URLSearchParams();
    params.append('epoch', localEpoch.toString());

    fetch('/setTime', { method: 'POST', body: params })
      .then(r => {
        if (!r.ok) throw new Error('HTTP ' + r.status);
        return r.text();
      })
      .then(result => {
        timeSynced = true;
        status.textContent = 'Синхронизировано: ' + result;
        status.style.color = '#28a745';
        fetchEspTime();
      })
      .catch(e => {
        status.textContent = 'Ошибка: ' + e;
        status.style.color = '#dc3545';
      })
      .finally(() => { btn.disabled = false; });
  }

  document.getElementById('fn').addEventListener('input', function(){ this.dataset.edited='1'; });

  function saveFile(){
    const p = new URLSearchParams();
    p.append('filename', document.getElementById('fn').value);
    p.append('text', document.getElementById('t').value);
    const st = document.getElementById('save-status');
    st.textContent = 'Сохранение…';
    fetch('/setText',{method:'POST',body:p})
      .then(r=>r.text()).then(m=>{st.textContent=m;})
      .catch(e=>{st.textContent='Ошибка: '+e;});
  }

  function saveConfig(){
    const p = new URLSearchParams();
    p.append('ap_ssid', document.getElementById('ap_ssid').value);
    p.append('ap_pass',  document.getElementById('ap_pass').value);
    p.append('sta_ssid', document.getElementById('sta_ssid').value);
    p.append('sta_pass', document.getElementById('sta_pass').value);
    const st = document.getElementById('cfg-status');
    st.textContent = 'Сохранение…';
    fetch('/saveConfig',{method:'POST',body:p})
      .then(r=>r.text()).then(m=>{st.textContent=m;})
      .catch(e=>{st.textContent='Ошибка: '+e;});
  }
</script>
</body>
</html>)=====";

    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
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
    if (!server.hasArg("epoch")) {
        server.send(400, "text/plain", "Missing epoch");
        return;
    }

    unsigned long epoch = server.arg("epoch").toInt();
    if (epoch < 1000000000UL) {
        server.send(400, "text/plain", "Bad epoch: " + server.arg("epoch"));
        return;
    }

    // Конвертируем epoch в struct tm
    time_t t = (time_t)epoch;
    struct tm *ti = gmtime(&t);  // UTC → tm

    // Записываем в аппаратный RTC M5StickCP2
    m5::rtc_date_t date;
    date.year  = ti->tm_year + 1900;
    date.month = ti->tm_mon + 1;
    date.date  = ti->tm_mday;

    m5::rtc_time_t time;
    time.hours   = ti->tm_hour;
    time.minutes = ti->tm_min;
    time.seconds = ti->tm_sec;

    StickCP2.Rtc.setDate(date);
    StickCP2.Rtc.setTime(time);

    // Читаем обратно для проверки
    auto dt = StickCP2.Rtc.getDateTime();
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
        dt.date.year, dt.date.month, dt.date.date,
        dt.time.hours, dt.time.minutes, dt.time.seconds);

    Serial.printf("RTC set: %s\n", buf);
    server.send(200, "text/plain", String(buf));
}

void handleGetTime() {
    auto dt = StickCP2.Rtc.getDateTime();

    // Проверка: если год < 2020 — RTC не установлен
    if (dt.date.year < 2020) {
        server.send(200, "text/plain", "unsynced");
        return;
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
        dt.date.year, dt.date.month, dt.date.date,
        dt.time.hours, dt.time.minutes, dt.time.seconds);

    server.send(200, "text/plain", String(buf));
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
        saveConfig();
        startAP();
    }

    server.on("/",           handleRoot);
    server.on("/setText",    handleSetText);
    server.on("/setTime",    handleSetTime);
    server.on("/getTime", handleGetTime);
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