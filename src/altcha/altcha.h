/*
 * ALTCHA v1 solver — plain C, no Qt, no libc++.
 * Object Hub: SHA-256, maxNumber typically 100000.
 *
 * Protocol:
 *   hash = hex(SHA256(salt + decimal(n)))
 *   find n in [0..max_number] where hash == challenge
 */
#ifndef OH_ALTCHA_H
#define OH_ALTCHA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Returns 1 on success, 0 on failure.
 * challenge_hex: 64-char lowercase hex (may be longer buffer, null-terminated)
 * salt:          exact salt string from server (often ends with '&')
 * max_number:    from server (e.g. 100000)
 * out_number:    found n
 *
 * Optional: if cancel_flag != 0 and *cancel_flag becomes non-zero, aborts early → 0
 */
int oh_altcha_solve(
    const char *challenge_hex,
    const char *salt,
    unsigned int max_number,
    unsigned int *out_number,
    volatile int *cancel_flag
);

/*
 * Build JSON payload then base64 into out_b64.
 * out_b64_size: capacity of out_b64 (recommend >= 512)
 * Returns length of base64 string, or -1 on error.
 *
 * JSON fields match altcha-lib-go Payload:
 *   algorithm, challenge, number, salt, signature
 */
int oh_altcha_make_payload_b64(
    const char *algorithm,
    const char *challenge_hex,
    unsigned int number,
    const char *salt,
    const char *signature_hex,
    char *out_b64,
    int out_b64_size
);

#ifdef __cplusplus
}
#endif

#endif
