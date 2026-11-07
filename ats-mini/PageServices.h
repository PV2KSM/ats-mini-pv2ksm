#ifndef PAGE_SERVICES_H
#define PAGE_SERVICES_H

#include <Arduino.h>

static const char pageServicesTitle[] PROGMEM = "ATS-Mini Weather";

static const char pageServices[] PROGMEM = R"HTML(<H1>{{title}}</H1>
{{{navigation}}}
<P CLASS="CENTER"><B>{{message}}</B></P>
<FORM ACTION="/services" METHOD="POST">
<TABLE COLUMNS="2">
  <TR><TH CLASS="HEADING" COLSPAN="2">Weather</TH></TR>
  <TR><TD CLASS="LABEL">Show Weather in the radio</TD>
      <TD><SELECT NAME="svcweather"><OPTION VALUE="1" {{on}}>Yes</OPTION><OPTION VALUE="0" {{off}}>No</OPTION></SELECT><BR>
      <SMALL>Choose No to hide Weather from the radio menu.</SMALL></TD></TR>
  <TR><TD CLASS="LABEL">City</TD>
      <TD><INPUT TYPE="text" NAME="wxcity" VALUE="{{wxcity}}" MAXLENGTH="39" PLACEHOLDER="e.g. Sao Paulo, BR"><BR>
      <SMALL>Weather and the rain radar use this city. Add a country code after a comma if needed. Leave empty to use the airport position of the Flights page. Found: {{wxfound}}</SMALL></TD></TR>
  <TR><TD CLASS="LABEL">Temperature unit</TD>
      <TD><SELECT NAME="tempunit">
        <OPTION VALUE="0" {{tempc}}>Celsius (C)</OPTION>
        <OPTION VALUE="1" {{tempf}}>Fahrenheit (F)</OPTION>
      </SELECT><BR><SMALL>Used on the Weather screen and in the weather cards of the radio.</SMALL></TD></TR>
  <TR><TD COLSPAN="2" CLASS="CENTER"><INPUT TYPE="submit" VALUE="Save"></TD></TR>
</TABLE>
</FORM>
)HTML";

#endif // PAGE_SERVICES_H
