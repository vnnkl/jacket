// The admin page, served at "/". Plain HTML/CSS/JS, no external files (works on the offline hotspot).
// Dynamic values are only ever set via textContent/value, never innerHTML.
#pragma once
#include <pgmspace.h>

static const char kAdminPage[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Jacket</title>
<style>
:root{--bg:#0f0f14;--card:#1b1b24;--line:#2c2c3a;--text:#f2f2f5;--dim:#9a9aae;--ok:#3ddc84;--bad:#ff5c6c;--acc:#ffcc33}
*{box-sizing:border-box}body{margin:0;font:16px/1.4 system-ui,sans-serif;background:var(--bg);color:var(--text)}
main{max-width:560px;margin:0 auto;padding:16px}h1{font-size:22px;margin:8px 0 16px}h2{font-size:15px;margin:0 0 12px;color:var(--dim);text-transform:uppercase;letter-spacing:.06em}
section{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:16px;margin-bottom:14px}
label{display:block;margin:10px 0 4px;color:var(--dim);font-size:14px}
input,select{width:100%;padding:11px;border-radius:10px;border:1px solid var(--line);background:#111118;color:var(--text);font-size:16px}
input[type=range]{padding:0}input[type=checkbox]{width:auto;margin-right:8px}
.row{display:flex;gap:10px}.row>*{flex:1}
button{padding:12px 14px;border:0;border-radius:10px;background:var(--acc);color:#111;font-weight:600;font-size:16px;cursor:pointer}
button.ghost{background:transparent;border:1px solid var(--line);color:var(--text)}button.danger{background:var(--bad);color:#fff}
button:disabled{opacity:.5}.btns{display:flex;gap:10px;flex-wrap:wrap;margin-top:14px}
.dot{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:8px;background:var(--bad)}.dot.on{background:var(--ok)}
.stat{display:flex;align-items:center;padding:4px 0}.muted{color:var(--dim);font-size:14px}
.msg{padding:10px 12px;border-radius:10px;margin-top:12px;display:none}.msg.err{display:block;background:#3a1720;color:#ffb3bc}.msg.ok{display:block;background:#143322;color:#a8f0c6}
.swatch{width:42px;height:42px;border-radius:10px;border:1px solid var(--line);flex:0 0 42px}
table{width:100%;border-collapse:collapse;font-size:14px}td{padding:6px 0;border-bottom:1px solid var(--line)}td:last-child{text-align:right;color:var(--dim)}
[hidden]{display:none!important}
</style></head><body><main>
<h1>Jacket</h1>

<section id="loading"><p class="muted">Loading…</p></section>

<section id="goHotspot" hidden><h2>Not set up yet</h2>
<p>Connect your phone to the Wi-Fi <b>Jacket-Setup</b> (default password <b>jacketsetup</b>) to set up the jacket.</p></section>

<section id="setup" hidden><h2>First-time setup</h2>
<form id="setupForm">
<label>Admin password (8+ characters, for this page)</label><input name="adminPassword" type="password" autocomplete="new-password" required minlength="8">
<label>Wi-Fi name (2.4 GHz)</label><input name="wifiSsid" required maxlength="32">
<label>Wi-Fi password</label><input name="wifiPass" type="password" autocomplete="off">
<label><input name="wifiOpen" type="checkbox">Open network (no password)</label>
<label>Payment websocket URL</label><input name="wsUrl" placeholder="wss://…/api/v1/ws/…" required>
<div class="btns"><button>Save and connect</button></div><div class="msg" data-msg></div></form></section>

<section id="login" hidden><h2>Log in</h2>
<form id="loginForm"><label>Admin password</label><input name="password" type="password" autocomplete="current-password" required>
<div class="btns"><button>Log in</button></div><div class="msg" data-msg></div></form></section>

<div id="app" hidden>
<section><h2>Status</h2>
<div class="stat"><span class="dot" id="sWifi"></span>Wi-Fi&nbsp;<span class="muted" id="sWifiText"></span></div>
<div class="stat"><span class="dot" id="sWs"></span>Payment websocket</div>
<div class="stat"><span class="dot" id="sJacket"></span>Jacket (Bluetooth)</div>
<div class="stat"><span class="dot" id="sHotspot"></span>Setup hotspot</div>
<p class="muted" id="sMeta"></p>
<div class="btns"><button class="ghost" data-test="small">Test small</button><button class="ghost" data-test="medium">Test medium</button><button class="ghost" data-test="big">Test big</button></div>
<div class="msg" id="testMsg"></div></section>

<section><h2>Last payments</h2><table><tbody id="payments"></tbody></table><p class="muted" id="noPayments">None since the last restart.</p></section>

<section><h2>Shows</h2><form id="showsForm">
<div class="row"><div><label>Minimum (sats)</label><input name="minSats" type="number" min="0" step="1"></div>
<div><label>Medium from</label><input name="mediumFrom" type="number" min="1" step="1"></div>
<div><label>Big from</label><input name="bigFrom" type="number" min="1" step="1"></div></div>
<label>Small-show colour</label><div class="row" style="align-items:center"><input name="smallColor" type="range" min="0" max="359"><div class="swatch" id="smallSwatch"></div></div>
<div class="row"><div><label>Small (s)</label><input name="smallSeconds" type="number" min="0.1" max="60" step="0.1"></div>
<div><label>Medium (s)</label><input name="mediumSeconds" type="number" min="0.1" max="60" step="0.1"></div>
<div><label>Big (s)</label><input name="bigSeconds" type="number" min="0.1" max="60" step="0.1"></div></div>
<label>Between payments</label><select name="idle"><option value="off">Off</option><option value="color">Colour</option><option value="preset">Built-in preset</option></select>
<div id="idleColorRow"><label>Idle colour</label><div class="row" style="align-items:center"><input name="idleColor" type="range" min="0" max="359"><div class="swatch" id="idleSwatch"></div></div></div>
<div id="idlePresetRow"><label>Preset number (0-20)</label><input name="idlePreset" type="number" min="0" max="255" step="1"></div>
<div class="btns"><button>Save shows</button></div><div class="msg" data-msg></div></form></section>

<section><h2>Connections</h2><form id="connForm">
<label>Wi-Fi name</label><input name="wifiSsid" maxlength="32">
<label>Wi-Fi password</label><input name="wifiPass" type="password" autocomplete="off" placeholder="unchanged">
<label><input name="wifiOpen" type="checkbox">Open network (no password)</label>
<label>Payment websocket URL</label><input name="wsUrl" placeholder="unchanged">
<label>Jacket Bluetooth address (empty = find automatically)</label><input name="jacketAddress" placeholder="aa:bb:cc:dd:ee:ff">
<label>Setup hotspot password</label><input name="hotspotPass" type="password" autocomplete="off" placeholder="unchanged">
<p class="muted">Saving restarts the jacket controller. Leave password fields empty to keep them.</p>
<div class="btns"><button>Save and restart</button></div><div class="msg" data-msg></div></form></section>

<section><h2>Admin password</h2><form id="pwForm">
<label>New admin password</label><input name="password" type="password" autocomplete="new-password" minlength="8" required>
<div class="btns"><button>Change password</button></div><div class="msg" data-msg></div></form></section>

<section><h2>Firmware</h2><form id="fwForm">
<label>Firmware file (.bin)</label><input name="file" type="file" accept=".bin" required>
<div class="btns"><button>Upload and restart</button></div><div class="msg" data-msg></div></form></section>

<section><h2>Danger zone</h2><div class="btns"><button class="ghost" id="reboot">Restart</button><button class="danger" id="reset">Factory reset</button><button class="ghost" id="logout">Log out</button></div>
<div class="msg" id="dangerMsg"></div></section>
</div>
</main>
<script>
const $=s=>document.querySelector(s);let auth=sessionStorage.getItem('auth')||'';let statusTimer=null;
const show=id=>{for(const s of ['loading','goHotspot','setup','login','app'])$('#'+s).hidden=s!==id};
function say(el,text,ok){el.textContent=text;el.className='msg '+(ok?'ok':'err')}
async function api(path,opts={}){const headers=Object.assign({},opts.headers||{});if(auth)headers.Authorization='Basic '+auth;
 const r=await fetch(path,Object.assign({},opts,{headers}));let body={};try{body=await r.json()}catch(e){}
 if(r.status===401&&!path.startsWith('/api/setup')){auth='';sessionStorage.removeItem('auth');show('login')}
 if(!r.ok)throw new Error(body.error||('Request failed ('+r.status+')'));return body}
const postJson=(path,data)=>api(path,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data)});
function formData(form){const d={};for(const el of form.elements){if(!el.name||el.type==='file')continue;
 if(el.type==='checkbox')d[el.name]=el.checked;else if(el.type==='number'||el.type==='range'){if(el.value!=='')d[el.name]=Number(el.value)}
 else d[el.name]=el.value.trim()}return d}
function hue(h){return 'hsl('+h+',100%,50%)'}
function bindSwatch(input,sw){const f=()=>sw.style.background=hue(input.value);input.addEventListener('input',f);f()}
function afterRestart(el){say(el,'Saved. Restarting… reconnect in a few seconds.',true);setTimeout(()=>location.reload(),8000)}
function onSubmit(id,fn){$(id).addEventListener('submit',async e=>{e.preventDefault();const f=e.target,m=f.querySelector('[data-msg]'),b=f.querySelector('button');
 b.disabled=true;try{await fn(f,m)}catch(err){say(m,err.message,false)}finally{b.disabled=false}})}
function ago(ms){const s=Math.round(ms/1000);return s<60?s+' s ago':s<3600?Math.round(s/60)+' min ago':Math.round(s/3600)+' h ago'}
function dot(id,on){$(id).className='dot'+(on?' on':'')}
async function refreshStatus(){try{const s=await api('/api/status');dot('#sWifi',s.wifi);dot('#sWs',s.websocket);dot('#sJacket',s.jacket);dot('#sHotspot',s.hotspot);
 $('#sWifiText').textContent=s.wifi?'('+s.wifiIp+', '+s.wifiRssi+' dBm)':'';
 $('#sMeta').textContent='Firmware '+s.version+' · up '+ago(s.uptimeMs).replace(' ago','');
 const tb=$('#payments');tb.replaceChildren();for(const p of s.payments){const tr=document.createElement('tr');
  for(const t of [p.sats.toLocaleString()+' sats',p.show,ago(p.agoMs)]){const td=document.createElement('td');td.textContent=t;tr.appendChild(td)}tb.appendChild(tr)}
 $('#noPayments').hidden=s.payments.length>0}catch(e){}}
function fill(form,s){for(const el of form.elements){if(!el.name||!(el.name in s))continue;if(el.type==='checkbox')el.checked=!!s[el.name];else el.value=s[el.name]}}
function idleRows(){const v=$('#showsForm').idle.value;$('#idleColorRow').hidden=v!=='color';$('#idlePresetRow').hidden=v!=='preset'}
async function loadApp(){const s=await api('/api/settings');fill($('#showsForm'),s);fill($('#connForm'),{wifiSsid:s.wifiSsid,jacketAddress:s.jacketAddress,wifiOpen:s.wifiSsid&&!s.wifiPassSet});
 $('#connForm').wsUrl.placeholder=s.wsUrlSet?'unchanged ('+s.wsHost+')':'wss://…';idleRows();
 bindSwatch($('#showsForm').smallColor,$('#smallSwatch'));bindSwatch($('#showsForm').idleColor,$('#idleSwatch'));
 show('app');refreshStatus();clearInterval(statusTimer);statusTimer=setInterval(refreshStatus,3000)}
async function start(){try{const st=await api('/api/state');
 if(!st.adminSet)return show(st.onHotspot?'setup':'goHotspot');if(!auth)return show('login');await loadApp()}catch(e){if(auth)show('login');else{$('#loading').textContent=e.message}}}
onSubmit('#setupForm',async(f,m)=>{await postJson('/api/setup',formData(f));auth=btoa('admin:'+f.adminPassword.value);sessionStorage.setItem('auth',auth);
 say(m,'Saved. The jacket is joining your Wi-Fi. Reconnect your phone to that Wi-Fi and open http://jacket.local',true)});
onSubmit('#loginForm',async(f,m)=>{auth=btoa('admin:'+f.password.value);try{await api('/api/settings');sessionStorage.setItem('auth',auth);f.reset();await loadApp()}
 catch(e){auth='';show('login');say($('#loginForm [data-msg]'),e.message,false)}});
onSubmit('#showsForm',async(f,m)=>{const d=formData(f);await postJson('/api/settings',d);say(m,'Saved.',true)});
onSubmit('#connForm',async(f,m)=>{const d=formData(f);for(const k of ['wifiPass','wsUrl','hotspotPass'])if(!d[k])delete d[k];
 const r=await postJson('/api/settings',d);r.restarting?afterRestart(m):say(m,'Saved.',true)});
onSubmit('#pwForm',async(f,m)=>{await postJson('/api/password',{password:f.password.value});auth=btoa('admin:'+f.password.value);sessionStorage.setItem('auth',auth);f.reset();say(m,'Password changed.',true)});
onSubmit('#fwForm',async(f,m)=>{const fd=new FormData();fd.append('firmware',f.file.files[0]);say(m,'Uploading… do not close this page.',true);
 await api('/update',{method:'POST',body:fd});afterRestart(m)});
for(const b of document.querySelectorAll('[data-test]'))b.addEventListener('click',async()=>{try{await api('/api/test?size='+b.dataset.test,{method:'POST'});say($('#testMsg'),'Playing the '+b.dataset.test+' show.',true)}catch(e){say($('#testMsg'),e.message,false)}});
$('#showsForm').idle.addEventListener('change',idleRows);
$('#reboot').addEventListener('click',async()=>{try{await api('/api/reboot',{method:'POST'});afterRestart($('#dangerMsg'))}catch(e){say($('#dangerMsg'),e.message,false)}});
$('#reset').addEventListener('click',async()=>{if(!confirm('Wipe all settings, Wi-Fi and the admin password?'))return;try{await api('/api/factory-reset',{method:'POST'});auth='';sessionStorage.removeItem('auth');
 say($('#dangerMsg'),'Wiped. Restarting into the Jacket-Setup hotspot.',true)}catch(e){say($('#dangerMsg'),e.message,false)}});
$('#logout').addEventListener('click',()=>{auth='';sessionStorage.removeItem('auth');clearInterval(statusTimer);show('login')});
start();
</script></body></html>)HTML";
