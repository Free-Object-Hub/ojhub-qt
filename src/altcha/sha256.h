/* Public-domain style compact SHA-256 — C89-friendly */
#ifndef OH_SHA256_H
#define OH_SHA256_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct oh_sha256_ctx {
    unsigned int state[8];
    unsigned long long bitlen;
    unsigned char data[64];
    unsigned int datalen;
} oh_sha256_ctx;

void oh_sha256_init(oh_sha256_ctx *ctx);
void oh_sha256_update(oh_sha256_ctx *ctx, const unsigned char *data, unsigned int len);
void oh_sha256_final(oh_sha256_ctx *ctx, unsigned char hash[32]);

/* hex lowercase, out must be >= 65 bytes */
void oh_sha256_hex(const unsigned char *data, unsigned int len, char *out_hex);

#ifdef __cplusplus
}
#endif

#endif
