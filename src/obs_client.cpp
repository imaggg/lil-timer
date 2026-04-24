#include "obs_client.h"
#include <WiFiClient.h>
#include <string.h>
#include <stdio.h>
#include "mbedtls/sha256.h"
#include "mbedtls/base64.h"

// --- WebSocket frame helpers ---

static void wsSendText(WiFiClient& client, const char* msg, size_t len) {
    uint8_t hdr[8];
    size_t hdrLen = 0;
    hdr[hdrLen++] = 0x81;
    if (len <= 125) {
        hdr[hdrLen++] = 0x80 | (uint8_t)len;
    } else {
        hdr[hdrLen++] = 0x80 | 126;
        hdr[hdrLen++] = (uint8_t)(len >> 8);
        hdr[hdrLen++] = (uint8_t)(len & 0xFF);
    }
    const uint8_t mask[4] = {0x37, 0xFA, 0x21, 0x3D};
    hdr[hdrLen++] = mask[0];
    hdr[hdrLen++] = mask[1];
    hdr[hdrLen++] = mask[2];
    hdr[hdrLen++] = mask[3];
    client.write(hdr, hdrLen);
    for (size_t i = 0; i < len; i++) {
        client.write((uint8_t)((uint8_t)msg[i] ^ mask[i & 3]));
    }
}

static int wsReadFrame(WiFiClient& client, char* buf, size_t bufLen, uint32_t timeoutMs) {
    uint32_t deadline = millis() + timeoutMs;

    auto waitN = [&](int n) -> bool {
        while (client.available() < n) {
            if ((uint32_t)millis() > deadline) return false;
            delay(5);
        }
        return true;
    };

    if (!waitN(2)) return -1;
    client.read();
    uint8_t b1 = client.read();
    bool masked = (b1 & 0x80) != 0;
    size_t payLen = b1 & 0x7F;

    if (payLen == 126) {
        if (!waitN(2)) return -1;
        payLen = ((size_t)client.read() << 8) | client.read();
    } else if (payLen == 127) {
        for (int i = 0; i < 8; i++) {
            if (!waitN(1)) return -1;
            client.read();
        }
        return -1;
    }

    if (masked) {
        if (!waitN(4)) return -1;
        for (int i = 0; i < 4; i++) client.read();
    }

    size_t toStore = (payLen < bufLen - 1) ? payLen : bufLen - 1;
    size_t idx = 0;
    while (idx < payLen) {
        if ((uint32_t)millis() > deadline) break;
        if (client.available()) {
            uint8_t b = client.read();
            if (idx < toStore) buf[idx] = (char)b;
            idx++;
        } else {
            delay(5);
        }
    }
    size_t stored = idx < toStore ? idx : toStore;
    buf[stored] = '\0';
    return (int)stored;
}

// --- Auth helpers ---

// SHA-256 of data, result written to out[32]
static void sha256Digest(const uint8_t* data, size_t len, uint8_t out[32]) {
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, data, len);
    mbedtls_sha256_finish(&ctx, out);
    mbedtls_sha256_free(&ctx);
}

// base64(sha256(a + b)) written to out (needs at least 45 bytes)
static bool sha256Base64(const char* a, size_t alen,
                         const char* b, size_t blen,
                         char* out, size_t outSize) {
    // concat into temporary buffer
    size_t total = alen + blen;
    if (total > 256) return false;
    uint8_t tmp[256];
    memcpy(tmp, a, alen);
    memcpy(tmp + alen, b, blen);

    uint8_t hash[32];
    sha256Digest(tmp, total, hash);

    size_t olen = 0;
    int rc = mbedtls_base64_encode(
        (unsigned char*)out, outSize, &olen, hash, 32);
    if (rc != 0) return false;
    out[olen] = '\0';
    return true;
}

// Extract a JSON string value: looks for "key":"<value>"
static bool jsonExtractString(const char* json, const char* key,
                              char* out, size_t outSize) {
    char search[48];
    snprintf(search, sizeof(search), "\"%s\":\"", key);
    const char* pos = strstr(json, search);
    if (!pos) return false;
    pos += strlen(search);
    const char* end = strchr(pos, '"');
    if (!end) return false;
    size_t len = (size_t)(end - pos);
    if (len >= outSize) return false;
    memcpy(out, pos, len);
    out[len] = '\0';
    return true;
}

// --- Public API ---

bool obsSetScene(const char* host, uint16_t port,
                 const char* password, const char* scene_name) {
    if (!host || host[0] == '\0' || !scene_name || scene_name[0] == '\0')
        return false;

    WiFiClient client;
    if (!client.connect(host, port)) return false;

    // WebSocket upgrade
    char req[256];
    snprintf(req, sizeof(req),
        "GET / HTTP/1.1\r\nHost: %s:%u\r\n"
        "Upgrade: websocket\r\nConnection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n",
        host, (unsigned)port);
    client.print(req);

    // Read HTTP 101
    uint32_t deadline = millis() + 3000;
    char hbuf[256];
    size_t hlen = 0;
    bool got101 = false;
    while ((uint32_t)millis() < deadline) {
        if (client.available()) {
            char c = (char)client.read();
            if (hlen < sizeof(hbuf) - 1) hbuf[hlen++] = c;
            if (hlen >= 4 &&
                hbuf[hlen-4]=='\r' && hbuf[hlen-3]=='\n' &&
                hbuf[hlen-2]=='\r' && hbuf[hlen-1]=='\n') {
                hbuf[hlen] = '\0';
                got101 = (strstr(hbuf, "101") != nullptr);
                break;
            }
        } else delay(5);
    }
    if (!got101) { client.stop(); return false; }

    // Read Hello (op:0)
    char frame[640];
    if (wsReadFrame(client, frame, sizeof(frame), 3000) < 0) {
        client.stop(); return false;
    }

    // Build Identify payload — with or without authentication
    char identify[256];
    bool needsAuth = (password && password[0] != '\0') &&
                     (strstr(frame, "\"authentication\":{") != nullptr);

    if (needsAuth) {
        char challenge[128] = {0};
        char salt[128] = {0};
        if (!jsonExtractString(frame, "challenge", challenge, sizeof(challenge)) ||
            !jsonExtractString(frame, "salt",      salt,      sizeof(salt))) {
            client.stop(); return false;
        }

        // secret = base64(sha256(password + salt))
        char secret[48];
        if (!sha256Base64(password, strlen(password),
                          salt, strlen(salt),
                          secret, sizeof(secret))) {
            client.stop(); return false;
        }

        // auth = base64(sha256(secret + challenge))
        char auth[48];
        if (!sha256Base64(secret, strlen(secret),
                          challenge, strlen(challenge),
                          auth, sizeof(auth))) {
            client.stop(); return false;
        }

        snprintf(identify, sizeof(identify),
            "{\"op\":1,\"d\":{\"rpcVersion\":1,\"authentication\":\"%s\"}}",
            auth);
    } else {
        snprintf(identify, sizeof(identify),
            "{\"op\":1,\"d\":{\"rpcVersion\":1}}");
    }

    wsSendText(client, identify, strlen(identify));

    // Read Identified (op:2)
    if (wsReadFrame(client, frame, sizeof(frame), 3000) < 0) {
        client.stop(); return false;
    }

    // Send SetCurrentProgramScene (op:6)
    char setScene[320];
    snprintf(setScene, sizeof(setScene),
        "{\"op\":6,\"d\":{\"requestType\":\"SetCurrentProgramScene\","
        "\"requestId\":\"x\",\"requestData\":{\"sceneName\":\"%s\"}}}",
        scene_name);
    wsSendText(client, setScene, strlen(setScene));

    delay(100);
    client.stop();
    return true;
}
