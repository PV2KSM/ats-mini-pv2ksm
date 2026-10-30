#include "NetFetch.h"
#include "Diag.h"
#include "Utils.h"
#include <Arduino.h>
#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <HTTPClient.h>
#include <string.h>

extern const uint8_t caBundleStart[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t caBundleEnd[] asm("_binary_x509_crt_bundle_end");

namespace {
class BufSink : public Stream
{
public:
  char *buf; size_t cap; size_t len = 0; bool overflow = false;
  BufSink(char *b, size_t c) : buf(b), cap(c) {}
  size_t write(uint8_t c) override { return write(&c, 1); }
  size_t write(const uint8_t *d, size_t n) override
  {
    if(len + n + 1 > cap) { overflow = true; return n; }
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

static int fetchImpl(const char *url, char *out, size_t cap, char *err, size_t errSize, const char *accept, bool takeLock)
{
  if(out && cap) out[0] = 0;
  if(err && errSize) err[0] = 0;
  auto fail = [&](const char *msg, int code) { if(err && errSize) snprintf(err, errSize, "%s", msg); return code; };

  if(WiFi.status() != WL_CONNECTED) return fail("Sem Wi-Fi", -100);
  if(!clockGetDate(nullptr, nullptr, nullptr, nullptr)) return fail("Sem hora (NTP)", -101);
  if(takeLock && !netLock(20000)) return fail("Rede ocupada", -102);

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
    if(!http.begin(client, url)) result = fail("Falha http.begin", -103);
    else
    {
      http.addHeader("Accept", accept ? accept : "*/*");
      int code = http.GET();
      if(code != HTTP_CODE_OK)
      {
        char msg[40];
        if(code > 0) snprintf(msg, sizeof(msg), "HTTP erro %d", code);
        else snprintf(msg, sizeof(msg), "Rede erro %d", code);
        http.end();
        result = fail(msg, code);
      }
      else
      {
        BufSink sink(out, cap);
        int wrote = http.writeToStream(&sink);
        http.end();
        if(sink.overflow) result = fail("Resposta grande", -104);
        else if(wrote < 0 || sink.len == 0) result = fail("Leitura falhou", -105);
        else result = 200;
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
