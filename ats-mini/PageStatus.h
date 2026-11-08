#ifndef PAGE_STATUS_H
#define PAGE_STATUS_H

#include <Arduino.h>

static const char pageStatusTitle[] PROGMEM = "ATS-Mini Status";

static const char pageStatus[] PROGMEM = R"HTML(<STYLE>
  .HERO { display: grid; grid-template-columns: repeat(auto-fit, minmax(290px, 1fr)); gap: 14px; max-width: 768px; margin: 14px auto; padding: 0 12px; }
  .HERO .pc { position: relative; overflow: hidden; border-radius: 18px; padding: 16px 18px; min-height: 190px; color: #fff; box-shadow: var(--shadow); }
  .HERO .tuned { background: linear-gradient(150deg, #16233f, #0c1426); border: 1px solid #1f2d4d; }
  .HERO .eyebrow { font-size: .8em; letter-spacing: .08em; text-transform: uppercase; color: #8fb4ff; }
  .HERO .tf { font: 700 3.1em/1.1 ui-monospace, Menlo, Consolas, monospace; color: #7cfc9a; margin-top: 6px; }
  .HERO .tf small { font-size: .38em; color: #9db0cc; margin-left: .3em; font-weight: 600; }
  .HERO .rds { margin-top: 6px; font-size: .9em; color: #bcd0f5; min-height: 1.2em; line-height: 1.35; }
  .HERO .rds B { color: #fff; letter-spacing: .06em; }
  .HERO .rds I { font-style: normal; opacity: .85; }
  .HERO .tsub { color: #cfe0ff; margin-top: 2px; font-size: 1.02em; min-height: 1.4em; }
  .HERO .chips { display: flex; flex-wrap: wrap; gap: 6px; margin-top: 12px; }
  .HERO .chip { background: rgba(255, 255, 255, .09); border-radius: 999px; padding: .25em .75em; font-size: .82em; color: #dbe7ff; }
  .HERO .eq { position: absolute; right: 16px; top: 16px; display: flex; gap: 3px; align-items: flex-end; height: 26px; }
  .HERO .eq i { display: block; width: 4px; height: 100%; border-radius: 2px; background: #7cfc9a; transform-origin: bottom; animation: eqb 1s ease-in-out infinite; }
  .HERO .eq i:nth-child(2) { animation-delay: -.3s; animation-duration: 1.3s; }
  .HERO .eq i:nth-child(3) { animation-delay: -.6s; animation-duration: .9s; }
  .HERO .eq i:nth-child(4) { animation-delay: -.2s; animation-duration: 1.15s; }
  .HERO .eq i:nth-child(5) { animation-delay: -.8s; animation-duration: 1.4s; }
  @keyframes eqb { 0% { transform: scaleY(.25); } 50% { transform: scaleY(1); } 100% { transform: scaleY(.25); } }
  .HERO .wx { background: linear-gradient(160deg, #2b7fff, #6bc4ff); transition: background .6s; }
  .HERO .wx.k-clear.night { background: linear-gradient(160deg, #0b1230, #23356b); }
  .HERO .wx.k-partly { background: linear-gradient(160deg, #3b86e8, #86b9e8); }
  .HERO .wx.k-partly.night { background: linear-gradient(160deg, #101a3a, #2e4478); }
  .HERO .wx.k-cloudy { background: linear-gradient(160deg, #56667d, #8f9db2); }
  .HERO .wx.k-fog { background: linear-gradient(160deg, #78838f, #aab3be); }
  .HERO .wx.k-rain { background: linear-gradient(160deg, #2c3a4e, #566c84); }
  .HERO .wx.k-snow { background: linear-gradient(160deg, #6a84a2, #b9c8da); }
  .HERO .wx.k-storm { background: linear-gradient(160deg, #181c30, #3d4262); }
  .HERO .wx .place { font-size: .85em; opacity: .9; letter-spacing: .02em; }
  .HERO .wx .now { display: flex; align-items: center; gap: 6px; }
  .HERO .wx .temp { font-size: 3.4em; font-weight: 700; line-height: 1; text-shadow: 0 2px 8px rgba(0, 0, 0, .25); }
  .HERO .wx .desc { font-size: 1.05em; text-shadow: 0 1px 4px rgba(0, 0, 0, .3); }
  .HERO .wx .meta { font-size: .8em; opacity: .9; margin-top: 2px; }
  .HERO .wx .wchips { display: flex; flex-wrap: wrap; gap: 6px; margin-top: 10px; }
  .HERO .wx .wchip { background: rgba(0, 0, 0, .22); border-radius: 10px; padding: .25em .6em; font-size: .78em; }
  .HERO .wx .days { display: grid; grid-template-columns: repeat(5, 1fr); gap: 4px; margin-top: 12px; text-align: center; font-size: .78em; }
  .HERO .wx .days div { background: rgba(0, 0, 0, .18); border-radius: 12px; padding: 4px 0 6px; }
  .HERO .wx .days .wi { width: 34px; height: 34px; }
  .HERO .wx .msg { opacity: .85; font-size: .9em; margin-top: 14px; }
  .wi { width: 92px; height: 92px; overflow: visible; flex: none; }
  .wi .spin { transform-box: fill-box; transform-origin: center; animation: wspin 26s linear infinite; }
  .wi .glow { transform-box: fill-box; transform-origin: center; animation: wglow 3.2s ease-in-out infinite; }
  .wi .drift { animation: wdrift 7s ease-in-out infinite alternate; }
  .wi .drift2 { animation: wdrift 9s ease-in-out infinite alternate-reverse; }
  .wi .drop { animation: wdrop 1s linear infinite; }
  .wi .flake { animation: wflake 2.6s linear infinite; }
  .wi .bolt { animation: wbolt 3s steps(1) infinite; }
  .wi .star { animation: wtw 2.4s ease-in-out infinite; }
  .wi .fog { animation: wdrift 5s ease-in-out infinite alternate; }
  @keyframes wspin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
  @keyframes wglow { 0% { transform: scale(1); opacity: .9; } 50% { transform: scale(1.12); opacity: 1; } 100% { transform: scale(1); opacity: .9; } }
  @keyframes wdrift { 0% { transform: translateX(-3px); } 100% { transform: translateX(3px); } }
  @keyframes wdrop { 0% { transform: translateY(-6px); opacity: 0; } 25% { opacity: 1; } 100% { transform: translateY(12px); opacity: 0; } }
  @keyframes wflake { 0% { transform: translate(0px, -6px); opacity: 0; } 20% { opacity: 1; } 50% { transform: translate(3px, 3px); } 100% { transform: translate(-2px, 14px); opacity: 0; } }
  @keyframes wbolt { 0% { opacity: 1; } 8% { opacity: .1; } 12% { opacity: 1; } 18% { opacity: .1; } 24% { opacity: 0; } 100% { opacity: 0; } }
  @keyframes wtw { 0% { opacity: .25; } 50% { opacity: 1; } 100% { opacity: .25; } }
</STYLE>
<H1>{{title}}</H1>
{{{navigation}}}
<DIV CLASS="HERO" ID="hero" DATA-LAT="{{lat}}" DATA-LON="{{lon}}" DATA-NAME="{{locname}}" DATA-LABEL="{{label}}" DATA-ICAO="{{icao}}">
  <DIV CLASS="pc tuned">
    <DIV CLASS="eq" ID="eq"><I></I><I></I><I></I><I></I><I></I></DIV>
    <DIV CLASS="eyebrow">You are tuned to</DIV>
    <DIV CLASS="tf"><SPAN ID="hf">{{frequency_big}}</SPAN><SMALL ID="hu">{{frequency_unit}}</SMALL></DIV>
    <DIV CLASS="tsub" ID="hs">{{band}}</DIV>
    <DIV CLASS="rds" ID="hrds"></DIV>
    <DIV CLASS="chips">
      <SPAN CLASS="chip" ID="hr">{{rssi}} dBuV</SPAN>
      <SPAN CLASS="chip" ID="hn">SNR {{snr}} dB</SPAN>
      <SPAN CLASS="chip" ID="hv">Vol {{volume}}</SPAN>
    </DIV>
  </DIV>
  <DIV CLASS="pc wx" ID="wx">
    <DIV CLASS="place" ID="wp">{{locname}}</DIV>
    <DIV CLASS="msg" ID="wm">Loading weather...</DIV>
  </DIV>
</DIV>
<SCRIPT>
(function() {
  var $ = function(id) { return document.getElementById(id); };
  var CF = '#f6f9fe', CS = '#8fa3c0';
  function cloud(x, y, sc, fill, cls) {
    return '<g transform="translate(' + x + ' ' + y + ') scale(' + sc + ')"><g class="' + (cls || '') + '">' +
      '<path d="M-24 12 H20 A10 10 0 0 0 22 -7.5 A13 13 0 0 0 -3 -12 A12 12 0 0 0 -22 -4 A9 9 0 0 0 -24 12 Z" fill="' + fill + '"/>' +
      '<path d="M-24 12 H20 A10 10 0 0 0 22 4 H-23 Z" fill="' + CS + '" opacity=".3"/></g></g>';
  }
  function sun(x, y, sc) {
    var r = '';
    for (var i = 0; i < 8; i++) r += '<line x1="0" y1="-17" x2="0" y2="-23" stroke="#ffd23f" stroke-width="3" stroke-linecap="round" transform="rotate(' + (i * 45) + ')"/>';
    return '<g transform="translate(' + x + ' ' + y + ') scale(' + sc + ')"><circle class="glow" r="15" fill="#ffd23f" opacity=".25"/>' +
      '<g class="spin">' + r + '</g><circle r="11" fill="#ffd23f"/></g>';
  }
  function moon(x, y, sc) {
    return '<g transform="translate(' + x + ' ' + y + ') scale(' + sc + ')">' +
      '<path d="M5 -11 A12 12 0 1 0 5 11 A11 11 0 0 1 5 -11 Z" fill="#f5f1d0"/>' +
      '<circle class="star" cx="16" cy="-12" r="1.6" fill="#fff"/><circle class="star" cx="19" cy="4" r="1.2" fill="#fff" style="animation-delay:-1s"/>' +
      '<circle class="star" cx="10" cy="-20" r="1.1" fill="#fff" style="animation-delay:-.5s"/></g>';
  }
  function drops(n, y) {
    var s = '';
    for (var i = 0; i < n; i++) {
      var x = -14 + i * (28 / (n - 1));
      s += '<line class="drop" x1="' + x + '" y1="' + y + '" x2="' + (x - 2) + '" y2="' + (y + 6) + '" stroke="#7cc4ff" stroke-width="2.4" stroke-linecap="round" style="animation-delay:' + (-(i * 0.23)).toFixed(2) + 's"/>';
    }
    return s;
  }
  function flakes(n, y) {
    var s = '';
    for (var i = 0; i < n; i++) {
      var x = -14 + i * (28 / (n - 1));
      s += '<circle class="flake" cx="' + x + '" cy="' + y + '" r="2" fill="#fff" style="animation-delay:' + (-(i * 0.5)).toFixed(2) + 's"/>';
    }
    return s;
  }
  function kindOf(c) {
    if (c <= 1) return 'clear';
    if (c === 2) return 'partly';
    if (c === 3) return 'cloudy';
    if (c === 45 || c === 48) return 'fog';
    if ((c >= 51 && c <= 67) || (c >= 80 && c <= 82)) return 'rain';
    if ((c >= 71 && c <= 77) || c === 85 || c === 86) return 'snow';
    if (c >= 95) return 'storm';
    return 'cloudy';
  }
  function icon(k, day) {
    var s = '<svg class="wi" viewBox="-32 -32 64 64" aria-hidden="true">';
    if (k === 'clear') s += day ? sun(0, 0, 1) : moon(0, 0, 1.1);
    else if (k === 'partly') s += (day ? sun(-8, -9, .75) : moon(-8, -9, .8)) + cloud(6, 7, .85, CF, 'drift');
    else if (k === 'cloudy') s += cloud(-9, -8, .6, '#b9c5d6', 'drift2') + cloud(4, 4, .95, CF, 'drift');
    else if (k === 'fog') s += cloud(0, -9, .85, '#d5dce8', 'drift') + '<g stroke="#e8edf5" stroke-width="3" stroke-linecap="round"><line class="fog" x1="-20" y1="12" x2="14" y2="12"/><line class="fog" x1="-14" y1="19" x2="20" y2="19" style="animation-delay:-2s"/></g>';
    else if (k === 'rain') s += cloud(0, -9, .95, '#dfe6f1', 'drift') + drops(5, 12);
    else if (k === 'snow') s += cloud(0, -9, .95, '#e9eef6', 'drift') + flakes(5, 12);
    else s += cloud(0, -10, .95, '#9aa3b8', 'drift') + '<polygon class="bolt" points="2,2 -6,14 0,14 -3,24 8,9 2,9" fill="#ffe14d"/>' + drops(3, 14);
    return s + '</svg>';
  }
  function desc(c) { return window.WXDESC(c); }
  var hero = $('hero'), LAT = hero.getAttribute('data-lat'), LON = hero.getAttribute('data-lon');
  var NAME = hero.getAttribute('data-name'), LABEL = hero.getAttribute('data-label'), ICAO = hero.getAttribute('data-icao');
  var URLW = 'https://api.open-meteo.com/v1/forecast?latitude=' + encodeURIComponent(LAT) + '&longitude=' + encodeURIComponent(LON) +
    '&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,precipitation,weather_code,pressure_msl,wind_speed_10m,wind_direction_10m' +
    '&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max&forecast_days=6&timezone=auto';
  function esc(t) { var d = document.createElement('div'); d.textContent = t; return d.innerHTML; }
  function renderWx(j) {
    var c = j.current, d = j.daily, k = kindOf(c.weather_code), day = c.is_day === 1;
    var box = $('wx');
    box.className = 'pc wx k-' + k + (day ? '' : ' night');
    var place = esc(NAME) + (ICAO ? ' &middot; ' + esc(ICAO) : (LABEL ? ' &middot; ' + esc(LABEL) : ''));
    var h = '<DIV CLASS="place">' + place + '</DIV><DIV CLASS="now">' + icon(k, day) +
      '<DIV><DIV CLASS="temp">' + Math.round(c.temperature_2m) + '&deg;</DIV><DIV CLASS="desc">' + desc(c.weather_code) + '</DIV></DIV></DIV>';
    h += '<DIV CLASS="meta">' + TR('Feels like') + ' ' + Math.round(c.apparent_temperature) + '&deg; &middot; ' + TR('Max') + ' ' + Math.round(d.temperature_2m_max[0]) + '&deg; &middot; ' + TR('Min') + ' ' + Math.round(d.temperature_2m_min[0]) + '&deg;</DIV>';
    h += '<DIV CLASS="wchips"><SPAN CLASS="wchip">' + TR('Humidity') + ' ' + Math.round(c.relative_humidity_2m) + '%</SPAN>' +
      '<SPAN CLASS="wchip">' + TR('Wind') + ' ' + Math.round(c.wind_speed_10m) + ' km/h</SPAN>' +
      '<SPAN CLASS="wchip">' + TR('Rain') + ' ' + Math.round(d.precipitation_probability_max[0] || 0) + '%</SPAN>' +
      '<SPAN CLASS="wchip">' + Math.round(c.pressure_msl) + ' hPa</SPAN></DIV><DIV CLASS="days">';
    for (var i = 1; i <= 5 && i < d.time.length; i++) {
      var dt = new Date(d.time[i] + 'T12:00:00');
      var wd = dt.toLocaleDateString(window.LOCALE, { weekday: 'short' }).replace('.', '');
      h += '<DIV><B>' + esc(wd) + '</B>' + icon(kindOf(d.weather_code[i]), true) + '<DIV>' + Math.round(d.temperature_2m_max[i]) + '&deg; <SMALL style="color:inherit;opacity:.7">' + Math.round(d.temperature_2m_min[i]) + '&deg;</SMALL></DIV></DIV>';
    }
    box.innerHTML = h + '</DIV>';
  }
  function loadWx() {
    var cached = null;
    try { cached = JSON.parse(localStorage.getItem('wx:' + LAT + ':' + LON) || 'null'); } catch (e) { cached = null; }
    if (cached && Date.now() - cached.t < 600000) { renderWx(cached.j); return; }
    fetch(URLW).then(function(r) { if (!r.ok) throw new Error(r.status); return r.json(); }).then(function(j) {
      try { localStorage.setItem('wx:' + LAT + ':' + LON, JSON.stringify({ t: Date.now(), j: j })); } catch (e) { }
      renderWx(j);
    }).catch(function() {
      if (cached) { renderWx(cached.j); return; }
      $('wm').textContent = TR('No weather: this device needs internet access.');
    });
  }
  function fmtF(s) {
    if (s.mode === 0) return [(s.freq / 1e6).toFixed(1), 'MHz'];
    return [(s.freq / 1e3).toFixed(s.mode === 3 ? 0 : 2), 'kHz'];
  }
  function esc(t) { return String(t).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;'); }
  var busy = false;
  function refreshRadio() {
    if (busy || document.hidden) return;
    busy = true;
    fetch('/api/status').then(function(r) { return r.json(); }).then(function(s) {
      var f = fmtF(s);
      $('hf').textContent = f[0]; $('hu').textContent = f[1];
      $('hs').textContent = s.bandName + (s.modeName !== s.bandName ? ' · ' + s.modeName : '') + (s.station ? ' · ' + s.station : '');
      var rd = $('hrds'), parts = [];
      if (s.station) parts.push('<B>' + esc(s.station) + '</B>');
      if (s.pty) parts.push(esc(s.pty));
      if (s.pi) parts.push('PI ' + esc(s.pi));
      rd.innerHTML = (parts.length ? 'RDS: ' + parts.join(' &middot; ') : '') + (s.rt ? '<BR><I>' + esc(s.rt) + '</I>' : '');
      $('hr').textContent = s.rssi + ' dBuV'; $('hn').textContent = 'SNR ' + s.snr + ' dB'; $('hv').textContent = 'Vol ' + s.volume;
    }).catch(function() { }).then(function() { busy = false; });
  }
  loadWx(); refreshRadio(); setInterval(loadWx, 600000);
  setInterval(refreshRadio, 3000);
})();
</SCRIPT>
<TABLE COLUMNS="2">
  <TR><TD CLASS="LABEL">IP Address</TD><TD><A HREF="http://{{ip}}">{{ip}}</A> ({{ssid}})</TD></TR>
  <TR><TD CLASS="LABEL">MAC Address</TD><TD>{{mac}}</TD></TR>
  <TR><TD CLASS="LABEL">Firmware</TD><TD>{{version}}</TD></TR>
  <TR><TD CLASS="LABEL">Date/Time</TD><TD>{{time}}</TD></TR>
  <TR><TD CLASS="LABEL">Band</TD><TD>{{band}}</TD></TR>
  <TR><TD CLASS="LABEL">Frequency</TD><TD>{{frequency}}{{mode}}</TD></TR>
  <TR><TD CLASS="LABEL">Signal Strength</TD><TD>{{rssi}}dBuV</TD></TR>
  <TR><TD CLASS="LABEL">Signal to Noise</TD><TD>{{snr}}dB</TD></TR>
  <TR><TD CLASS="LABEL">Last reset</TD><TD>{{lastreset}}</TD></TR>
  <TR><TD CLASS="LABEL">Battery Voltage</TD><TD>{{battery}}V</TD></TR>
</TABLE>
<P CLASS="CENTER"><A HREF="/screenshot.bmp">Download screenshot</A></P>
)HTML";

#endif // PAGE_STATUS_H
