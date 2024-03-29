/* ---------------------------------------------------------------- *\

  file    : auth_md5.c
  author  : m. gumz <akira at fluxbox dot org>
  copyr   : copyright (c) 2005 - 2007 by m. gumz

  license : based on: openbsd md5.c/h

    This code implements the MD5 message-digest algorithm.
    The algorithm is due to Ron Rivest.  This code was
    written by Colin Plumb in 1993, no copyright is claimed.
    This code is in the public domain; do with it what you wish.

    Equivalent code is available from RSA Data Security, Inc.
    This code has been tested against that, and is equivalent,
    except that you don't need to include two pages of legalese
    with every copy.

  start   : Sa 07 Mai 2005 13:21:45 CEST

\* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- *\

  about :

    provide -auth md5:hash=<hash>,file=<filename>

\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
  includes
\* ---------------------------------------------------------------- */

#ifndef STAND_ALONE
#   include "alock.h"
#endif /* STAND_ALONE */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

enum {
    MD5_BLOCK_LENGTH = 64,
    MD5_DIGEST_LENGTH = 16,
    MD5_DIGEST_STRING_LENGTH = (MD5_DIGEST_LENGTH * 2 + 1)
};


typedef struct {
    u_int32_t state[4];                 /* state */
    u_int64_t count;                    /* number of bits, mod 2^64 */
    u_int8_t buffer[MD5_BLOCK_LENGTH];  /* input buffer */
} md5Context;

static void md5_init(md5Context*);
static void md5_update(md5Context*, const u_int8_t*, size_t);
static void md5_pad(md5Context*);
static void md5_final(u_int8_t [MD5_DIGEST_LENGTH], md5Context*);
static void md5_transform(u_int32_t [4], const u_int8_t [MD5_BLOCK_LENGTH]);

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

#define PUT_64BIT_LE(cp, value) do {  \
    (cp)[7] = (value) >> 56;          \
    (cp)[6] = (value) >> 48;          \
    (cp)[5] = (value) >> 40;          \
    (cp)[4] = (value) >> 32;          \
    (cp)[3] = (value) >> 24;          \
    (cp)[2] = (value) >> 16;          \
    (cp)[1] = (value) >> 8;           \
    (cp)[0] = (value); } while (0)

#define PUT_32BIT_LE(cp, value) do {  \
    (cp)[3] = (value) >> 24;          \
    (cp)[2] = (value) >> 16;          \
    (cp)[1] = (value) >> 8;           \
    (cp)[0] = (value); } while (0)

static u_int8_t PADDING[MD5_BLOCK_LENGTH] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*------------------------------------------------------------------*\
   Start MD5 accumulation.  Set bit count to 0 and buffer to
   mysterious initialization constants.
\*------------------------------------------------------------------*/
static void md5_init(md5Context* ctx) {
    ctx->count = 0;
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
}
/*------------------------------------------------------------------*\
   Update context to reflect the concatenation of another buffer
   full of bytes.
\*------------------------------------------------------------------*/
static void md5_update(md5Context* ctx, const unsigned char* input, size_t len) {

    size_t have, need;

    /* Check how many bytes we already have and how many more we need. */
    have = (size_t)((ctx->count >> 3) & (MD5_BLOCK_LENGTH - 1));
    need = MD5_BLOCK_LENGTH - have;

    /* Update bitcount */
    ctx->count += (u_int64_t)len << 3;

    if (len >= need) {
        if (have != 0) {
            memcpy(ctx->buffer + have, input, need);
            md5_transform(ctx->state, ctx->buffer);
            input += need;
            len -= need;
            have = 0;
        }

        /* Process data in MD5_BLOCK_LENGTH-byte chunks. */
        while (len >= MD5_BLOCK_LENGTH) {
            md5_transform(ctx->state, input);
            input += MD5_BLOCK_LENGTH;
            len -= MD5_BLOCK_LENGTH;
        }
    }

    /* Handle any remaining bytes of data. */
    if (len != 0)
        memcpy(ctx->buffer + have, input, len);
}

/*------------------------------------------------------------------*\
  Pad pad to 64-byte boundary with the bit pattern
  1 0* (64-bit count of bits processed, MSB-first)
\*------------------------------------------------------------------*/
static void md5_pad(md5Context *ctx)
{
    u_int8_t count[8];
    size_t padlen;

    /* Convert count to 8 bytes in little endian order. */
    PUT_64BIT_LE(count, ctx->count);

    /* Pad out to 56 mod 64. */
    padlen = MD5_BLOCK_LENGTH -
        ((ctx->count >> 3) & (MD5_BLOCK_LENGTH - 1));
    if (padlen < 1 + 8)
        padlen += MD5_BLOCK_LENGTH;
    md5_update(ctx, PADDING, padlen - 8);        /* padlen - 8 <= 64 */
    md5_update(ctx, count, 8);
}

/*------------------------------------------------------------------*\
   Final wrapup--call MD5Pad, fill in digest and zero out ctx.
\*------------------------------------------------------------------*/
static void md5_final(unsigned char digest[MD5_DIGEST_LENGTH], md5Context *ctx) {
    int i;

    md5_pad(ctx);
    if (digest != NULL) {
        for (i = 0; i < 4; i++)
            PUT_32BIT_LE(digest + i * 4, ctx->state[i]);
        memset(ctx, 0, sizeof(*ctx));
    }
}


/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f, w, x, y, z, data, s) \
    ( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )
/*------------------------------------------------------------------*\
   The core of the MD5 algorithm, this alters an existing MD5 hash to
   reflect the addition of 16 longwords of new data.  MD5Update blocks
   the data and converts bytes into longwords for this routine.

\*------------------------------------------------------------------*/
static void md5_transform(u_int32_t state[4], const u_int8_t block[MD5_BLOCK_LENGTH]) {
    u_int32_t a, b, c, d, in[MD5_BLOCK_LENGTH / 4];

#if BYTE_ORDER == LITTLE_ENDIAN
    memcpy(in, block, sizeof(in));
#else
    for (a = 0; a < MD5_BLOCK_LENGTH / 4; a++) {
        in[a] = (u_int32_t)(
            (u_int32_t)(block[a * 4 + 0]) |
            (u_int32_t)(block[a * 4 + 1]) <<  8 |
            (u_int32_t)(block[a * 4 + 2]) << 16 |
            (u_int32_t)(block[a * 4 + 3]) << 24);
    }
#endif

    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];

    MD5STEP(F1, a, b, c, d, in[ 0] + 0xd76aa478,  7);
    MD5STEP(F1, d, a, b, c, in[ 1] + 0xe8c7b756, 12);
    MD5STEP(F1, c, d, a, b, in[ 2] + 0x242070db, 17);
    MD5STEP(F1, b, c, d, a, in[ 3] + 0xc1bdceee, 22);
    MD5STEP(F1, a, b, c, d, in[ 4] + 0xf57c0faf,  7);
    MD5STEP(F1, d, a, b, c, in[ 5] + 0x4787c62a, 12);
    MD5STEP(F1, c, d, a, b, in[ 6] + 0xa8304613, 17);
    MD5STEP(F1, b, c, d, a, in[ 7] + 0xfd469501, 22);
    MD5STEP(F1, a, b, c, d, in[ 8] + 0x698098d8,  7);
    MD5STEP(F1, d, a, b, c, in[ 9] + 0x8b44f7af, 12);
    MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
    MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
    MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122,  7);
    MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
    MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
    MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

    MD5STEP(F2, a, b, c, d, in[ 1] + 0xf61e2562,  5);
    MD5STEP(F2, d, a, b, c, in[ 6] + 0xc040b340,  9);
    MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
    MD5STEP(F2, b, c, d, a, in[ 0] + 0xe9b6c7aa, 20);
    MD5STEP(F2, a, b, c, d, in[ 5] + 0xd62f105d,  5);
    MD5STEP(F2, d, a, b, c, in[10] + 0x02441453,  9);
    MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
    MD5STEP(F2, b, c, d, a, in[ 4] + 0xe7d3fbc8, 20);
    MD5STEP(F2, a, b, c, d, in[ 9] + 0x21e1cde6,  5);
    MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6,  9);
    MD5STEP(F2, c, d, a, b, in[ 3] + 0xf4d50d87, 14);
    MD5STEP(F2, b, c, d, a, in[ 8] + 0x455a14ed, 20);
    MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905,  5);
    MD5STEP(F2, d, a, b, c, in[ 2] + 0xfcefa3f8,  9);
    MD5STEP(F2, c, d, a, b, in[ 7] + 0x676f02d9, 14);
    MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

    MD5STEP(F3, a, b, c, d, in[ 5] + 0xfffa3942,  4);
    MD5STEP(F3, d, a, b, c, in[ 8] + 0x8771f681, 11);
    MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
    MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
    MD5STEP(F3, a, b, c, d, in[ 1] + 0xa4beea44,  4);
    MD5STEP(F3, d, a, b, c, in[ 4] + 0x4bdecfa9, 11);
    MD5STEP(F3, c, d, a, b, in[ 7] + 0xf6bb4b60, 16);
    MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
    MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6,  4);
    MD5STEP(F3, d, a, b, c, in[ 0] + 0xeaa127fa, 11);
    MD5STEP(F3, c, d, a, b, in[ 3] + 0xd4ef3085, 16);
    MD5STEP(F3, b, c, d, a, in[ 6] + 0x04881d05, 23);
    MD5STEP(F3, a, b, c, d, in[ 9] + 0xd9d4d039,  4);
    MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
    MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
    MD5STEP(F3, b, c, d, a, in[2 ] + 0xc4ac5665, 23);

    MD5STEP(F4, a, b, c, d, in[ 0] + 0xf4292244,  6);
    MD5STEP(F4, d, a, b, c, in[7 ] + 0x432aff97, 10);
    MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
    MD5STEP(F4, b, c, d, a, in[5 ] + 0xfc93a039, 21);
    MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3,  6);
    MD5STEP(F4, d, a, b, c, in[3 ] + 0x8f0ccc92, 10);
    MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
    MD5STEP(F4, b, c, d, a, in[1 ] + 0x85845dd1, 21);
    MD5STEP(F4, a, b, c, d, in[8 ] + 0x6fa87e4f,  6);
    MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
    MD5STEP(F4, c, d, a, b, in[6 ] + 0xa3014314, 15);
    MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
    MD5STEP(F4, a, b, c, d, in[4 ] + 0xf7537e82,  6);
    MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
    MD5STEP(F4, c, d, a, b, in[2 ] + 0x2ad7d2bb, 15);
    MD5STEP(F4, b, c, d, a, in[9 ] + 0xeb86d391, 21);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

#ifndef STAND_ALONE

static char* userhash = NULL;

static int alock_auth_md5_init(const char* args) {

    if (!args) {
        fprintf(stderr, "%s", "alock: error, missing arguments for [md5].\n");
        return 0;
    }

    if (strstr(args, "md5:") == args && strlen(&args[4]) > 0) {
        char* arguments = strdup(&args[4]);
        char* tmp;
        char* arg = NULL;
        for (tmp = arguments; tmp; ) {
            arg = strsep(&tmp, ",");
            if (arg && !userhash) {
                if (strstr(arg, "hash=") == arg && strlen(arg) > 5) {
                    if (strlen(&arg[5]) == MD5_DIGEST_STRING_LENGTH - 1) {
                        if (!userhash)
                            userhash = strdup(&arg[5]);
                    } else {
                        fprintf(stderr, "%s", "alock: error, missing or incorrect hash for [md5].\n");
                        free(arguments);
                        return 0;
                    }
                } else if (strstr(arg, "file=") == arg && strlen(arg) > 6) {
                    char* tmp_hash = NULL;
                    FILE* hashfile = fopen(&arg[5], "r");
                    if (hashfile) {
                        int c;
                        size_t i = 0;
                        tmp_hash = (char*)malloc(MD5_DIGEST_STRING_LENGTH);
                        memset(tmp_hash, 0, MD5_DIGEST_STRING_LENGTH);
                        for(i = 0, c = fgetc(hashfile);
                            i < MD5_DIGEST_STRING_LENGTH - 1 && c != EOF; i++, c = fgetc(hashfile)) {
                            tmp_hash[i] = c;
                        }
                        fclose(hashfile);
                    } else {
                        fprintf(stderr, "alock: error, couldnt read [%s] for [md5].\n",
                                &arg[5]);
                        free(arguments);
                        return 0;
                    }

                    if (!tmp_hash || strlen(tmp_hash) != MD5_DIGEST_STRING_LENGTH - 1) {
                        fprintf(stderr, "alock: error, given file [%s] doesnt contain a valid hash for [md5].\n",
                                &arg[5]);
                        if (tmp_hash)
                            free(tmp_hash);
                        free(arguments);
                        return 0;
                    }

                    userhash = tmp_hash;
                }
            }
        }
        free(arguments);
    } else {
        fprintf(stderr, "%s", "alock: error, missing arguments for [md5].\n");
        return 0;
    }

    if (!userhash) {
        fprintf(stderr, "%s", "alock: error, missing hash for [md5].\n");
        return 0;
    }

    alock_string2lower(userhash);

    return 1;
}

static int alock_auth_md5_deinit() {
    if (userhash)
        free(userhash);
    return 1;
}

static int alock_auth_md5_auth(const char* pass) {

    unsigned char digest[MD5_DIGEST_LENGTH];
    unsigned char stringdigest[MD5_DIGEST_STRING_LENGTH];
    size_t i;
    md5Context md5;

    if (!pass || !userhash)
        return 0;

    md5_init(&md5);
    md5_update(&md5, (unsigned char*)pass, strlen(pass));
    md5_final(digest, &md5);

    memset(stringdigest, 0, MD5_DIGEST_STRING_LENGTH);
    for (i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf((char*)&stringdigest[i*2], "%02x", digest[i]);
    }

    return !strcmp((char*)stringdigest, userhash);
}

struct aAuth alock_auth_md5 = {
    "md5",
    alock_auth_md5_init,
    alock_auth_md5_auth,
    alock_auth_md5_deinit
};

/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */
#else

int main(int argc, char* argv[]) {

    unsigned char digest[MD5_DIGEST_LENGTH];
    unsigned int i;
    unsigned char c;
    md5Context md5;

    if (argc > 1) {
        printf("%s", "amd5 - reads from stdin to calculate a md5-hash.\n");
        exit(EXIT_SUCCESS);
    }

    md5_init(&md5);
    while((c = fgetc(stdin)) != (unsigned char)EOF) {
        md5_update(&md5, &c, 1);
    }
    md5_final(digest, &md5);

    for(i = 0; i < MD5_DIGEST_LENGTH; ++i)
        printf("%02x", digest[i]);
    printf("%s", "\n");
    fflush(stdout);

    return EXIT_SUCCESS;
}

#endif /* STAND_ALONE */
/* ---------------------------------------------------------------- *\
\* ---------------------------------------------------------------- */

