#ifndef STATUS_CHIPS_H
#define STATUS_CHIPS_H

// Small "SQL 12" / "MUTE" chips shown only while squelch is on or the audio is muted.
// Each layout has its own spot where nothing else is drawn (see scDraw()).

#include "CockpitUI.h"

enum { SC_COCKPIT = 0, SC_ANALOG, SC_AVION, SC_NANALOG, SC_NSMETER, SC_VINTAGE, SC_CLOCK, SC_MEMORIES, SC_MINIMAL, SC_VFD, SC_LCD, SC_NEON };

struct ScLook { uint16_t acc, txt, dark; };

template<class G>
static void scChip(G &g, int &x, int y, bool right, const char *t, const ScLook &l, bool mute)
{
  const int w = (int)strlen(t) * 6 + 8, h = 11;
  const int cx = right ? x - w : x;
  if(mute)
  {
    g.fillRoundRect(cx, y, w, h, 3, 0xB800);
    ckText(g, FontB612Reg10, t, cx + w / 2, y + 6, 0xFFFF, 5);
  }
  else
  {
    g.fillRoundRect(cx, y, w, h, 3, l.dark);
    g.drawRoundRect(cx, y, w, h, 3, l.acc);
    ckText(g, FontB612Reg10, t, cx + w / 2, y + 6, l.txt, 5);
  }
  x = right ? cx - 3 : cx + w + 3;
}

// One chip pair. 'right' anchors the pair to x on its right edge (MUTE rightmost).
template<class G>
static void scPair(G &g, const CkIn &in, int x, int y, bool right, const ScLook &l)
{
  char q[12];
  snprintf(q, sizeof(q), "%s %d", in.sqlSnr ? "SQN" : "SQL", in.squelch);
  if(right)
  {
    if(in.muted) scChip(g, x, y, true, "MUTE", l, true);
    if(in.squelch) scChip(g, x, y, true, q, l, false);
  }
  else
  {
    if(in.squelch) scChip(g, x, y, false, q, l, false);
    if(in.muted) scChip(g, x, y, false, "MUTE", l, true);
  }
}

// pal* colours for the new layouts (acc/txt/bg1), NULL for the fixed-colour originals
template<class G>
static void scDraw(G &g, const CkIn &in, int style, uint16_t acc, uint16_t txt, uint16_t bg)
{
  if(!in.squelch && !in.muted) return;
  const uint16_t dark = ckMix(bg, 0x0000, 150);
  ScLook l = {acc, txt, dark};
  switch(style)
  {
    case SC_COCKPIT:
      l.acc = 0xFD80; l.txt = 0xFFFF; l.dark = 0x1082;
      scPair(g, in, 100, 4, false, l);
      break;
    case SC_ANALOG:
    {
      l.acc = 0x28E2; l.txt = 0x28E2; l.dark = 0xF75A;          // ink on cream, like the dials
      char q[12]; snprintf(q, sizeof(q), "%s %d", in.sqlSnr ? "SQN" : "SQL", in.squelch);
      int x = 174; if(in.squelch) scChip(g, x, 75, false, q, l, false);
      x = 312; if(in.muted) scChip(g, x, 75, true, "MUTE", l, true);
    }
      break;
    case SC_AVION:
      l.acc = 0xFD80; l.txt = 0xFFFF; l.dark = 0x2104;
    {
      // Right of the display frame, under the knob (the banner and the digits own everything else)
      char q[12]; snprintf(q, sizeof(q), "%s %d", in.sqlSnr ? "SQN" : "SQL", in.squelch);
      int x = 314; if(in.squelch) scChip(g, x, 84, true, q, l, false);
      x = 314; if(in.muted) scChip(g, x, in.squelch ? 97 : 84, true, "MUTE", l, true);
    }
      break;
    case SC_NANALOG:
    {
      char q[12]; snprintf(q, sizeof(q), "%s %d", in.sqlSnr ? "SQN" : "SQL", in.squelch);
      int x = 170; if(in.squelch) scChip(g, x, 128, false, q, l, false);
      x = 312; if(in.muted) scChip(g, x, 128, true, "MUTE", l, true);
    }
      break;
    case SC_NSMETER:  scPair(g, in, 316, 18, true, l); break;
    case SC_VINTAGE:  scPair(g, in, 236, 109, false, l); break;
    case SC_CLOCK:    scPair(g, in, 312, 18, true, l); break;
    case SC_MEMORIES: scPair(g, in, 316, 52, true, l); break;
    case SC_MINIMAL:  scPair(g, in, 312, 18, true, l); break;
    case SC_VFD:
    case SC_LCD:      scPair(g, in, 312, 17, true, l); break;
    case SC_NEON:     scPair(g, in, 310, 25, true, l); break;
  }
}

#endif
