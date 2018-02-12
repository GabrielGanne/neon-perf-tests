#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arm_neon.h>

#define likely(x)   __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

static inline uint32_t
u8x16_compare_byte_mask_ref(uint8x16_t x)
{
    static int8_t const __attribute__ ((aligned(16))) xr[8] = {
        -7, -6, -5, -4, -3, -2, -1, 0
    };
    uint8x8_t mask_and = vdup_n_u8(0x80);
    int8x8_t mask_shift = vld1_s8(xr);

    uint8x8_t lo = vget_low_u8(x);
    uint8x8_t hi = vget_high_u8(x);

    lo = vand_u8(lo, mask_and);
    lo = vshl_u8(lo, mask_shift);

    hi = vand_u8(hi, mask_and);
    hi = vshl_u8(hi, mask_shift);

    lo = vpadd_u8(lo, lo);
    lo = vpadd_u8(lo, lo);
    lo = vpadd_u8(lo, lo);

    hi = vpadd_u8(hi, hi);
    hi = vpadd_u8(hi, hi);
    hi = vpadd_u8(hi, hi);

    return ((hi[0] << 8) | (lo[0] & 0xff));
}

static inline uint32_t
u8x16_compare_byte_mask_test(uint8x16_t x)
{
    uint8x16_t mask_shift = {-7, -6, -5, -4, -3, -2, -1, 0, -7, -6, -5, -4, -3, -2, -1, 0};
    uint8x16_t mask_and = vdupq_n_u8(0x80);
    x = vandq_u8(x, mask_and);
    x = vshlq_u8(x, vreinterpretq_s8_u8(mask_shift));
    x = vpaddq_u8(x, x);
    x = vpaddq_u8(x, x);
    x = vpaddq_u8(x, x);
    return vgetq_lane_u8(x, 0) | (vgetq_lane_u8(x, 1) << 8);
}

#define TEST_NUM 10000
#ifndef fn
#define fn u8x16_compare_byte_mask_ref
#endif
#ifndef N
#define N 4
#endif

int
main()
{
    int i, j;
    uint8x16_t v[N];
    volatile uint32_t r[N];
    volatile uint32_t rv;

    srandom(0);
    for (i = 0; i < N; i++)
    {
        v[i] = vdupq_n_u8((uint8_t) random());
    }
    rv = 0;
    memset(r, 0, sizeof(r));

    for (i = 0; i < TEST_NUM; i++)
    {
        for (j = 0; j < N; j++)
        {
            r[j] ^= fn(v[j]);
        }
    }

    rv = 0;
    for (i = 0; i < N; i++)
    {
        rv ^= r[i];
    }

    return rv;
}