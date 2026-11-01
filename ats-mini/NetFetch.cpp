#include "NetFetch.h"
#include "Diag.h"
#include "Utils.h"
#include "I18n.h"
#include <Arduino.h>
#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <HTTPClient.h>
#include <string.h>
#include <esp_heap_caps.h>

extern const uint8_t caBundleStart[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t caBundleEnd[] asm("_binary_x509_crt_bundle_end");

namespace {
class BufSink : public Stream
{
public:
  char *buf; size_t cap; size_t len = 0; bool overflow = false; bool trunc; bool cut = false;
  BufSink(char *b, size_t c, bool t = false) : buf(b), cap(c), trunc(t) {}
  size_t write(uint8_t c) override { return write(&c, 1); }
  size_t write(const uint8_t *d, size_t n) override
  {
    if(len + n + 1 > cap)
    {
      if(trunc) { cut = true; return n; }   // Keep what fits, drop the rest (feeds, page heads)
      overflow = true; return n;
    }
    memcpy(buf + len, d, n);
    len += n;
    buf[len] = 0;
    return n;
  }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
};
}

static int fetchImpl(const char *url, char *out, size_t cap, char *err, size_t errSize, const char *accept, bool takeLock, bool trunc = false, size_t *gotLen = nullptr)
{
  if(out && cap) out[0] = 0;
  if(err && errSize) err[0] = 0;
  auto fail = [&](const char *msg, int code) { if(err && errSize) snprintf(err, errSize, "%s", msg); return code; };

  if(WiFi.status() != WL_CONNECTED) return fail(tr(T_NO_WIFI), -100);
  if(!clockGetDate(nullptr, nullptr, nullptr, nullptr)) return fail(tr(T_NO_TIME), -101);
  if(takeLock && !netLock(20000)) { char m[48]; snprintf(m, sizeof(m), tr(T_NET_ERR), -102); return fail(m, -102); }

  int result;
  {
    NetworkClientSecure client;
    client.setCACertBundle(caBundleStart, caBundleEnd - caBundleStart);
    client.setHandshakeTimeout(10);
    HTTPClient http;
    http.setConnectTimeout(10000);
    http.setTimeout(15000);
    http.setUserAgent(getUserAgent());
    http.setReuse(false);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    if(!http.begin(client, url)) { char m[48]; snprintf(m, sizeof(m), tr(T_NET_ERR), -103); result = fail(m, -103); }
    else
    {
      http.addHeader("Accept", accept ? accept : "*/*");
      int code = http.GET();
      if(code != HTTP_CODE_OK)
      {
        char msg[80];
        if(code > 0) snprintf(msg, sizeof(msg), tr(T_HTTP_ERR), code);
        else
        {
          // Connection failures: add the TLS error and the free internal memory (to tell memory from certificates)
          char tls[40] = "";
          const int te = client.lastError(tls, sizeof(tls));
          const int l0 = snprintf(msg, sizeof(msg), tr(T_NET_ERR), code);
          snprintf(msg + l0, sizeof(msg) - l0, " (tls %d, heap %uk/%uk)", te,
                   (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024), (unsigned)(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) / 1024));
        }
        http.end();
        result = fail(msg, code);
      }
      else
      {
        BufSink sink(out, cap, trunc);
        int wrote = http.writeToStream(&sink);
        http.end();
        if(sink.overflow) result = fail(tr(T_BIG_RESP), -104);
        else if(wrote < 0 || sink.len == 0) result = fail(tr(T_READ_FAIL), -105);
        else { result = 200; if(gotLen) *gotLen = sink.len; }
      }
    }
  }
  if(takeLock) netUnlock();
  return result;
}

int netFetchText(const char *url, char *out, size_t cap, char *err, size_t errSize, const char *accept)
{
  return fetchImpl(url, out, cap, err, errSize, accept, true);
}

int netFetchLocked(const char *url, char *out, size_t cap, char *err, size_t errSize, const char *accept)
{
  return fetchImpl(url, out, cap, err, errSize, accept, false);
}

// Same, but a response bigger than the buffer is cut instead of failing (RSS feeds, page heads)
int netFetchTextTrunc(const char *url, char *out, size_t cap, char *err, size_t errSize, size_t *gotLen, const char *accept)
{
  if(gotLen) *gotLen = 0;
  return fetchImpl(url, out, cap, err, errSize, accept, true, true, gotLen);
}
