#pragma once
#include <stddef.h>

// Small HTTPS GET into a caller buffer (METAR, prices...). Shares the network lock with the
// flights, weather and map workers so only one TLS conversation runs at a time.
// Returns the HTTP status (200 = ok, text in out, NUL-terminated); anything else is a failure and
// err holds a short message in Portuguese.
// Same as netFetchText but the caller already holds netLock()
int netFetchLocked(const char *url, char *out, size_t cap, char *err, size_t errSize, const char *accept = "*/*");
int netFetchText(const char *url, char *out, size_t cap, char *err, size_t errSize, const char *accept = "*/*");

// Like netFetchText, but a response bigger than the buffer is cut (not an error); gotLen = bytes kept
int netFetchTextTrunc(const char *url, char *out, size_t cap, char *err, size_t errSize, size_t *gotLen, const char *accept = "*/*");
