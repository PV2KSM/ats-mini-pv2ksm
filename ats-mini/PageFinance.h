#ifndef PAGE_FINANCE_H
#define PAGE_FINANCE_H

#include <Arduino.h>

static const char pageFinanceTitle[] PROGMEM = "ATS-Mini Finance";

static const char pageFinance[] PROGMEM = R"HTML(<H1>{{title}}</H1>
{{{navigation}}}
<P CLASS="CENTER"><B>{{message}}</B></P>
<FORM ACTION="/finance" METHOD="POST">
<TABLE COLUMNS="2">
  <TR><TH CLASS="HEADING" COLSPAN="2">Finance screen</TH></TR>
  <TR><TD CLASS="LABEL">Local currency</TD>
      <TD><SELECT NAME="currency">{{{options}}}</SELECT><BR>
      <SMALL>The Finance item of the radio menu shows the US dollar, the euro and Bitcoin converted to this currency.
      Rates come from Coinbase (Bitcoin) and the European Central Bank via Frankfurter (dollar and euro).
      The radio needs Wi-Fi and the correct time.</SMALL></TD></TR>
  <TR><TD COLSPAN="2" CLASS="CENTER"><INPUT TYPE="submit" VALUE="Save"></TD></TR>
</TABLE>
</FORM>
)HTML";

#endif // PAGE_FINANCE_H
