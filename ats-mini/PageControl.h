#ifndef PAGE_CONTROL_H
#define PAGE_CONTROL_H

#include <Arduino.h>

static const char pageControlTitle[] PROGMEM = "ATS-Mini Control";

// NOTE: this text is a template: never write two consecutive opening braces in the script.
static const char pageControl[] PROGMEM = R"HTML(<STYLE>
  .RC { max-width: 560px; margin: 0 auto; padding: 0 12px 16px; }
  .RC .card { background: linear-gradient(160deg, #15203a, #0d1424); color: #e6edf7; border-radius: 18px; padding: 16px; margin: 14px 0; box-shadow: 0 8px 28px rgba(0, 0, 0, .25); border: 1px solid #1f2d4d; }
  .RC .freq { font: 700 3.3em/1.1 ui-monospace, Menlo, Consolas, monospace; text-align: center; color: #7cfc9a; letter-spacing: .03em; }
  .RC .sub { text-align: center; color: #9db0cc; margin-top: 4px; }
  .RC .row { display: flex; gap: 8px; margin: 10px 0 0; flex-wrap: wrap; }
  .RC .row > * { flex: 1; }
  .RC button { min-width: 0; padding: .7em 0; font-size: 1.05em; border: 0; border-radius: 10px; background: #2b3c5c; color: #fff; cursor: pointer; }
  .RC button:active { background: #3f5a8c; }
  .RC button.on { background: #2f7d4f; }
  .RC button:disabled { opacity: .35; cursor: default; }
  .RC input[type=text], .RC select { width: 100%; box-sizing: border-box; padding: .6em; border-radius: 8px; border: 1px solid #34466a; background: #0b1220; color: #fff; }
  .RC input[type=range] { width: 100%; }
  .RC label { color: #9db0cc; font-size: .85em; display: block; margin-bottom: 4px; }
  .RC .bar { height: 10px; background: #0b1220; border-radius: 6px; overflow: hidden; }
  .RC .bar > div { height: 100%; width: 0; background: linear-gradient(90deg, #2f7d4f, #e6c84a, #d9534f); transition: width .4s; }
  .RC .note { color: #9db0cc; font-size: .85em; margin-top: 10px; }
</STYLE>
<H1>{{title}}</H1>
{{{navigation}}}
<DIV CLASS="RC">
  <DIV CLASS="card" STYLE="text-align:center">
    <IMG ID="scr" ALT="Radio screen" STYLE="width:100%;max-width:640px;aspect-ratio:320/170;border-radius:10px;background:#000">
  </DIV>
  <DIV CLASS="card">
    <DIV CLASS="freq"><SPAN ID="freq">---</SPAN> <SMALL ID="unit"></SMALL></DIV>
    <DIV CLASS="sub"><SPAN ID="band">-</SPAN> &middot; <SPAN ID="mode">-</SPAN> &middot; <SPAN ID="conn">connecting</SPAN></DIV>
    <DIV CLASS="row">
      <BUTTON ID="dd" TITLE="-10 steps">&laquo;</BUTTON>
      <BUTTON ID="d1" TITLE="-1 step">&lsaquo;</BUTTON>
      <BUTTON ID="u1" TITLE="+1 step">&rsaquo;</BUTTON>
      <BUTTON ID="uu" TITLE="+10 steps">&raquo;</BUTTON>
    </DIV>
    <DIV CLASS="note">Step: <SPAN ID="step">-</SPAN></DIV>
  </DIV>

  <DIV CLASS="card">
    <LABEL>Go to frequency</LABEL>
    <DIV CLASS="row">
      <INPUT TYPE="text" ID="fin" INPUTMODE="decimal" PLACEHOLDER="e.g. 98.1 or 7150">
      <SELECT ID="fun" STYLE="max-width:6em"><OPTION VALUE="1000000">MHz</OPTION><OPTION VALUE="1000">kHz</OPTION></SELECT>
      <BUTTON ID="go" STYLE="max-width:5em">Go</BUTTON>
    </DIV>
    <DIV CLASS="row">
      <DIV><LABEL>Band</LABEL><SELECT ID="bsel">{{{bands}}}</SELECT></DIV>
    </DIV>
    <DIV CLASS="row" ID="modes">
      <BUTTON DATA-M="0">FM</BUTTON><BUTTON DATA-M="1">LSB</BUTTON><BUTTON DATA-M="2">USB</BUTTON><BUTTON DATA-M="3">AM</BUTTON>
    </DIV>
  </DIV>

  <DIV CLASS="card">
    <LABEL>Volume <SPAN ID="vol">-</SPAN></LABEL>
    <INPUT TYPE="range" ID="vsl" MIN="0" MAX="63" VALUE="35">
    <LABEL STYLE="margin-top:12px">Signal <SPAN ID="sig">-</SPAN></LABEL>
    <DIV CLASS="bar"><DIV ID="sbar"></DIV></DIV>
    <DIV CLASS="note">Audio streaming is not available: the SI4732 audio goes straight to the speaker amplifier and never reaches the ESP32.</DIV>
  </DIV>
</DIV>
<SCRIPT>
  var $ = function(id) { return document.getElementById(id); };
  var cur = null, busyUntil = 0;
  function fmt(s) {
    if (s.mode === 0) return [(s.freq / 1e6).toFixed(2), 'MHz'];
    return [(s.freq / 1e3).toFixed(s.mode === 3 ? 0 : 2), 'kHz'];
  }
  function stepText(st) { return st >= 1000000 ? (st / 1e6) + ' MHz' : st >= 1000 ? (st / 1e3) + ' kHz' : st + ' Hz'; }
  function render(s) {
    cur = s;
    var f = fmt(s);
    $('freq').textContent = f[0]; $('unit').textContent = f[1];
    $('band').textContent = s.bandName; $('mode').textContent = s.modeName;
    $('conn').textContent = 'online';
    $('step').textContent = stepText(s.step);
    $('bsel').value = s.band;
    if (Date.now() > busyUntil) { $('vsl').value = s.volume; $('vol').textContent = s.volume; }
    $('sig').textContent = s.rssi + ' dBuV / SNR ' + s.snr + ' dB';
    $('sbar').style.width = Math.min(100, Math.round(s.rssi * 100 / 70)) + '%';
    var buttons = $('modes').children;
    for (var i = 0; i < buttons.length; i++) {
      var m = +buttons[i].getAttribute('data-m');
      buttons[i].className = (m === s.mode) ? 'on' : '';
      buttons[i].disabled = (s.mode === 0) || (m === 0);
    }
    if (document.activeElement !== $('fin')) $('fun').value = (s.mode === 0) ? '1000000' : '1000';
  }
  function refresh() {
    fetch('/api/status').then(function(r) { return r.json(); }).then(render)
      .catch(function() { $('conn').textContent = 'offline'; });
  }
  function send(p) {
    fetch('/api/tune', { method: 'POST', body: new URLSearchParams(p) })
      .then(function() { setTimeout(refresh, 120); })
      .catch(function() { $('conn').textContent = 'offline'; });
  }
  $('dd').onclick = function() { send({ step: -10 }); };
  $('d1').onclick = function() { send({ step: -1 }); };
  $('u1').onclick = function() { send({ step: 1 }); };
  $('uu').onclick = function() { send({ step: 10 }); };
  $('go').onclick = function() {
    var v = parseFloat($('fin').value.replace(',', '.'));
    if (isNaN(v) || v <= 0) return;
    send({ freq: Math.round(v * parseFloat($('fun').value)) });
    $('fin').value = '';
  };
  $('fin').onkeydown = function(e) { if (e.key === 'Enter') $('go').onclick(); };
  $('bsel').onchange = function() { send({ band: $('bsel').value }); };
  $('modes').onclick = function(e) {
    var m = e.target.getAttribute && e.target.getAttribute('data-m');
    if (m !== null && m !== undefined && !e.target.disabled) send({ mode: m });
  };
  $('vsl').oninput = function() { busyUntil = Date.now() + 2000; $('vol').textContent = $('vsl').value; };
  $('vsl').onchange = function() { send({ volume: $('vsl').value }); };
  // Live mirror of the radio screen: one request at a time, paused while the tab is hidden
  function nextScr() {
    if (document.hidden) { setTimeout(nextScr, 1500); return; }
    var im = new Image();
    im.onload = function() { $('scr').src = im.src; setTimeout(nextScr, 1200); };
    im.onerror = function() { setTimeout(nextScr, 3000); };
    im.src = '/screenshot.bmp?t=' + Date.now();
  }
  nextScr();
  setInterval(refresh, 1500); refresh();
</SCRIPT>
)HTML";

#endif // PAGE_CONTROL_H
