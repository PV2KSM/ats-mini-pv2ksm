#ifndef PAGE_DMR_H
#define PAGE_DMR_H

#include <Arduino.h>

static const char pageDmrTitle[] PROGMEM = "ATS-Mini ID DMR";

static const char pageDmr[] PROGMEM = R"HTML(<H1>{{title}}</H1>
{{{navigation}}}
<P CLASS="CENTER"><B>{{message}}</B></P>
<FORM ACTION="/dmr" METHOD="POST">
<TABLE COLUMNS="2">
  <TR><TH CLASS="HEADING" COLSPAN="2">ID DMR</TH></TR>
  <TR><TD CLASS="LABEL">Show DMR in the radio</TD>
      <TD><SELECT NAME="svcdmr"><OPTION VALUE="1" {{on}}>Yes</OPTION><OPTION VALUE="0" {{off}}>No</OPTION></SELECT><BR>
      <SMALL>Choose No to hide DMR from the radio menu.</SMALL></TD></TR>
  <TR><TD CLASS="LABEL">Look on the radio</TD>
      <TD><SELECT NAME="dmrtheme">
        <OPTION VALUE="0" {{th0}}>Classic</OPTION>
        <OPTION VALUE="1" {{th1}}>Handheld</OPTION>
        <OPTION VALUE="2" {{th2}}>Pi-Star</OPTION>
        <OPTION VALUE="3" {{th3}}>Phosphor</OPTION>
        <OPTION VALUE="4" {{th4}}>Minimal</OPTION>
        <OPTION VALUE="5" {{th5}}>Flag Hero</OPTION>
        <OPTION VALUE="6" {{th6}}>Badge</OPTION>
        <OPTION VALUE="7" {{th7}}>Rows</OPTION>
        <OPTION VALUE="8" {{th8}}>Paper</OPTION>
      </SELECT><BR><SMALL>Also in the radio menu: Visual, DMR theme.</SMALL></TD></TR>
  <TR><TD CLASS="LABEL">Lines in the list</TD>
      <TD><SELECT NAME="dmrrows">
        <OPTION VALUE="4" {{rows4}}>4</OPTION><OPTION VALUE="5" {{rows5}}>5</OPTION><OPTION VALUE="6" {{rows6}}>6</OPTION>
        <OPTION VALUE="7" {{rows7}}>7</OPTION><OPTION VALUE="8" {{rows8}}>8</OPTION>
      </SELECT></TD></TR>
  <TR><TD CLASS="LABEL">Hotspot address</TD>
      <TD><INPUT TYPE="text" NAME="dmrhost" VALUE="{{host}}" MAXLENGTH="31" PLACEHOLDER="pi-star.local or 192.168.1.50"><BR>
      <SMALL>IP of your hotspot on the same Wi-Fi (Pi-Star or WPSD), for example 192.168.0.45. A name such as pi-star.local also works if your router resolves it.</SMALL></TD></TR>
  <TR><TD COLSPAN="2" CLASS="CENTER"><INPUT TYPE="submit" VALUE="Save"></TD></TR>
</TABLE>
</FORM>
)HTML";

#endif // PAGE_DMR_H
