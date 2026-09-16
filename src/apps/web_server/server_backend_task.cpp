#include "M5StickCPlus2.h"
#include "server_backend_task.h"

#include <LittleFS.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <vector>
#include "../../core/preferences/prefs.h"

namespace {
// Экранирование спецсимволов для вставки строки в JSON.
String jsonEscape(const String &input) {
    String out;
    out.reserve(input.length());
    for (size_t i = 0; i < input.length(); ++i) {
        char c = input[i];
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}
}  // namespace

// ─────────────────────────────────────────
//  Config persistence
// ─────────────────────────────────────────

void ServerBackendTask::loadConfig() {
    cfg_ap_ssid  = prefGetString(KEY_AP_SSID, DEFAULT_AP_SSID);
    cfg_ap_pass  = prefGetString(KEY_AP_PASS, DEFAULT_AP_PASS);
    cfg_sta_ssid = prefGetString(KEY_STA_SSID, DEFAULT_STA_SSID);
    cfg_sta_pass = prefGetString(KEY_STA_PASS, DEFAULT_STA_PASS);
    cfg_mode     = prefGetBool(KEY_SERVER_MODE, false);
}

void ServerBackendTask::saveConfig() {
    prefSetString("ap_ssid",  cfg_ap_ssid);
    prefSetString("ap_pass",  cfg_ap_pass);
    prefSetString("sta_ssid", cfg_sta_ssid);
    prefSetString("sta_pass", cfg_sta_pass);
    prefSetBool("mode",     cfg_mode);
}

// ─────────────────────────────────────────
//  Network startup
// ─────────────────────────────────────────

void ServerBackendTask::startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(cfg_ap_ssid.c_str(), cfg_ap_pass.c_str());
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    statusText_ = "Mode: AP\n" + cfg_ap_ssid + "\nIP: " + WiFi.softAPIP().toString();
}

bool ServerBackendTask::startSTA() {
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
    statusText_ = "Mode: STA\n" + cfg_sta_ssid + "\nIP: " + WiFi.localIP().toString();
    return true;
}

void ServerBackendTask::stopNetwork() {
    server_.stop();

    WiFi.disconnect(true, true); // отключить STA и удалить настройки подключения
    WiFi.softAPdisconnect(true); // выключить AP

    WiFi.mode(WIFI_OFF);         // полностью выключить Wi-Fi
    delay(200);
}

void ServerBackendTask::applyMode() {
    bool staOk = false;
    if (cfg_mode == 1) {
        staOk = startSTA();
    }
    if (!staOk) {
        cfg_mode = 0;
        saveConfig();
        startAP();
    }
    server_.begin();
}

void ServerBackendTask::ToggleMode() {
    cfg_mode = !cfg_mode;
    saveConfig();
    statusText_ = "Switching to\n" + String(cfg_mode ? "STA" : "AP");
    stopNetwork();
    applyMode();
    Serial.println("Network restarted");
}

// ─────────────────────────────────────────
//  Helper: current date string YYYY-MM-DD
// ─────────────────────────────────────────

String ServerBackendTask::currentDateString() const {
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

void ServerBackendTask::saveTextToFile(const String &filename, const String &text) {
    String path = filename.startsWith("/") ? filename : "/" + filename;
    File file = LittleFS.open(path, FILE_WRITE);
    if (!file) { Serial.println("Failed to open file for writing"); return; }
    file.print(text);
    file.close();
    Serial.printf("Saved: %s\n", path.c_str());
}

String ServerBackendTask::normalizeUploadImageFilename(const String &filename) const {
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

void ServerBackendTask::handleUploadImagePost() {
    server_.send(200, "text/plain", imageUploadResponse_);
}

void ServerBackendTask::handleUploadImage() {
    HTTPUpload& upload = server_.upload();

    if (upload.status == UPLOAD_FILE_START) {
        String filename = normalizeUploadImageFilename(upload.filename);
        imageUploadSavedName_ = filename;

      imageUploadBytes_ = 0;
      imageUploadFile_ = LittleFS.open(filename, FILE_WRITE);
      imageUploadOpen_ = (bool)imageUploadFile_;
      imageUploadResponse_ = imageUploadOpen_ ? String("Uploading to ") + filename : String("Failed to open ") + filename;
      Serial.printf("Upload start: %s\n", filename.c_str());
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (imageUploadOpen_) {
        imageUploadFile_.write(upload.buf, upload.currentSize);
        imageUploadBytes_ += upload.currentSize;
        if (imageUploadBytes_ > kMaxImageUploadBytes) {
          imageUploadFile_.close();
          imageUploadOpen_ = false;
          LittleFS.remove(imageUploadSavedName_);
          imageUploadResponse_ = "Upload too large";
          Serial.printf("Upload aborted: %s (exceeded %u bytes)\n", imageUploadSavedName_.c_str(), (unsigned)kMaxImageUploadBytes);
        }
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (imageUploadOpen_) {
        imageUploadFile_.close();
        imageUploadOpen_ = false;
        imageUploadResponse_ = String("Saved image as: ") + imageUploadSavedName_;
        statusText_ = "Image: " + imageUploadSavedName_ + "\nSaved!";
        Serial.printf("Upload finished: %s (%u bytes)\n", imageUploadSavedName_.c_str(), (unsigned)imageUploadBytes_);
      } else {
        if (imageUploadResponse_.length() == 0) imageUploadResponse_ = "Upload failed";
      }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        if (imageUploadOpen_) {
            imageUploadFile_.close();
            imageUploadOpen_ = false;
        }
        imageUploadResponse_ = "Upload aborted";
    }
}

// ─────────────────────────────────────────
//  Файловый менеджер
// ─────────────────────────────────────────

// Нормализует путь: добавляет ведущий "/", убирает "..", двойные слэши
// и завершающий слэш (кроме корня). Защищает от выхода за пределы ФС.
String ServerBackendTask::sanitizeFsPath(const String &rawPath) const {
    String path = rawPath;
    if (path.length() == 0) path = "/";
    if (!path.startsWith("/")) path = "/" + path;

    while (path.indexOf("..") >= 0) {
        path.replace("..", "");
    }
    while (path.indexOf("//") >= 0) {
        path.replace("//", "/");
    }
    if (path.length() > 1 && path.endsWith("/")) {
        path.remove(path.length() - 1);
    }
    if (path.length() == 0) path = "/";
    return path;
}

// Рекурсивно удаляет файл или папку со всем содержимым.
bool ServerBackendTask::removeRecursiveFS(const String &path) const {
    File entry = LittleFS.open(path);
    if (!entry) return false;

    if (!entry.isDirectory()) {
        entry.close();
        return LittleFS.remove(path);
    }

    std::vector<String> children;
    File child = entry.openNextFile();
    while (child) {
        String name = String(child.name());
        int slash = name.lastIndexOf('/');
        if (slash >= 0) name = name.substring(slash + 1);
        children.push_back(name);
        child = entry.openNextFile();
    }
    entry.close();

    bool ok = true;
    for (const auto &name : children) {
        String childPath = (path == "/" ? "" : path) + "/" + name;
        ok = removeRecursiveFS(childPath) && ok;
    }
    return LittleFS.rmdir(path) && ok;
}

// GET /fm/list?path=/some/dir -> {"path":"...","entries":[{"name":..,"isDir":..,"size":..}, ...]}
void ServerBackendTask::handleFmList() {
    String path = sanitizeFsPath(server_.hasArg("path") ? server_.arg("path") : "/");

    File dir = LittleFS.open(path);
    if (!dir || !dir.isDirectory()) {
        server_.send(404, "application/json", "{\"error\":\"not a directory\"}");
        return;
    }

    String json = "{\"path\":\"" + jsonEscape(path) + "\",\"entries\":[";
    bool first = true;
    File file = dir.openNextFile();
    while (file) {
        String name = String(file.name());
        int slash = name.lastIndexOf('/');
        if (slash >= 0) name = name.substring(slash + 1);

        if (!first) json += ",";
        first = false;
        json += "{\"name\":\"" + jsonEscape(name) + "\",";
        json += "\"isDir\":";
        json += (file.isDirectory() ? "true" : "false");
        json += ",\"size\":" + String((unsigned long)file.size()) + "}";

        file = dir.openNextFile();
    }
    dir.close();
    json += "]}";

    server_.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server_.send(200, "application/json; charset=utf-8", json);
}

// POST /fm/mkdir  (path, name)
void ServerBackendTask::handleFmMkdir() {
    if (server_.method() != HTTP_POST) { server_.send(405, "text/plain", "Method Not Allowed"); return; }

    String parent = sanitizeFsPath(server_.arg("path"));
    String name = server_.arg("name");
    name.trim();

    if (name.length() == 0 || name.indexOf('/') >= 0 || name == "..") {
        server_.send(400, "text/plain", "Bad folder name");
        return;
    }

    String fullPath = (parent == "/" ? "" : parent) + "/" + name;
    if (LittleFS.mkdir(fullPath)) {
        server_.send(200, "text/plain", "Created: " + fullPath);
    } else {
        server_.send(500, "text/plain", "Failed to create folder");
    }
}

// POST /fm/delete  (path)
void ServerBackendTask::handleFmDelete() {
    if (server_.method() != HTTP_POST) { server_.send(405, "text/plain", "Method Not Allowed"); return; }

    String path = sanitizeFsPath(server_.arg("path"));
    if (path == "/") {
        server_.send(400, "text/plain", "Cannot delete root");
        return;
    }

    File entry = LittleFS.open(path);
    bool isDir = entry && entry.isDirectory();
    if (entry) entry.close();

    bool ok = isDir ? removeRecursiveFS(path) : LittleFS.remove(path);
    if (ok) {
        server_.send(200, "text/plain", "Deleted: " + path);
    } else {
        server_.send(500, "text/plain", "Failed to delete");
    }
}

// GET /fm/download?path=/some/file.ext
void ServerBackendTask::handleFmDownload() {
    String path = sanitizeFsPath(server_.arg("path"));
    File file = LittleFS.open(path, FILE_READ);
    if (!file || file.isDirectory()) {
        if (file) file.close();
        server_.send(404, "text/plain", "Not found");
        return;
    }

    String name = path;
    int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);

    server_.sendHeader("Content-Disposition", "attachment; filename=\"" + name + "\"");
    server_.streamFile(file, "application/octet-stream");
    file.close();
}

void ServerBackendTask::handleFmUploadPost() {
    server_.send(200, "text/plain", fmUploadResponse_);
}

// POST /fm/upload?path=/target/dir  (multipart, поле "file", любое имя/тип)
void ServerBackendTask::handleFmUpload() {
    HTTPUpload &upload = server_.upload();

    if (upload.status == UPLOAD_FILE_START) {
        String dir = sanitizeFsPath(server_.hasArg("path") ? server_.arg("path") : "/");

        String filename = upload.filename;
        int slash = filename.lastIndexOf('/');
        if (slash >= 0) filename = filename.substring(slash + 1);
        if (filename.length() == 0) filename = "file.bin";

        String fullPath = (dir == "/" ? "" : dir) + "/" + filename;

        fmUploadBytes_ = 0;
        fmUploadFile_ = LittleFS.open(fullPath, FILE_WRITE);
        fmUploadOpen_ = (bool)fmUploadFile_;
        fmUploadSavedName_ = fullPath;
        fmUploadResponse_ = fmUploadOpen_ ? String("Uploading to ") + fullPath : String("Failed to open ") + fullPath;
        Serial.printf("FM upload start: %s\n", fullPath.c_str());
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (fmUploadOpen_) {
            fmUploadFile_.write(upload.buf, upload.currentSize);
            fmUploadBytes_ += upload.currentSize;
            if (fmUploadBytes_ > kMaxFmUploadBytes) {
                fmUploadFile_.close();
                fmUploadOpen_ = false;
                LittleFS.remove(fmUploadSavedName_);
                fmUploadResponse_ = "Upload too large";
                Serial.printf("FM upload aborted: %s (exceeded %u bytes)\n", fmUploadSavedName_.c_str(), (unsigned)kMaxFmUploadBytes);
            }
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (fmUploadOpen_) {
            fmUploadFile_.close();
            fmUploadOpen_ = false;
            fmUploadResponse_ = String("Saved file as: ") + fmUploadSavedName_;
            statusText_ = "File: " + fmUploadSavedName_ + "\nUploaded!";
            Serial.printf("FM upload finished: %s (%u bytes)\n", fmUploadSavedName_.c_str(), (unsigned)fmUploadBytes_);
        } else {
            if (fmUploadResponse_.length() == 0) fmUploadResponse_ = "Upload failed";
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        if (fmUploadOpen_) {
            fmUploadFile_.close();
            fmUploadOpen_ = false;
        }
        fmUploadResponse_ = "Upload aborted";
    }
}

// ─────────────────────────────────────────
//  HTML page
// ─────────────────────────────────────────
void ServerBackendTask::handleRoot() {
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
  .fm-toolbar{display:flex;gap:8px;margin-bottom:10px;flex-wrap:wrap}
  .fm-btn{width:auto;padding:6px 12px;font-size:13px;border-radius:6px;border:1px solid #ccc;
    background:#f8f9fa;color:#333;cursor:pointer}
  .fm-btn:hover{background:#e9ecef}
  .fm-btn:disabled{opacity:.4;cursor:default}
  .fm-btn-del{color:#dc3545;border-color:#dc3545}
  .fm-path{font-family:monospace;font-size:13px;color:#555;background:#f8f9fa;padding:6px 8px;
    border-radius:6px;margin-bottom:8px;word-break:break-all}
  .fm-list{border:1px solid #eee;border-radius:6px;max-height:280px;overflow-y:auto}
  .fm-row{display:flex;align-items:center;gap:8px;padding:8px 10px;border-bottom:1px solid #f0f0f0}
  .fm-row:last-child{border-bottom:none}
  .fm-row .fm-btn{padding:4px 9px;font-size:12px}
  .fm-name{flex:1;word-break:break-all;font-size:14px}
  .fm-size{font-size:12px;color:#888;white-space:nowrap}
  .fm-empty{padding:16px;text-align:center;color:#999;font-size:13px}
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

  <!-- ── File manager ── -->
  <div class="card">
    <h2>Файловый менеджер</h2>
    <div class="fm-toolbar">
      <button class="fm-btn" id="fm-up" onclick="fmUp()">⬆ Назад</button>
      <button class="fm-btn" onclick="fmMkdir()">📁 Новая папка</button>
      <button class="fm-btn" onclick="fmLoad(fmPath)">↻ Обновить</button>
    </div>
    <div class="fm-path" id="fm-path">/</div>
    <div class="fm-list" id="fm-list"></div>
    <div style="margin-top:14px">
      <label>Загрузить файл в текущую папку</label>
      <input type="file" id="fm-file">
      <button class="btn-save" onclick="fmUpload()">Загрузить</button>
      <div class="status" id="fm-upload-status"></div>
    </div>
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
  // ── File manager ──
  let fmPath = '/';

  function fmIcon(isDir) { return isDir ? '📁' : '📄'; }

  function fmJoin(base, name) {
    return (base === '/' ? '' : base) + '/' + name;
  }

  function fmFormatSize(bytes) {
    if (bytes < 1024) return bytes + ' Б';
    if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' КБ';
    return (bytes / 1024 / 1024).toFixed(2) + ' МБ';
  }

  function fmRender(data) {
    fmPath = data.path;
    document.getElementById('fm-path').textContent = fmPath;
    document.getElementById('fm-up').disabled = (fmPath === '/');

    const list = document.getElementById('fm-list');
    list.innerHTML = '';

    const entries = (data.entries || []).slice().sort((a, b) => {
      if (a.isDir !== b.isDir) return a.isDir ? -1 : 1;
      return a.name.localeCompare(b.name);
    });

    if (entries.length === 0) {
      list.innerHTML = '<div class="fm-empty">Пусто</div>';
      return;
    }

    entries.forEach(e => {
      const row = document.createElement('div');
      row.className = 'fm-row';

      const nameEl = document.createElement('span');
      nameEl.className = 'fm-name';
      nameEl.textContent = fmIcon(e.isDir) + ' ' + e.name;
      if (e.isDir) {
        nameEl.style.cursor = 'pointer';
        nameEl.onclick = () => fmLoad(fmJoin(fmPath, e.name));
      }
      row.appendChild(nameEl);

      if (!e.isDir) {
        const sizeEl = document.createElement('span');
        sizeEl.className = 'fm-size';
        sizeEl.textContent = fmFormatSize(e.size);
        row.appendChild(sizeEl);

        const dlBtn = document.createElement('button');
        dlBtn.className = 'fm-btn';
        dlBtn.textContent = '⬇';
        dlBtn.title = 'Скачать';
        dlBtn.onclick = () => { window.location.href = '/fm/download?path=' + encodeURIComponent(fmJoin(fmPath, e.name)); };
        row.appendChild(dlBtn);
      }

      const delBtn = document.createElement('button');
      delBtn.className = 'fm-btn fm-btn-del';
      delBtn.textContent = '✕';
      delBtn.title = 'Удалить';
      delBtn.onclick = () => fmDelete(fmJoin(fmPath, e.name), e.isDir);
      row.appendChild(delBtn);

      list.appendChild(row);
    });
  }

  function fmLoad(path) {
    fetch('/fm/list?path=' + encodeURIComponent(path))
      .then(r => r.json())
      .then(fmRender)
      .catch(() => { document.getElementById('fm-list').innerHTML = '<div class="fm-empty">Ошибка загрузки</div>'; });
  }

  function fmUp() {
    if (fmPath === '/') return;
    const idx = fmPath.lastIndexOf('/');
    const parent = idx <= 0 ? '/' : fmPath.substring(0, idx);
    fmLoad(parent);
  }

  function fmMkdir() {
    const name = prompt('Имя новой папки:');
    if (!name) return;
    const p = new URLSearchParams();
    p.append('path', fmPath);
    p.append('name', name);
    fetch('/fm/mkdir', { method: 'POST', body: p })
      .then(r => r.text())
      .then(() => fmLoad(fmPath))
      .catch(e => alert('Ошибка: ' + e));
  }

  function fmDelete(path, isDir) {
    const msg = (isDir ? 'Удалить папку со всем содержимым: ' : 'Удалить файл: ') + path + '?';
    if (!confirm(msg)) return;
    const p = new URLSearchParams();
    p.append('path', path);
    fetch('/fm/delete', { method: 'POST', body: p })
      .then(r => r.text())
      .then(() => fmLoad(fmPath))
      .catch(e => alert('Ошибка: ' + e));
  }

  function fmUpload() {
    const input = document.getElementById('fm-file');
    const status = document.getElementById('fm-upload-status');
    if (!input.files.length) { status.textContent = 'Выберите файл'; return; }

    const file = input.files[0];
    const form = new FormData();
    form.append('file', file, file.name);

    status.textContent = 'Загрузка…';
    fetch('/fm/upload?path=' + encodeURIComponent(fmPath), { method: 'POST', body: form })
      .then(r => r.text())
      .then(t => { status.textContent = t; input.value = ''; fmLoad(fmPath); })
      .catch(e => { status.textContent = 'Ошибка: ' + e; });
  }

  fmLoad('/');

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
      displayedTime = displayedTime;
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

    server_.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server_.sendHeader("Pragma", "no-cache");
    server_.sendHeader("Expires", "0");
    server_.send(200, "text/html; charset=utf-8", html);
}

// ─────────────────────────────────────────
//  Handlers
// ─────────────────────────────────────────
void ServerBackendTask::handleSetText() {
    if (server_.method() != HTTP_POST) { server_.send(405, "text/plain", "Method Not Allowed"); return; }

    String filename = server_.arg("filename");
    String text     = server_.arg("text");

    if (filename.length() == 0) filename = currentDateString();
    if (!filename.endsWith(".txt") && !filename.endsWith(".script") && !filename.endsWith(".html"))
        filename += ".txt";

    saveTextToFile(filename, text);
    statusText_ = "File: " + filename + "\nSaved!";
    server_.send(200, "text/plain", "Saved as: " + filename);
}

void ServerBackendTask::handleSetTime() {
    if (!server_.hasArg("epoch")) {
        server_.send(400, "text/plain", "Missing epoch");
        return;
    }

    unsigned long epoch = server_.arg("epoch").toInt();
    if (epoch < 1000000000UL) {
        server_.send(400, "text/plain", "Bad epoch: " + server_.arg("epoch"));
        return;
    }

    time_t t = (time_t)epoch;
    struct tm *ti = gmtime(&t);  // UTC → tm

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

    auto dt = StickCP2.Rtc.getDateTime();
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
        dt.date.year, dt.date.month, dt.date.date,
        dt.time.hours, dt.time.minutes, dt.time.seconds);

    Serial.printf("RTC set: %s\n", buf);
    server_.send(200, "text/plain", String(buf));
}

void ServerBackendTask::handleGetTime() {
    auto dt = StickCP2.Rtc.getDateTime();

    if (dt.date.year < 2020) {
        server_.send(200, "text/plain", "unsynced");
        return;
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
        dt.date.year, dt.date.month, dt.date.date,
        dt.time.hours, dt.time.minutes, dt.time.seconds);

    server_.send(200, "text/plain", String(buf));
}

void ServerBackendTask::handleSaveConfig() {
    if (server_.method() != HTTP_POST) { server_.send(405, "text/plain", "Method Not Allowed"); return; }

    if (server_.hasArg("ap_ssid") && server_.arg("ap_ssid").length() > 0)
        cfg_ap_ssid = server_.arg("ap_ssid");
    if (server_.hasArg("ap_pass") && server_.arg("ap_pass").length() >= 8)
        cfg_ap_pass = server_.arg("ap_pass");
    if (server_.hasArg("sta_ssid"))
        cfg_sta_ssid = server_.arg("sta_ssid");
    if (server_.hasArg("sta_pass"))
        cfg_sta_pass = server_.arg("sta_pass");

    saveConfig();
    statusText_ = "Config saved!\nRestart to\napply network";
    server_.send(200, "text/plain", "Config saved. Press BtnA to switch mode or restart.");
}

// ─────────────────────────────────────────
//  Task lifecycle
// ─────────────────────────────────────────
void ServerBackendTask::Setup() {
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
    }

    loadConfig();
    applyMode();

    server_.on("/",           [this]() { handleRoot(); });
    server_.on("/setText",    [this]() { handleSetText(); });
    server_.on("/setTime",    [this]() { handleSetTime(); });
    server_.on("/getTime",    [this]() { handleGetTime(); });
    server_.on("/uploadImage", HTTP_POST, [this]() { handleUploadImagePost(); }, [this]() { handleUploadImage(); });
    server_.on("/saveConfig",  [this]() { handleSaveConfig(); });

    server_.on("/fm/list",     [this]() { handleFmList(); });
    server_.on("/fm/mkdir",    [this]() { handleFmMkdir(); });
    server_.on("/fm/delete",   [this]() { handleFmDelete(); });
    server_.on("/fm/download", [this]() { handleFmDownload(); });
    server_.on("/fm/upload", HTTP_POST, [this]() { handleFmUploadPost(); }, [this]() { handleFmUpload(); });

    server_.onNotFound([this]() { server_.send(404, "text/plain", "Not Found"); });

    server_.begin();
    Serial.println("Web server started!");
}

void ServerBackendTask::Stop() {
    stopNetwork();
}

bool ServerBackendTask::Loop() {
    server_.handleClient();
    // Задача работает бессрочно, пока её явно не остановят через
    // TaskManager::Stop("Server") (например, из ServerFrontendApp).
    return true;
}
