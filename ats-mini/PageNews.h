#ifndef PAGE_NEWS_H
#define PAGE_NEWS_H

#include <Arduino.h>

static const char pageNewsTitle[] PROGMEM = "ATS-Mini News";

// NOTE: this text is a template: never write two consecutive opening braces in the script.
static const char pageNews[] PROGMEM = R"HTML(<H1>{{title}}</H1>
{{{navigation}}}
<P CLASS="CENTER"><B>{{message}}</B></P>
<FORM ACTION="/news" METHOD="POST">
<TABLE COLUMNS="2">
  <TR><TH CLASS="HEADING" COLSPAN="2">News screen</TH></TR>
  <TR><TD CLASS="LABEL">Country</TD>
      <TD><SELECT NAME="ed">{{{editions}}}</SELECT><BR>
      <SMALL>Headlines come from the Google News edition of this country.</SMALL></TD></TR>
  <TR><TD CLASS="LABEL">City or subject (optional)</TD>
      <TD><INPUT TYPE="text" NAME="q" MAXLENGTH="47" VALUE="{{query}}"><BR>
      <SMALL>Adds the news about this city or subject, for example Campinas or solar energy.</SMALL></TD></TR>
  <TR><TD CLASS="LABEL">Topics</TD>
      <TD>{{{topics}}}</TD></TR>
  <TR><TD CLASS="LABEL">Your own site or feed (optional)</TD>
      <TD><INPUT TYPE="text" NAME="url" ID="nurl" MAXLENGTH="159" VALUE="{{url}}" PLACEHOLDER="https://...">
      <INPUT TYPE="button" ID="ntest" VALUE="Test"><BR>
      <DIV ID="nres"></DIV>
      <SMALL>Paste the address of a news site or of its RSS feed. The radio looks for the feed by itself.
      Each publisher sets the terms of use of its own feed. Leave empty to use only Google News.</SMALL></TD></TR>
  <TR><TD CLASS="LABEL">Update every</TD>
      <TD><SELECT NAME="ref">{{{refresh}}}</SELECT></TD></TR>
  <TR><TD CLASS="LABEL">Seconds per item</TD>
      <TD><SELECT NAME="sec">{{{seconds}}}</SELECT></TD></TR>
  <TR><TD CLASS="LABEL">Show</TD>
      <TD><LABEL><INPUT TYPE="checkbox" NAME="s1" VALUE="1"{{sum}}> Summary</LABEL>
          <LABEL><INPUT TYPE="checkbox" NAME="s2" VALUE="1"{{src}}> Source and age</LABEL></TD></TR>
  <TR><TD COLSPAN="2" CLASS="CENTER"><INPUT TYPE="submit" VALUE="Save"><BR>
      <SMALL>The News item of the radio menu shows the headlines. The radio needs Wi-Fi and the correct time.
      Google News feeds have no summary. The radio screen has no accents, so they are removed there.</SMALL></TD></TR>
</TABLE>
</FORM>
<H2 STYLE="font-size:16px;margin:18px 0 4px">Current news and QR code</H2>
<P><SMALL>Lists the items the radio holds now (open News on the radio first). Tap a headline to open the article. Last column: length of the article address; the QR code needs 1000 or less.</SMALL></P>
<INPUT TYPE="button" ID="nlist" VALUE="Refresh list">
<TABLE ID="nrows" STYLE="width:100%;border-collapse:collapse;font-size:12px;line-height:1.25;margin-top:6px"></TABLE>
<SCRIPT>
  var nr = document.getElementById('nres'), tries = 0;
  function esc(s) { var d = document.createElement('div'); d.textContent = s; return d.innerHTML; }
  function poll() {
    fetch('/api/news/test').then(function(r) { return r.json(); }).then(function(j) {
      if (j.state === 1 && tries++ < 60) { setTimeout(poll, 700); return; }
      if (j.state === 2) {
        nr.innerHTML = '<B>' + esc(j.msg) + '</B><BR>' + esc(j.head) + (j.sum ? '<BR><SMALL>' + esc(j.sum) + '</SMALL>' : '');
      } else {
        nr.innerHTML = '<B>' + esc(j.state === 1 ? (window.TR ? TR('Took too long. Try again.') : 'Took too long. Try again.') : j.msg) + '</B>';
      }
    }).catch(function() { nr.textContent = window.TR ? TR('Could not reach the radio.') : 'Could not reach the radio.'; });
  }
  document.getElementById('ntest').onclick = function() {
    tries = 0;
    nr.textContent = window.TR ? TR('Testing...') : 'Testing...';
    fetch('/api/news/test', { method: 'POST', body: new URLSearchParams({ url: document.getElementById('nurl').value }) })
      .then(function() { setTimeout(poll, 700); })
      .catch(function() { nr.textContent = window.TR ? TR('Could not reach the radio.') : 'Could not reach the radio.'; });
  };
  document.getElementById('nlist').onclick = function() {
    var t = document.getElementById('nrows');
    fetch('/api/news/list').then(function(r) { return r.json(); }).then(function(a) {
      var h = '';
      var td = 'style="padding:2px 4px;border-bottom:1px solid #2a3550;vertical-align:top"';
      a.forEach(function(x, i) {
        var s = x.h.length > 64 ? x.h.substring(0, 62) + '..' : x.h;
        h += '<TR><TD ' + td + '>' + (i + 1) + '</TD><TD ' + td + '>' + (x.q ? '<A HREF="/api/news/link?i=' + i + '" TARGET="_blank" STYLE="color:inherit">' + esc(s) + '</A>' : esc(s)) + '</TD><TD ' + td + ' STYLE="white-space:nowrap">' + x.n + (x.q ? ' \u2713' : ' \u2717') + '</TD></TR>';
      });
      t.innerHTML = h || '<TR><TD>-</TD></TR>';
    }).catch(function() { t.innerHTML = '<TR><TD>' + (window.TR ? TR('Could not reach the radio.') : 'Could not reach the radio.') + '</TD></TR>'; });
  };
  document.getElementById('nlist').onclick();
</SCRIPT>
)HTML";

#endif // PAGE_NEWS_H
