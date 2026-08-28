#include "M5StickCPlus2.h"
#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include "../../libs/prefs.h"
#include "../../libs/gui/gui.h"
#include <esp_sntp.h>
#include "server.h"

// --- Defaults ---
#define CFG_FILE "/server.cfg"
#define DEFAULT_AP_SSID     "M5Stick_AP"
#define DEFAULT_AP_PASS     "password123"
#define DEFAULT_STA_SSID    ""
#define DEFAULT_STA_PASS    ""
#define DEFAULT_MODE        0   // 0 = AP, 1 = STA

WebServer server(80);

// --- Runtime config ---

static String cfg_ap_ssid   = DEFAULT_AP_SSID;
static String cfg_ap_pass   = DEFAULT_AP_PASS;
static String cfg_sta_ssid  = DEFAULT_STA_SSID;
static String cfg_sta_pass  = DEFAULT_STA_PASS;
static bool   cfg_mode      = DEFAULT_MODE;   // current active mode

// ─────────────────────────────────────────
//  Config persistence
// ─────────────────────────────────────────

static void loadConfig() {
    cfg_ap_ssid  = prefGetString("ap_ssid",  "M5Stick_AP"); 
    cfg_ap_pass  = prefGetString("ap_pass",  "12345678");
    cfg_sta_ssid = prefGetString("sta_ssid", ""); 
    cfg_sta_pass = prefGetString("sta_pass", "");
    cfg_mode     = prefGetBool("mode", false);
}

static void saveConfig() {
    prefSetString("ap_ssid",  cfg_ap_ssid);
    prefSetString("ap_pass",  cfg_ap_pass);
    prefSetString("sta_ssid", cfg_sta_ssid);
    prefSetString("sta_pass", cfg_sta_pass);
    prefSetBool("mode",     cfg_mode);
}

// ─────────────────────────────────────────
//  Network startup
// ─────────────────────────────────────────

static void startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(cfg_ap_ssid.c_str(), cfg_ap_pass.c_str());
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    displayText(("Mode: AP\n" + cfg_ap_ssid + "\nIP: " + WiFi.softAPIP().toString()).c_str());
}

static bool startSTA() {
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
    displayText(("Mode: STA\n" + cfg_sta_ssid + "\nIP: " + WiFi.localIP().toString()).c_str());
    return true;
}

static void stopNetwork() {
    server.stop();

    WiFi.disconnect(true, true); // отключить STA и удалить настройки подключения
    WiFi.softAPdisconnect(true); // выключить AP

    WiFi.mode(WIFI_OFF);         // полностью выключить Wi-Fi
    delay(200);
}

// ─────────────────────────────────────────
//  Helper: current date string YYYY-MM-DD
// ─────────────────────────────────────────

static String currentDateString() {
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

static void saveTextToFile(const String &filename, const String &text) {
    String path = filename.startsWith("/") ? filename : "/" + filename;
    File file = LittleFS.open(path, FILE_WRITE);
    if (!file) { Serial.println("Failed to open file for writing"); return; }
    file.print(text);
    file.close();
    Serial.printf("Saved: %s\n", path.c_str());
}

static File imageUploadFile;
static String imageUploadResponse = "No image uploaded";
static bool imageUploadOpen = false;
static String imageUploadSavedName = "";
static size_t imageUploadBytes = 0;
static const size_t MAX_IMAGE_UPLOAD_BYTES = 2 * 1024 * 1024; // 2 MB limit

static String normalizeUploadImageFilename(const String &filename) {
    String name = filename;
    if (name.length() == 0) {
        name = currentDateString();
    }
    if (!name.startsWith("/")) {
        name = "/" + name;
    }

    int slashPos = name.lastIndexOf('/');
    int dotPos = name.lastIndexOf('.');
    if (dotPos <= slashPos) {
        name += ".bmp";
        return name;
    }

    String ext = name.substring(dotPos);
    ext.toLowerCase();
    if (ext != ".bmp") {
        name = name.substring(0, dotPos) + ".bmp";
    }
    return name;
}

static void handleUploadImagePost() {
    server.send(200, "text/plain", imageUploadResponse);
}

static void handleUploadImage() {
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        String filename = normalizeUploadImageFilename(upload.filename);
        imageUploadSavedName = filename;

      imageUploadBytes = 0;
      imageUploadFile = LittleFS.open(filename, FILE_WRITE);
      imageUploadOpen = (bool)imageUploadFile;
      imageUploadResponse = imageUploadOpen ? String("Uploading to ") + filename : String("Failed to open ") + filename;
      Serial.printf("Upload start: %s\n", filename.c_str());
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (imageUploadOpen) {
        imageUploadFile.write(upload.buf, upload.currentSize);
        imageUploadBytes += upload.currentSize;
        if (imageUploadBytes > MAX_IMAGE_UPLOAD_BYTES) {
          // Too large: abort and remove partial file
          imageUploadFile.close();
          imageUploadOpen = false;
          LittleFS.remove(imageUploadSavedName);
          imageUploadResponse = "Upload too large";
          Serial.printf("Upload aborted: %s (exceeded %u bytes)\n", imageUploadSavedName.c_str(), (unsigned)MAX_IMAGE_UPLOAD_BYTES);
        }
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (imageUploadOpen) {
        imageUploadFile.close();
        imageUploadOpen = false;
        imageUploadResponse = String("Saved image as: ") + imageUploadSavedName;
        displayText(("Image: " + imageUploadSavedName + "\nSaved!").c_str());
        Serial.printf("Upload finished: %s (%u bytes)\n", imageUploadSavedName.c_str(), (unsigned)imageUploadBytes);
      } else {
        if (imageUploadResponse.length() == 0) imageUploadResponse = "Upload failed";
      }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        if (imageUploadOpen) {
            imageUploadFile.close();
            imageUploadOpen = false;
        }
        imageUploadResponse = "Upload aborted";
    }
}

// ─────────────────────────────────────────
//  HTML page
// ─────────────────────────────────────────
static void handleRoot() {
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

  <!-- ── Upload image ── -->
  <div class="card">
    <h2>Загрузить изображение</h2>
    <label>Выберите файл</label>
    <input type="file" id="image-file" accept="image/*">
    <button class="btn-save" onclick="uploadImage()">Загрузить и сжать</button>
    <div class="status" id="upload-status"></div>
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

  function loadImage(file) {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = () => {
        const img = new Image();
        img.onload = () => resolve(img);
        img.onerror = reject;
        img.src = reader.result;
      };
      reader.onerror = reject;
      reader.readAsDataURL(file);
    });
  }

  function fixBmpName(name) {
    if (!name) return 'image.bmp';
    if (name.lastIndexOf('.') === -1) return name + '.bmp';
    return name.replace(/\.[^.]+$/, '.bmp');
  }

  function resizeDimensions(width, height, maxSide) {
    if (width <= maxSide && height <= maxSide) return [width, height];
    const ratio = width > height ? maxSide / width : maxSide / height;
    return [Math.round(width * ratio), Math.round(height * ratio)];
  }

  function canvasToBmpBlob(canvas) {
    const width = canvas.width;
    const height = canvas.height;
    const ctx = canvas.getContext('2d');
    const imageData = ctx.getImageData(0, 0, width, height);
    const pixels = imageData.data;
    const rowSize = Math.floor((24 * width + 31) / 32) * 4;
    const pixelArraySize = rowSize * height;
    const headerSize = 14 + 40;
    const fileSize = headerSize + pixelArraySize;
    const buffer = new ArrayBuffer(fileSize);
    const view = new DataView(buffer);
    let p = 0;
    view.setUint8(p++, 0x42);
    view.setUint8(p++, 0x4D);
    view.setUint32(p, fileSize, true);
    p += 4;
    view.setUint16(p, 0, true);
    p += 2;
    view.setUint16(p, 0, true);
    p += 2;
    view.setUint32(p, headerSize, true);
    p += 4;
    view.setUint32(p, 40, true);
    p += 4;
    view.setInt32(p, width, true);
    p += 4;
    view.setInt32(p, height, true);
    p += 4;
    view.setUint16(p, 1, true);
    p += 2;
    view.setUint16(p, 24, true);
    p += 2;
    view.setUint32(p, 0, true);
    p += 4;
    view.setUint32(p, pixelArraySize, true);
    p += 4;
    view.setUint32(p, 2835, true);
    p += 4;
    view.setUint32(p, 2835, true);
    p += 4;
    view.setUint32(p, 0, true);
    p += 4;
    view.setUint32(p, 0, true);
    p += 4;

    let offset = headerSize;
    const rowPadding = rowSize - width * 3;
    for (let y = height - 1; y >= 0; y--) {
      for (let x = 0; x < width; x++) {
        const i = (y * width + x) * 4;
        view.setUint8(offset++, pixels[i + 2]);
        view.setUint8(offset++, pixels[i + 1]);
        view.setUint8(offset++, pixels[i + 0]);
      }
      for (let pI = 0; pI < rowPadding; pI++) {
        view.setUint8(offset++, 0);
      }
    }

    return new Blob([buffer], { type: 'image/bmp' });
  }

  async function uploadImage() {
    const input = document.getElementById('image-file');
    const status = document.getElementById('upload-status');

    if (!input.files.length) {
      status.textContent = 'Выберите изображение сначала';
      return;
    }

    const file = input.files[0];
    status.textContent = 'Сжатие изображения…';

    try {
      const img = await loadImage(file);
      // Resize large images on the client to avoid large uploads and
      // memory pressure on the device. maxSide=800 produces smaller BMPs.
      const [width, height] = resizeDimensions(img.width, img.height, 800);
      const canvas = document.createElement('canvas');
      canvas.width = width;
      canvas.height = height;
      const ctx = canvas.getContext('2d');
      ctx.drawImage(img, 0, 0, width, height);

      const blob = canvasToBmpBlob(canvas);
      if (!blob) throw new Error('Не удалось создать BMP');

      const form = new FormData();
      form.append('image', blob, fixBmpName(file.name));

      status.textContent = 'Загрузка на устройство…';
      const resp = await fetch('/uploadImage', { method: 'POST', body: form });
      const text = await resp.text();
      status.textContent = resp.ok ? text : 'Ошибка: ' + text;
    } catch (err) {
      status.textContent = 'Ошибка: ' + err.message;
    }
  }

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
static void handleSetText() {
    if (server.method() != HTTP_POST) { server.send(405, "text/plain", "Method Not Allowed"); return; }

    String filename = server.arg("filename");
    String text     = server.arg("text");

    if (filename.length() == 0) filename = currentDateString();
    if (!filename.endsWith(".txt") && !filename.endsWith(".script") && !filename.endsWith(".html"))
        filename += ".txt";

    saveTextToFile(filename, text);
    displayText(("File: " + filename + "\nSaved!").c_str());
    server.send(200, "text/plain", "Saved as: " + filename);
}

static void handleSetTime() {
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

static void handleGetTime() {
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

static void handleSaveConfig() {
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
WebServerApp webServerApp;

void WebServerApp::Setup() {
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
    server.on("/getTime",     handleGetTime);
    server.on("/uploadImage", HTTP_POST, handleUploadImagePost, handleUploadImage);
    server.on("/saveConfig",  handleSaveConfig);
    server.onNotFound([]() { server.send(404, "text/plain", "Not Found"); });

    server.begin();
    Serial.println("Web server started!");
}

// ─────────────────────────────────────────
//  Loop
// ─────────────────────────────────────────
bool WebServerApp::Loop() {
    server.handleClient();

    if (StickCP2.BtnA.wasPressed()) {
        cfg_mode = !cfg_mode;
        saveConfig();

        displayText(("Switching to\n" + String(cfg_mode ? "STA" : "AP")).c_str());

        stopNetwork();

        bool ok = false;

        if (cfg_mode) {
            ok = startSTA();
        }

        if (!ok) {
            cfg_mode = 0;
            saveConfig();
            startAP();
        }

        server.begin();

        Serial.println("Network restarted");
    }

    if (StickCP2.BtnB.wasPressed()) {
      stopNetwork();
      displayText("WiFi OFF");
      delay(300);
      return false;
    }

    return true;
}