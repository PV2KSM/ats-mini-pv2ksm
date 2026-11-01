#pragma once
#include "Common.h"

// News screen: headlines and a short summary from RSS/Atom feeds.
//   Default source: the news feed of a country edition (and topics, city or subject) of Google News.
//   Optional: the address of any news site or feed; the radio looks for the feed by itself.
// Settings come from the web page (News tab).

void newsBegin();                         // Create the worker task (call once, from setup)
void newsOpen();                          // Called when the News screen is opened
bool newsIsActive();                      // True while the News screen is shown
bool newsTick();                          // Main loop: apply queued settings, rotate items; true when a redraw is needed
void newsDraw(LGFX_Sprite &spr);
void newsScroll(int16_t delta);           // Encoder: previous / next item
bool newsClick();
bool newsQrOpen();                        // Long press: show the QR code of the article address (false = no address)
void newsQrClose();
bool newsQrActive();                         // Short press: pause / resume the rotation (true = handled)

#define NEWS_TOPICS 9                     // Bits of the topic mask, see newsTopicName()
struct NewsCfg
{
  uint8_t edition;                        // Index in the edition list
  uint16_t topics;                        // Bit mask of the topics
  char query[48];                         // City or subject (adds a search feed)
  char url[160];                          // Own site or feed address (adds a source)
  uint8_t refreshMin;                     // 15, 30 or 60
  uint8_t secs;                           // Seconds each item stays on screen (6..30)
  uint8_t show;                           // bit0 summary, bit1 source and age
};

void newsGetCfg(NewsCfg *out);
const char *newsSetCfg(const NewsCfg &cfg);   // nullptr on success, else an error message (English)
int newsEditionCount();
const char *newsEditionName(int i);            // English name (web page, translated there)
const char *newsEditionLabel(int i);           // Name on the radio screen, in the device language
const char *newsTopicName(int i);              // English label, i = 0..NEWS_TOPICS-1

// Test button of the web page (runs in the worker task, the page polls the result)
void newsRequestTest(const char *url);         // Empty url = test the Google News settings
// state: 0 idle, 1 running, 2 ok, 3 failed
int newsTestResult(char *msg, size_t msgSize, char *head, size_t headSize, char *sum, size_t sumSize, int *count);

bool newsListItem(int i, char *head, size_t headSize, char *src, size_t srcSize, int *linkLen, bool *qrOk);   // false = no such item
bool newsLinkItem(int i, char *out, size_t size);   // Article address of item i, false when none
