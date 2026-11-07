#ifndef STATUS_CHIPS_H
#define STATUS_CHIPS_H

// One status label, as in the original UI: red "MUTED" while the audio is muted, otherwise red "SQ" while the
// squelch is silencing it. No squelch level and never both. The new layouts draw it top right (see
// scNew() / Layout-New.cpp); Cockpit, Analog and Avionics have their own free spot (see scDraw()).

#include "CockpitUI.h"

enum { SC_COCKPIT = 0, SC_ANALOG, SC_AVION, SC_NANALOG, SC_NSMETER, SC_VINTAGE, SC_CLOCK, SC_MEMORIES, SC_MINIMAL, SC_VFD, SC_LCD, SC_NEON };

// Red label. x,y = left (or right, if right=true) and top corner.
template<class G>
static void scLabel(G &g, const char *t, int x, int y, bool right = false)
{
  g.setFont(&FontB612Bold12);
  const int w = g.textWidth(t) + 10, h = 17;
  const int bx = right ? x - w : x;
  g.fillRoundRect(bx, y, w, h, 4, 0xB800);
  ckText(g, FontB612Bold12, t, bx + 5, y + 2, 0xFFFF);
}

// Layouts of Layout-New.cpp: top right under the battery wherever that spot is empty; the layouts built around
// a meter keep the label on a free patch of their own.
template<class G>
static void scNew(G &g, int style, const char *t)
{
  switch(style)
  {
    default:         scLabel(g, t, 316, 17, true); break;
  }
}

// Layouts drawn by their own code (not Layout-New.cpp)
template<class G>
static void scDraw(G &g, const CkIn &in, int style, uint16_t, uint16_t, uint16_t)
{
  const char *t = in.muted ? uiText("MUTED") : (in.sqActive ? "SQ" : nullptr);
  if(!t) return;
  switch(style)
  {
    case SC_COCKPIT: if(!in.muted) scLabel(g, t, 100, 2); break;      // MUTED already shows in the VOL window
    case SC_ANALOG:  scLabel(g, t, 316, 17, true); break;
    case SC_AVION:   scLabel(g, t, 314, 82, true); break;
    default: break;
  }
}

#endif
