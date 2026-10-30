#ifndef PAGE_BACKUP_H
#define PAGE_BACKUP_H

#include <Arduino.h>

static const char pageBackupTitle[] PROGMEM = "ATS-Mini Backup";

static const char pageBackup[] PROGMEM = R"HTML(<H1>{{title}}</H1>
{{{navigation}}}
<TABLE COLUMNS="2">
  <TR><TH CLASS="HEADING" COLSPAN="2">Export</TH></TR>
  <TR><TD COLSPAN="2">Saves every setting, memory, band, position and map option into one file.<BR>
    <LABEL><INPUT TYPE="checkbox" ID="secrets"> Include Wi-Fi networks, the web login and the map key</LABEL><BR>
    <SMALL>Leave this off if you will share the file: those items are passwords.</SMALL></TD></TR>
  <TR><TD COLSPAN="2" CLASS="CENTER"><BUTTON TYPE="button" ID="exp">Download backup</BUTTON></TD></TR>
  <TR><TH CLASS="HEADING" COLSPAN="2">Import</TH></TR>
  <TR><TD COLSPAN="2">Restores a backup file made by this page. The radio restarts afterwards.<BR>
    <INPUT TYPE="file" ID="file" ACCEPT=".json,application/json"><BR>
    <SMALL>Settings in the file replace the current ones. Wi-Fi networks are only restored if the file includes them.</SMALL></TD></TR>
  <TR><TD COLSPAN="2" CLASS="CENTER"><BUTTON TYPE="button" ID="imp">Import and restart</BUTTON></TD></TR>
  <TR><TD COLSPAN="2" CLASS="CENTER"><B ID="msg"></B></TD></TR>
</TABLE>
<SCRIPT>
(function () {
  var msg = document.getElementById('msg');
  document.getElementById('exp').onclick = function () {
    var s = document.getElementById('secrets').checked ? '1' : '0';
    window.location.href = '/backup.json?secrets=' + s;
  };
  document.getElementById('imp').onclick = function () {
    var f = document.getElementById('file').files[0];
    if (!f) { msg.textContent = 'Choose a backup file first.'; return; }
    if (f.size > 90000) { msg.textContent = 'File too large.'; return; }
    f.text().then(function (txt) {
      var j;
      try { j = JSON.parse(txt); } catch (e) { msg.textContent = 'Not a valid JSON file.'; return; }
      if (!j || j.app !== 'ats-mini-pv2ksm') { msg.textContent = 'This is not an ATS-Mini backup file.'; return; }
      if (!confirm('Replace the radio settings with this backup and restart?')) return;
      msg.textContent = 'Sending...';
      return fetch('/backup/import', { method: 'POST', headers: { 'Content-Type': 'text/plain' }, body: txt })
        .then(function (r) { return r.text(); })
        .then(function (t) { msg.textContent = t; });
    }).catch(function () { msg.textContent = 'Failed (the radio may be restarting).'; });
  };
})();
</SCRIPT>
)HTML";

#endif // PAGE_BACKUP_H
