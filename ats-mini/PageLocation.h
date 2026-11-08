#ifndef PAGE_LOCATION_H
#define PAGE_LOCATION_H

#include <Arduino.h>

static const char pageLocationTitle[] PROGMEM = "ATS-Mini Flights";

static const char pageLocation[] PROGMEM = R"HTML(<H1>{{title}}</H1>
{{{navigation}}}
<P CLASS="CENTER"><B>{{message}}</B></P>
<FORM ACTION="/location" METHOD="POST">
<TABLE COLUMNS="2">
  <TR><TH CLASS="HEADING" COLSPAN="2">Flights</TH></TR>
  <TR><TD CLASS="LABEL">Show Flights in the radio</TD>
      <TD><SELECT NAME="svcflights"><OPTION VALUE="1" {{on}}>Yes</OPTION><OPTION VALUE="0" {{off}}>No</OPTION></SELECT><BR>
      <SMALL>Choose No to hide Flights from the radio menu.</SMALL></TD></TR>
  <TR><TD CLASS="LABEL">Radar look</TD>
      <TD><SELECT NAME="radarstyle">
        <OPTION VALUE="0" {{rs0}}>Map</OPTION>
        <OPTION VALUE="1" {{rs1}}>A32X (airliner display)</OPTION>
        <OPTION VALUE="2" {{rs2}}>ATCAS (grey TCAS)</OPTION>
        <OPTION VALUE="3" {{rs3}}>BTCAS (bronze TCAS)</OPTION>
        <OPTION VALUE="4" {{rs4}}>B7X (737 display)</OPTION>
      </SELECT><BR><SMALL>The map shows the aircraft over a basemap. The cockpit looks draw the traffic like an airliner display and do not download a map.</SMALL></TD></TR>
  <TR><TH CLASS="HEADING" COLSPAN="2">Airport position (Flights, Map and METAR)</TH></TR>
  <TR><TD CLASS="LABEL">Airport ICAO or IATA</TD>
      <TD><INPUT TYPE="text" NAME="icao" VALUE="{{icao}}" MAXLENGTH="4" PLACEHOLDER="e.g. SBSJ or SJK"><BR>
      <SMALL>If filled, the airport position is used ({{count}} airports in the table) and the fields below are ignored.</SMALL></TD></TR>
  <TR><TD CLASS="LABEL">Latitude</TD><TD><INPUT TYPE="text" NAME="lat" VALUE="{{lat}}" INPUTMODE="decimal"></TD></TR>
  <TR><TD CLASS="LABEL">Longitude</TD><TD><INPUT TYPE="text" NAME="lon" VALUE="{{lon}}" INPUTMODE="decimal"></TD></TR>
  <TR><TD CLASS="LABEL">Name</TD><TD><INPUT TYPE="text" NAME="name" VALUE="{{name}}" MAXLENGTH="27"></TD></TR>
  <TR><TD CLASS="LABEL">Label</TD>
      <TD><INPUT TYPE="text" NAME="label" VALUE="{{label}}" MAXLENGTH="7"><BR>
      <SMALL>Short text drawn at your position on the radar, e.g. SJK.</SMALL></TD></TR>
  <TR><TD CLASS="LABEL">Map range (nm)</TD><TD><INPUT TYPE="number" NAME="radius" VALUE="{{radius}}" MIN="5" MAX="110"><BR><SMALL>Distance from your position to the top/bottom edge of the Flights screen. The width shows about 1.9x more.</SMALL></TD></TR>
  <TR><TH CLASS="HEADING" COLSPAN="2">Radar map (Flights)</TH></TR>
  <TR><TD CLASS="LABEL">Show map</TD>
      <TD><LABEL><INPUT TYPE="checkbox" NAME="mapon" VALUE="1" {{mapchecked}}> Draw the basemap behind the radar</LABEL></TD></TR>
  <TR><TD CLASS="LABEL">Map source</TD>
      <TD><SELECT NAME="mapsrc">
        <OPTION VALUE="1" {{srcosm}}>OpenStreetMap (default, no key needed)</OPTION>
        <OPTION VALUE="2" {{srccustom}}>Custom (Tile URL below)</OPTION>
      </SELECT><BR><SMALL>OpenStreetMap tiles are a free community service. Access may be blocked without notice if usage is heavy.</SMALL><BR>
      <A HREF="https://operations.osmfoundation.org/policies/tiles/">OpenStreetMap tile usage policy</A></TD></TR>
  <TR><TD CLASS="LABEL">Tile URL</TD>
      <TD><INPUT TYPE="text" NAME="mapurl" VALUE="{{mapurl}}" MAXLENGTH="159" AUTOCAPITALIZE="off" SPELLCHECK="false"><BR>
      <SMALL>Raster PNG template with {z}, {x}, {y}. Use {key} where a key goes (only for custom providers that need one). Clear the field to restore OpenStreetMap. The map is downloaded when the Flights screen opens (needs Wi-Fi).</SMALL></TD></TR>
  <TR><TD CLASS="LABEL">Key ({{keystate}})</TD>
      <TD><INPUT TYPE="password" NAME="ckey" VALUE="" AUTOCOMPLETE="off" PLACEHOLDER="leave empty to keep the current key"><BR>
      <LABEL><INPUT TYPE="checkbox" NAME="clearkey" VALUE="1"> Erase the stored key</LABEL><BR>
      <SMALL>Stored only on this radio and never shown again.</SMALL></TD></TR>
  <TR><TD COLSPAN="2" CLASS="CENTER"><INPUT TYPE="submit" VALUE="Save"></TD></TR>
</TABLE>
</FORM>
)HTML";

#endif // PAGE_LOCATION_H
