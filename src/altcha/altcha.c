#include "altcha.h"
#include "sha256.h"

#include <stdio.h>
#include <string.h>

/* --- minimal base64 encode (no Qt) --- */
static int b64_encode(const unsigned char *src, int src_len, char *dst, int dst_size)
{
    static const char tbl[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i, o = 0;
    unsigned int v;

    if (dst_size < ((src_len + 2) / 3) * 4 + 1)
        return -1;

    for (i = 0; i < src_len; i += 3) {
        int left = src_len - i;
        v = ((unsigned int)src[i]) << 16;
        if (left > 1) v |= ((unsigned int)src[i + 1]) << 8;
        if (left > 2) v |= (unsigned int)src[i + 2];

        dst[o++] = tbl[(v >> 18) & 63];
        dst[o++] = tbl[(v >> 12) & 63];
        dst[o++] = (left > 1) ? tbl[(v >> 6) & 63] : '=';
        dst[o++] = (left > 2) ? tbl[v & 63] : '=';
    }
    dst[o] = '\0';
    return o;
}

/* fast path: hash salt||decimal without printf each time for small n */
static void u32_to_dec(unsigned int n, char *buf, int *len_out)
{
    char tmp[12];
    int i = 0, j;
    if (n == 0) {
        buf[0] = '0';
        *len_out = 1;
        return;
    }
    while (n > 0) {
        tmp[i++] = (char)('0' + (n % 10));
        n /= 10;
    }
    for (j = 0; j < i; ++j)
        buf[j] = tmp[i - 1 - j];
    *len_out = i;
}

int oh_altcha_solve(
    const char *challenge_hex,
    const char *salt,
    unsigned int max_number,
    unsigned int *out_number,
    volatile int *cancel_flag)
{
    unsigned int n;
    unsigned int salt_len;
    char numbuf[16];
    char hex[68];
    unsigned char block[512];
    int num_len;
    oh_sha256_ctx ctx;
    unsigned char hash[32];
    static const char hx[] = "0123456789abcdef";
    unsigned int i;

    if (!challenge_hex || !salt || !out_number)
        return 0;
    if (strlen(challenge_hex) < 64)
        return 0;

    salt_len = (unsigned int)strlen(salt);
    if (salt_len + 16 >= sizeof(block))
        return 0;

    memcpy(block, salt, salt_len);

    for (n = 0; n <= max_number; ++n) {
        if (cancel_flag && *cancel_flag)
            return 0;

        u32_to_dec(n, numbuf, &num_len);
        memcpy(block + salt_len, numbuf, (unsigned int)num_len);

        oh_sha256_init(&ctx);
        oh_sha256_update(&ctx, block, salt_len + (unsigned int)num_len);
        oh_sha256_final(&ctx, hash);

        for (i = 0; i < 32; ++i) {
            hex[i * 2]     = hx[(hash[i] >> 4) & 0xf];
            hex[i * 2 + 1] = hx[hash[i] & 0xf];
        }
        /* compare first 64 chars only */
        if (memcmp(hex, challenge_hex, 64) == 0) {
            *out_number = n;
            return 1;
        }
    }
    return 0;
}

int oh_altcha_make_payload_b64(
    const char *algorithm,
    const char *challenge_hex,
    unsigned int number,
    const char *salt,
    const char *signature_hex,
    char *out_b64,
    int out_b64_size)
{
    char json[768];
    int n;

    if (!algorithm || !challenge_hex || !salt || !signature_hex || !out_b64)
        return -1;

    /* Escape is not needed: hex and salt from server are safe ASCII for our case.
     * Salt may contain '&' — fine inside JSON string. */
    n = sprintf(json,
        "{\"algorithm\":\"%s\",\"challenge\":\"%s\",\"number\":%u,\"salt\":\"%s\",\"signature\":\"%s\"}",
        algorithm,
        challenge_hex,
        number,
        salt,
        signature_hex);

    if (n <= 0 || n >= (int)sizeof(json))
        return -1;

    return b64_encode((const unsigned char *)json, n, out_b64, out_b64_size);
}
