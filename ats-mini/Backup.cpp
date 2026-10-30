#include "Backup.h"
#include "Common.h"
#include "Storage.h"
#include "Utils.h"
#include <nvs.h>
#include <nvs_flash.h>
#include <ArduinoJson.h>
#include <mbedtls/base64.h>
#include <esp_heap_caps.h>
#include <string.h>

namespace {
struct NsSpec { const char *part; const char *ns; bool secret; };
const NsSpec specs[] =
{
  { STORAGE_PARTITION, "settings", false },
  { STORAGE_PARTITION, "memories", false },
  { STORAGE_PARTITION, "bands",    false },
  { STORAGE_PARTITION, "network",  true  },   // Wi-Fi networks and the web login
  { "nvs",             "location", false },   // Position, map options (the key "ckey" is a secret)
  { "nvs",             "finance",  false },
};

const NsSpec *findSpec(const char *part, const char *ns)
{
  for(const NsSpec &s : specs) if(!strcmp(s.part, part) && !strcmp(s.ns, ns)) return &s;
  return nullptr;
}

bool isSecretKey(const NsSpec &s, const char *key) { return s.secret || (!strcmp(s.ns, "location") && !strcmp(key, "ckey")); }

const char *typeName(nvs_type_t t)
{
  switch(t)
  {
    case NVS_TYPE_U8: return "u8";   case NVS_TYPE_I8: return "i8";
    case NVS_TYPE_U16: return "u16"; case NVS_TYPE_I16: return "i16";
    case NVS_TYPE_U32: return "u32"; case NVS_TYPE_I32: return "i32";
    case NVS_TYPE_U64: return "u64"; case NVS_TYPE_I64: return "i64";
    case NVS_TYPE_STR: return "str"; case NVS_TYPE_BLOB: return "blob";
    default: return nullptr;
  }
}

void jsonEscape(String &out, const char *s)
{
  for( ; *s ; s++)
  {
    const unsigned char c = *s;
    if(c == '"' || c == '\\') { out += '\\'; out += (char)c; }
    else if(c < 32) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); out += b; }
    else out += (char)c;
  }
}

String base64Of(const uint8_t *d, size_t n)
{
  size_t olen = 0;
  mbedtls_base64_encode(nullptr, 0, &olen, d, n);
  String s;
  s.reserve(olen + 1);
  uint8_t *tmp = (uint8_t *)malloc(olen + 1);
  if(!tmp) return s;
  if(!mbedtls_base64_encode(tmp, olen + 1, &olen, d, n)) { tmp[olen] = 0; s = (const char *)tmp; }
  free(tmp);
  return s;
}
}

bool backupExport(String &out, bool includeSecrets)
{
  out = "";
  out.reserve(16384);
  out += "{\"app\":\"ats-mini-pv2ksm\",\"version\":1,\"fw\":";
  out += String(VER_APP);
  out += ",\"secrets\":";
  out += includeSecrets ? "true" : "false";
  out += ",\"items\":[";
  bool first = true;

  for(const NsSpec &spec : specs)
  {
    if(spec.secret && !includeSecrets) continue;
    nvs_handle_t h;
    if(nvs_open_from_partition(spec.part, spec.ns, NVS_READONLY, &h) != ESP_OK) continue;
    nvs_iterator_t it = nullptr;
    esp_err_t r = nvs_entry_find(spec.part, spec.ns, NVS_TYPE_ANY, &it);
    while(r == ESP_OK && it)
    {
      nvs_entry_info_t info;
      nvs_entry_info(it, &info);
      const char *tn = typeName(info.type);
      if(tn && (includeSecrets || !isSecretKey(spec, info.key)))
      {
        String item = "{\"p\":\"";
        item += spec.part; item += "\",\"n\":\""; item += spec.ns; item += "\",\"k\":\""; item += info.key;
        item += "\",\"t\":\""; item += tn; item += "\",\"v\":";
        bool ok = true;
        switch(info.type)
        {
          case NVS_TYPE_U8:  { uint8_t v;  ok = nvs_get_u8(h, info.key, &v) == ESP_OK;  if(ok) item += String((unsigned)v); break; }
          case NVS_TYPE_I8:  { int8_t v;   ok = nvs_get_i8(h, info.key, &v) == ESP_OK;  if(ok) item += String((int)v); break; }
          case NVS_TYPE_U16: { uint16_t v; ok = nvs_get_u16(h, info.key, &v) == ESP_OK; if(ok) item += String((unsigned)v); break; }
          case NVS_TYPE_I16: { int16_t v;  ok = nvs_get_i16(h, info.key, &v) == ESP_OK; if(ok) item += String((int)v); break; }
          case NVS_TYPE_U32: { uint32_t v; ok = nvs_get_u32(h, info.key, &v) == ESP_OK; if(ok) item += String((unsigned long)v); break; }
          case NVS_TYPE_I32: { int32_t v;  ok = nvs_get_i32(h, info.key, &v) == ESP_OK; if(ok) item += String((long)v); break; }
          case NVS_TYPE_U64: { uint64_t v; ok = nvs_get_u64(h, info.key, &v) == ESP_OK; if(ok) { item += '"'; item += String((unsigned long long)v); item += '"'; } break; }
          case NVS_TYPE_I64: { int64_t v;  ok = nvs_get_i64(h, info.key, &v) == ESP_OK; if(ok) { item += '"'; item += String((long long)v); item += '"'; } break; }
          case NVS_TYPE_STR:
          {
            size_t len = 0;
            ok = nvs_get_str(h, info.key, nullptr, &len) == ESP_OK && len > 0 && len <= 512;
            if(ok)
            {
              char *buf = (char *)malloc(len);
              if(buf && nvs_get_str(h, info.key, buf, &len) == ESP_OK) { item += '"'; jsonEscape(item, buf); item += '"'; }
              else ok = false;
              free(buf);
            }
            break;
          }
          case NVS_TYPE_BLOB:
          {
            size_t len = 0;
            ok = nvs_get_blob(h, info.key, nullptr, &len) == ESP_OK && len > 0 && len <= 2048;
            if(ok)
            {
              uint8_t *buf = (uint8_t *)malloc(len);
              if(buf && nvs_get_blob(h, info.key, buf, &len) == ESP_OK) { item += '"'; item += base64Of(buf, len); item += '"'; }
              else ok = false;
              free(buf);
            }
            break;
          }
          default: ok = false;
        }
        if(ok)
        {
          item += '}';
          if(!first) out += ',';
          out += item;
          first = false;
        }
      }
      r = nvs_entry_next(&it);
    }
    if(it) nvs_release_iterator(it);
    nvs_close(h);
  }
  out += "]}";
  return true;
}

static bool writeItem(JsonObjectConst o, const char *&why)
{
  const char *part = o["p"] | "";
  const char *ns = o["n"] | "";
  const char *key = o["k"] | "";
  const char *type = o["t"] | "";
  const NsSpec *spec = findSpec(part, ns);
  if(!spec) { why = "namespace not allowed"; return false; }
  if(!*key || strlen(key) > 15) { why = "invalid key"; return false; }

  nvs_handle_t h;
  if(nvs_open_from_partition(part, ns, NVS_READWRITE, &h) != ESP_OK) { why = "cannot open storage"; return false; }
  esp_err_t e = ESP_ERR_INVALID_ARG;
  JsonVariantConst v = o["v"];
  if(!strcmp(type, "u8"))       e = nvs_set_u8(h, key, (uint8_t)(v.as<unsigned>()));
  else if(!strcmp(type, "i8"))  e = nvs_set_i8(h, key, (int8_t)(v.as<int>()));
  else if(!strcmp(type, "u16")) e = nvs_set_u16(h, key, (uint16_t)(v.as<unsigned>()));
  else if(!strcmp(type, "i16")) e = nvs_set_i16(h, key, (int16_t)(v.as<int>()));
  else if(!strcmp(type, "u32")) e = nvs_set_u32(h, key, (uint32_t)(v.as<unsigned long>()));
  else if(!strcmp(type, "i32")) e = nvs_set_i32(h, key, (int32_t)(v.as<long>()));
  else if(!strcmp(type, "u64")) e = nvs_set_u64(h, key, strtoull(v.as<const char *>() ? v.as<const char *>() : "0", nullptr, 10));
  else if(!strcmp(type, "i64")) e = nvs_set_i64(h, key, strtoll(v.as<const char *>() ? v.as<const char *>() : "0", nullptr, 10));
  else if(!strcmp(type, "str"))
  {
    const char *s = v.as<const char *>();
    if(s && strlen(s) <= 512) e = nvs_set_str(h, key, s);
  }
  else if(!strcmp(type, "blob"))
  {
    const char *s = v.as<const char *>();
    if(s)
    {
      const size_t slen = strlen(s);
      uint8_t *buf = (uint8_t *)malloc(slen);
      size_t olen = 0;
      if(buf && slen <= 3000 && !mbedtls_base64_decode(buf, slen, &olen, (const unsigned char *)s, slen) && olen > 0 && olen <= 2048)
        e = nvs_set_blob(h, key, buf, olen);
      free(buf);
    }
  }
  if(e == ESP_OK) e = nvs_commit(h);
  nvs_close(h);
  if(e != ESP_OK) { why = "write failed"; return false; }
  return true;
}

int backupImport(const char *json, size_t len, char *msg, size_t msgSize)
{
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json, len);
  if(err) { snprintf(msg, msgSize, "Not a valid JSON file."); return -1; }
  if(strcmp(doc["app"] | "", "ats-mini-pv2ksm")) { snprintf(msg, msgSize, "This is not an ATS-Mini backup file."); return -1; }
  JsonArrayConst items = doc["items"];
  if(items.isNull() || items.size() == 0 || items.size() > 4000) { snprintf(msg, msgSize, "The file has no valid items."); return -1; }

  int ok = 0, bad = 0;
  const char *why = "";
  for(JsonObjectConst o : items)
  {
    if(writeItem(o, why)) ok++; else bad++;
  }
  if(!ok) { snprintf(msg, msgSize, "Nothing could be imported (%s).", why); return -1; }
  snprintf(msg, msgSize, "%d items imported%s. The radio restarts now.", ok, bad ? " (some were skipped)" : "");
  return ok;
}

static uint32_t restartAt = 0;
void backupRestartSoon() { restartAt = millis() + 2000; if(!restartAt) restartAt = 1; }
void backupTick()
{
  if(restartAt && (int32_t)(millis() - restartAt) >= 0) { restartAt = 0; ESP.restart(); }
}
