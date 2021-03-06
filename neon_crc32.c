#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arm_acle.h>

#define likely(x)   __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

static inline uint64_t
oat_hash(uint8_t const * buffer, unsigned int len)
{
    uint8_t const * p = buffer;
    uint32_t h = 0;
    unsigned int i;

    for (i = 0; i < len; i++)
    {
        h += p[i];
        h += (h << 10);
        h ^= (h >> 6);
    }

    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);

    return h;
}

#define PRIME64_1 11400714785074694791ULL
#define PRIME64_2 14029467366897019727ULL
#define PRIME64_3  1609587929392839161ULL
#define PRIME64_4  9650029242287828579ULL
#define PRIME64_5  2870177450012600261ULL
#define XXH_rotl64(x, r) ((x << r) | (x >> (64 - r)))

static inline uint64_t
__clib_xxhash(uint64_t const key)
{
    uint64_t k1, h64;

    k1 = key;
    h64 = 0x9e3779b97f4a7c13LL + PRIME64_5 + 8;
    k1 *= PRIME64_2;
    k1 = XXH_rotl64(k1, 31);
    k1 *= PRIME64_1;
    h64 ^= k1;
    h64 = XXH_rotl64(h64, 27) * PRIME64_1 + PRIME64_4;

    h64 ^= h64 >> 33;
    h64 *= PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= PRIME64_3;
    h64 ^= h64 >> 32;
    return h64;
}

static inline uint64_t
clib_xxhash(uint8_t const * buffer, unsigned int len)
{
    size_t i;
    unsigned int _len = len / 8;
    uint64_t tmp;

    tmp = 0;
    if (unlikely(8*_len != len))
        memcpy(&tmp, buffer + 8*_len, len - 8*_len);
    for (i = 0; i < _len; i++)
    {
        tmp ^= ((uint64_t *) buffer)[i];
    }

    return __clib_xxhash(tmp);
}

static inline uint32_t
clib_crc32c(uint8_t const * s, unsigned int len)
{
    uint32_t v = 0;

    for ( ; len >= 8; len -= 8, s += 8)
    {
        v = __crc32cd(v, *((uint64_t *) s));
    }

    for ( ; len >= 4; len -= 4, s += 4)
    {
        v = __crc32cw(v, *((uint32_t *) s));
    }

    for ( ; len >= 2; len -= 2, s += 2)
    {
        v = __crc32ch(v, *((uint16_t *) s));
    }

    for ( ; len >= 1; len -= 1, s += 1)
    {
        v = __crc32cb(v, *((uint8_t *) s));
    }

    return v;
}

#define TEST_NUM 1000000
#ifndef hash_fn
#define hash_fn clib_crc32c
#endif
#ifndef N
#define N 4
#endif
#ifndef BUFF_SZ
#define BUFF_SZ 16
#endif

int
main()
{
    int i, j;
    size_t k;
    volatile uint32_t rv;
    volatile uint32_t r[N];

    srandom(0);
    rv = 0;

    unsigned long buffer[BUFF_SZ];
    for (k = 0; k < sizeof(buffer)/sizeof(*buffer); k++)
    {
        buffer[k] = random();
    }

    for (j = 0; j < N; j++)
    {
        r[j] = 0;
    }
    for (i = 0; i < TEST_NUM; i++)
    {
        for (j = 0; j < N; j++)
        {
            r[j] ^= hash_fn((uint8_t const *)buffer, BUFF_SZ);
        }
    }

    rv = 0;
    for (j = 0; j < N; j++)
    {
        rv ^= r[j];
    }

    return rv;
}