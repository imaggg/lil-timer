#include "web_server.h"
#include "wifi_manager.h"
#include <WebServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <string.h>

static WebServer server(80);
static TimerPreset* g_presets = nullptr;
static AppSettings* g_settings = nullptr;
static UIState* g_ui = nullptr;
static bool g_running = false;

// =====================
//     HTML PAGE
// =====================
static const char HTML_PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Darkroom Timer</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#1a0000;color:#cc0000;font-family:'Courier New',monospace;padding:12px;max-width:520px;margin:auto}
h1{text-align:center;margin:8px 0 16px;color:#ff2200;font-size:22px}
h2{margin:12px 0 8px;font-size:15px;color:#ff2200}
.tabs{display:flex;gap:2px;margin-bottom:14px}
.tab{flex:1;padding:10px;text-align:center;background:#2a0000;border:1px solid #440000;cursor:pointer;font-size:14px}
.tab.active{background:#440000;color:#ff2200;font-weight:bold}
.panel{display:none}.panel.active{display:block}
label{display:block;margin:8px 0 3px;font-size:13px}
input,select{width:100%;padding:7px;background:#2a0000;border:1px solid #440000;color:#cc0000;font-family:inherit;font-size:14px;border-radius:3px}
input:focus,select:focus{border-color:#880000;outline:none}
button{display:inline-block;padding:9px 18px;background:#440000;border:1px solid #660000;color:#cc0000;cursor:pointer;font-family:inherit;font-size:13px;border-radius:3px;margin:3px}
button:hover{background:#660000}
button:active{background:#880000}
.btn-primary{background:#660000;color:#ff2200;border-color:#880000}
.btn-primary:hover{background:#880000}
.step{display:flex;gap:4px;align-items:center;margin:5px 0;padding:7px;background:#200000;border:1px solid #330000;border-radius:3px;flex-wrap:wrap}
.step select{width:72px}
.step input[type=number]{width:48px;text-align:center;padding:5px 2px}
.step label{display:inline;margin:0;white-space:nowrap;font-size:12px}
.step input[type=checkbox]{width:auto;margin:0 2px}
.rm{background:none;border:none;color:#660000;cursor:pointer;font-size:18px;padding:2px 8px}
.rm:hover{color:#ff0000}
.msg{padding:8px;margin:8px 0;background:#200000;border:1px solid #330000;border-radius:3px;font-size:13px}
.msg.ok{border-color:#004400;color:#00aa00}
.net{padding:7px 10px;margin:3px 0;background:#200000;border:1px solid #330000;cursor:pointer;display:flex;justify-content:space-between;border-radius:3px;font-size:13px}
.net:hover{background:#2a0000}
.row{display:flex;gap:8px;align-items:center;margin:8px 0}
.row label{margin:0;flex-shrink:0;width:90px;font-size:13px}
.row input,.row select{flex:1}
.center{text-align:center;margin:12px 0}
input[type=range]{-webkit-appearance:none;height:6px;background:#440000;border:none;border-radius:3px}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:18px;height:18px;background:#cc0000;border-radius:50%;cursor:pointer}
</style></head><body>
<h1>DARKROOM TIMER</h1>
<div class="tabs">
<div class="tab active" data-tab="presets">Presets</div>
<div class="tab" data-tab="settings">Settings</div>
<div class="tab" data-tab="wifi">WiFi</div>
</div>
<div id="presets" class="panel active">
<div class="row"><label>Preset:</label><select id="psel"></select></div>
<div class="row"><label>Name:</label><input id="pname" maxlength="10"></div>
<h2>Steps</h2>
<div id="steps"></div>
<button onclick="addStep()">+ Add Step</button>
<div class="center"><button class="btn-primary" onclick="savePreset()">Save Preset</button></div>
<div id="pmsg"></div>
</div>

<div id="settings" class="panel">
<div class="row"><label>Brightness:</label><input type="range" id="sbright" min="0" max="4" step="1"></div>
<div class="row"><label>Volume:</label><select id="svol"><option value="0">OFF</option><option value="1">LOW</option><option value="2">MED</option><option value="3">HIGH</option></select></div>
<div class="row"><label>Language:</label><select id="slang"><option value="0">EN</option><option value="1">UK</option></select></div>
<div class="row"><label>Swap A/B:</label><select id="sswap"><option value="0">OFF</option><option value="1">ON</option></select></div>
<div class="center"><button class="btn-primary" onclick="saveSettings()">Save</button></div>
<div id="smsg"></div>
</div>

<div id="wifi" class="panel">
<div class="msg" id="wstat">Loading...</div>
<div class="center"><button onclick="scanWifi()">Scan Networks</button></div>
<div id="nets"></div>
<div class="row"><label>SSID:</label><input id="wssid"></div>
<div class="row"><label>Password:</label><input id="wpass" type="password"></div>
<div class="center">
<button class="btn-primary" onclick="connectWifi()">Connect</button>
<button onclick="disconnectWifi()">Disconnect</button>
</div>
<div id="wmsg"></div>
</div>

<script>
const LABELS=['DEV','STOP','FIX','STB','WASH','HCA','#1','#2','#3','#4'];
let P=[],cur=0;

document.querySelectorAll('.tab').forEach(t=>t.onclick=function(){
  document.querySelectorAll('.tab').forEach(x=>x.classList.remove('active'));
  document.querySelectorAll('.panel').forEach(x=>x.classList.remove('active'));
  this.classList.add('active');
  document.getElementById(this.dataset.tab).classList.add('active');
});

function showMsg(id,text,ok){
  let el=document.getElementById(id);
  el.innerHTML='<div class="msg'+(ok?' ok':'')+'">'+text+'</div>';
  setTimeout(()=>el.innerHTML='',3000);
}

async function init(){
  try{
    let r=await fetch('/api/presets');P=await r.json();
    let sel=document.getElementById('psel');sel.innerHTML='';
    P.forEach((p,i)=>{let o=document.createElement('option');o.value=i;o.text=p.name||('Preset '+(i+1));sel.add(o)});
    sel.onchange=()=>{cur=+sel.value;showPreset()};
    showPreset();
    r=await fetch('/api/settings');let s=await r.json();
    document.getElementById('sbright').value=s.brightness;
    document.getElementById('svol').value=s.volume;
    document.getElementById('slang').value=s.lang_uk?1:0;
    document.getElementById('sswap').value=s.swap_ab?1:0;
    loadWifiStatus();
  }catch(e){showMsg('pmsg','Error loading data: '+e,false)}
}

function showPreset(){
  let p=P[cur];
  document.getElementById('pname').value=p.name;
  let d=document.getElementById('steps');d.innerHTML='';
  (p.steps||[]).forEach(s=>d.appendChild(mkStep(s)));
}

function mkStep(s){
  let d=document.createElement('div');d.className='step';
  let sel=document.createElement('select');
  LABELS.forEach(l=>{let o=document.createElement('option');o.value=l;o.text=l;if(l===s.label)o.selected=true;sel.add(o)});
  if(s.label&&!LABELS.includes(s.label)){let o=document.createElement('option');o.value=s.label;o.text=s.label;o.selected=true;sel.add(o)}
  let mm=document.createElement('input');mm.type='number';mm.min=0;mm.max=99;mm.value=Math.floor((s.duration||60)/60);
  let sp=document.createElement('span');sp.textContent=':';sp.style.color='#ff2200';
  let ss=document.createElement('input');ss.type='number';ss.min=0;ss.max=59;mm.step=1;ss.step=1;
  ss.value=String((s.duration||60)%60).padStart(2,'0');
  let lb=document.createElement('label');
  let cb=document.createElement('input');cb.type='checkbox';cb.checked=s.sound!==false;
  lb.appendChild(cb);lb.append(' Snd');
  let rm=document.createElement('button');rm.className='rm';rm.textContent='\u2715';rm.onclick=()=>d.remove();
  d.append(sel,' ',mm,sp,ss,' ',lb,' ',rm);
  d._getData=()=>({label:sel.value,duration:Math.max(5,+mm.value*60+(+ss.value)),sound:cb.checked});
  return d;
}

function addStep(){
  let d=document.getElementById('steps');
  if(d.children.length>=10)return;
  d.appendChild(mkStep({label:'DEV',duration:60,sound:true}));
}

function getPresetData(){
  let steps=[];
  document.querySelectorAll('#steps .step').forEach(d=>{if(d._getData)steps.push(d._getData())});
  return{name:document.getElementById('pname').value.trim()||('Preset '+(cur+1)),steps};
}

async function savePreset(){
  try{
    P[cur]=getPresetData();
    let r=await fetch('/api/preset/'+cur,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(P[cur])});
    if(r.ok){
      document.getElementById('psel').options[cur].text=P[cur].name;
      showMsg('pmsg','Saved!',true);
    }else showMsg('pmsg','Save failed',false);
  }catch(e){showMsg('pmsg','Error: '+e,false)}
}

async function saveSettings(){
  try{
    let s={brightness:+document.getElementById('sbright').value,volume:+document.getElementById('svol').value,
      lang_uk:document.getElementById('slang').value==='1',swap_ab:document.getElementById('sswap').value==='1'};
    let r=await fetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(s)});
    showMsg('smsg',r.ok?'Saved!':'Failed',r.ok);
  }catch(e){showMsg('smsg','Error: '+e,false)}
}

async function loadWifiStatus(){
  try{
    let r=await fetch('/api/wifi/status');let s=await r.json();
    let t='AP: '+s.ap_ssid+' / '+s.ap_ip;
    if(s.sta_connected)t+='<br>Connected: '+s.sta_ssid+' / '+s.sta_ip;
    else if(s.sta_ssid)t+='<br>Connecting to '+s.sta_ssid+'...';
    document.getElementById('wstat').innerHTML=t;
    if(s.sta_ssid)document.getElementById('wssid').value=s.sta_ssid;
  }catch(e){document.getElementById('wstat').textContent='Error loading status'}
}

async function scanWifi(){
  document.getElementById('nets').innerHTML='<div class="msg">Scanning...</div>';
  try{
    let r=await fetch('/api/wifi/scan');let nets=await r.json();
    let d=document.getElementById('nets');d.innerHTML='';
    if(!nets.length){d.innerHTML='<div class="msg">No networks found</div>';return}
    nets.forEach(n=>{
      let el=document.createElement('div');el.className='net';
      el.innerHTML='<span>'+n.ssid+'</span><span>'+n.rssi+'dBm'+(n.open?' ':'&#x1f512;')+'</span>';
      el.onclick=()=>{document.getElementById('wssid').value=n.ssid;document.getElementById('wpass').focus()};
      d.appendChild(el);
    });
  }catch(e){document.getElementById('nets').innerHTML='<div class="msg">Scan failed</div>'}
}

async function connectWifi(){
  let ssid=document.getElementById('wssid').value.trim();
  if(!ssid)return;
  let pass=document.getElementById('wpass').value;
  try{
    await fetch('/api/wifi/connect',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid,pass})});
    showMsg('wmsg','Connecting to '+ssid+'...',true);
    setTimeout(loadWifiStatus,6000);
  }catch(e){showMsg('wmsg','Error: '+e,false)}
}

async function disconnectWifi(){
  try{
    await fetch('/api/wifi/disconnect',{method:'POST'});
    showMsg('wmsg','Disconnected',true);
    document.getElementById('wssid').value='';
    document.getElementById('wpass').value='';
    setTimeout(loadWifiStatus,1000);
  }catch(e){showMsg('wmsg','Error: '+e,false)}
}

init();
</script></body></html>)rawliteral";

// =====================
//     API HANDLERS
// =====================

static void sendTimerStatus() {
    TimerEngine& eng = g_ui->timer_engine;
    const char* stateStr = "ready";
    if (eng.state == TIMER_RUNNING) stateStr = "running";
    else if (eng.state == TIMER_PAUSED) stateStr = "paused";
    else if (eng.state == TIMER_DONE) stateStr = "done";

    JsonDocument doc;
    doc["state"] = stateStr;
    doc["step"] = eng.current_step;
    doc["remaining"] = eng.remaining_sec;
    if (eng.preset && eng.current_step < eng.preset->step_count)
        doc["label"] = eng.preset->steps[eng.current_step].label;
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

static void handleTimerStatus() {
    sendTimerStatus();
}

static void handleTimerToggle() {
    if (g_ui && g_ui->timer_engine.preset) {
        TimerEngine& eng = g_ui->timer_engine;
        if (eng.state == TIMER_READY)        timerStart(eng);
        else if (eng.state == TIMER_RUNNING) timerPause(eng);
        else if (eng.state == TIMER_PAUSED)  timerResume(eng);
    }
    sendTimerStatus();
}

static void handleTimerNext() {
    if (g_ui && g_ui->timer_engine.preset)
        timerSkipStep(g_ui->timer_engine);
    sendTimerStatus();
}

static void handleRoot() {
    server.send(200, "text/html", HTML_PAGE);
}

static void handleGetPresets() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < MAX_PRESETS; i++) {
        JsonObject p = arr.add<JsonObject>();
        p["name"] = g_presets[i].name;
        JsonArray steps = p["steps"].to<JsonArray>();
        for (int s = 0; s < g_presets[i].step_count; s++) {
            JsonObject st = steps.add<JsonObject>();
            st["label"] = g_presets[i].steps[s].label;
            st["duration"] = g_presets[i].steps[s].duration_sec;
            st["sound"] = g_presets[i].steps[s].end_sound_enabled;
        }
    }
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

static void handleSavePreset(int index) {
    if (index < 0 || index >= MAX_PRESETS) {
        server.send(400, "text/plain", "Invalid index");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }

    TimerPreset& preset = g_presets[index];
    const char* name = doc["name"] | "Preset";
    strncpy(preset.name, name, MAX_PRESET_NAME);
    preset.name[MAX_PRESET_NAME] = '\0';

    JsonArray steps = doc["steps"];
    preset.step_count = 0;
    for (JsonObject st : steps) {
        if (preset.step_count >= MAX_STEPS) break;
        TimerStep& step = preset.steps[preset.step_count];
        const char* label = st["label"] | "DEV";
        strncpy(step.label, label, MAX_LABEL_LEN);
        step.label[MAX_LABEL_LEN] = '\0';
        step.duration_sec = st["duration"] | 60;
        if (step.duration_sec < MIN_DURATION_SEC) step.duration_sec = MIN_DURATION_SEC;
        if (step.duration_sec > MAX_DURATION_SEC) step.duration_sec = MAX_DURATION_SEC;
        step.end_sound_enabled = st["sound"] | true;
        preset.step_count++;
    }

    storageSavePreset(index, preset);
    server.send(200, "text/plain", "OK");
}

static void handleGetSettings() {
    JsonDocument doc;
    doc["brightness"] = g_settings->brightness;
    doc["volume"] = g_settings->volume;
    doc["lang_uk"] = g_settings->lang_uk;
    doc["swap_ab"] = g_settings->swap_ab;
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

static void handleSaveSettings() {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }

    g_settings->brightness = doc["brightness"] | DEFAULT_BRIGHTNESS;
    g_settings->volume = doc["volume"] | DEFAULT_VOLUME;
    g_settings->lang_uk = doc["lang_uk"] | false;
    g_settings->swap_ab = doc["swap_ab"] | false;

    if (g_settings->brightness >= BRIGHTNESS_COUNT) g_settings->brightness = DEFAULT_BRIGHTNESS;
    if (g_settings->volume >= VOLUME_COUNT) g_settings->volume = DEFAULT_VOLUME;

    storageSaveSettings(*g_settings);
    server.send(200, "text/plain", "OK");
}

static void handleWifiStatus() {
    JsonDocument doc;
    doc["ap_ssid"] = WIFI_AP_SSID;
    doc["ap_ip"] = WiFi.softAPIP().toString();
    doc["sta_connected"] = wifiIsSTAConnected();
    doc["sta_ip"] = WiFi.localIP().toString();
    doc["sta_ssid"] = g_settings->wifi_ssid;
    doc["ip"] = wifiGetIP();
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

static void handleWifiScan() {
    int n = WiFi.scanNetworks();
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < n; i++) {
        JsonObject net = arr.add<JsonObject>();
        net["ssid"] = WiFi.SSID(i);
        net["rssi"] = WiFi.RSSI(i);
        net["open"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
    }
    WiFi.scanDelete();
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

static void handleWifiConnect() {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }

    const char* ssid = doc["ssid"] | "";
    const char* pass = doc["pass"] | "";

    strncpy(g_settings->wifi_ssid, ssid, 32);
    g_settings->wifi_ssid[32] = '\0';
    strncpy(g_settings->wifi_pass, pass, 64);
    g_settings->wifi_pass[64] = '\0';
    storageSaveSettings(*g_settings);

    WiFi.begin(g_settings->wifi_ssid, g_settings->wifi_pass);
    server.send(200, "text/plain", "OK");
}

static void handleWifiDisconnect() {
    WiFi.disconnect();
    g_settings->wifi_ssid[0] = '\0';
    g_settings->wifi_pass[0] = '\0';
    storageSaveSettings(*g_settings);
    server.send(200, "text/plain", "OK");
}

// =====================
//     PUBLIC API
// =====================

void webServerStart(TimerPreset presets[], AppSettings& settings, UIState& ui) {
    if (g_running) return;
    g_presets = presets;
    g_settings = &settings;
    g_ui = &ui;

    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/presets", HTTP_GET, handleGetPresets);
    server.on("/api/settings", HTTP_GET, handleGetSettings);
    server.on("/api/settings", HTTP_POST, handleSaveSettings);
    server.on("/api/wifi/status", HTTP_GET, handleWifiStatus);
    server.on("/api/wifi/scan", HTTP_GET, handleWifiScan);
    server.on("/api/wifi/connect", HTTP_POST, handleWifiConnect);
    server.on("/api/wifi/disconnect", HTTP_POST, handleWifiDisconnect);
    server.on("/api/timer", HTTP_GET, handleTimerStatus);
    server.on("/api/timer/toggle", HTTP_GET, handleTimerToggle);
    server.on("/api/timer/next", HTTP_GET, handleTimerNext);

    for (int i = 0; i < MAX_PRESETS; i++) {
        char path[20];
        snprintf(path, sizeof(path), "/api/preset/%d", i);
        int idx = i;
        server.on(path, HTTP_POST, [idx]() { handleSavePreset(idx); });
    }

    server.begin();
    g_running = true;
}

void webServerStop() {
    if (!g_running) return;
    server.stop();
    g_running = false;
}

void webServerHandle() {
    if (g_running) server.handleClient();
}

bool webServerIsRunning() {
    return g_running;
}
