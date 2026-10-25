/* Standalone: gcc -O2 -o altcha_cli altcha_cli.c sha256.c altcha.c
 * Reads challenge JSON from stdin, prints base64 payload to stdout.
 * Exit 0 on success.
 */
#include "altcha.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void extract(const char *json, const char *key, char *out, int outsz)
{
    char pat[64];
    const char *p;
    int i = 0;
    snprintf(pat, sizeof(pat), "\"%s\":\"", key);
    p = strstr(json, pat);
    if (!p) { out[0] = 0; return; }
    p += strlen(pat);
    while (*p && *p != '"' && i < outsz - 1) {
        if (p[0] == '\\' && p[1] == 'u' && p[2] == '0' && p[3] == '0'
            && p[4] == '2' && p[5] == '6') {
            out[i++] = '&';
            p += 6;
            continue;
        }
        out[i++] = *p++;
    }
    out[i] = 0;
}

int main(void)
{
    char buf[2048];
    size_t n = fread(buf, 1, sizeof(buf) - 1, stdin);
    buf[n] = 0;

    char algo[32], challenge[128], salt[128], sig[128];
    unsigned int maxn = 100000;
    const char *pm;

    extract(buf, "algorithm", algo, sizeof(algo));
    extract(buf, "challenge", challenge, sizeof(challenge));
    extract(buf, "salt", salt, sizeof(salt));
    extract(buf, "signature", sig, sizeof(sig));
    pm = strstr(buf, "\"maxNumber\":");
    if (pm) maxn = (unsigned)atoi(pm + 12);
    if (!algo[0]) strcpy(algo, "SHA-256");

    unsigned int number = 0;
    if (!oh_altcha_solve(challenge, salt, maxn, &number, 0)) {
        fprintf(stderr, "solve failed\n");
        return 1;
    }

    char b64[1024];
    if (oh_altcha_make_payload_b64(algo, challenge, number, salt, sig, b64, sizeof(b64)) < 0) {
        fprintf(stderr, "payload failed\n");
        return 1;
    }
    printf("%s\n", b64);
    fprintf(stderr, "number=%u\n", number);
    return 0;
}
