#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

static inline uint16_t
byte_swap_u16_ref(uint16_t x)
{
    return (x >> 8) | (x << 8);
}

static inline uint16_t
byte_swap_u16_asm(uint16_t x)
{
    if (!__builtin_constant_p(x))
    {
        __asm__ ("rev16 %w0, %w0" : "+r" (x));
        return x;
    }
    return byte_swap_u16_ref(x);
}

static inline uint32_t
byte_swap_u32_ref(uint32_t x)
{
    return ((x << 24) | ((x & 0xff00) << 8) | ((x >> 8) & 0xff00) | (x >> 24));
}

static inline uint32_t
byte_swap_u32_asm(uint32_t x)
{
    if (!__builtin_constant_p(x))
    {
        __asm__ ("rev %w0, %w0" : "+r" (x));
        return x;
    }
    return byte_swap_u32_ref(x);
}

static inline uint64_t
byte_swap_u64_ref(uint64_t x)
{
#define _(x, n, i) \
    ((((x) >> (8*(i))) & 0xff) << (8*((n)-(i)-1)))

    return (_(x, 8, 0) | _(x, 8, 1)
           | _(x, 8, 2) | _(x, 8, 3)
           | _(x, 8, 4) | _(x, 8, 5) | _(x, 8, 6) | _(x, 8, 7));
#undef _
}

static inline uint64_t
byte_swap_u64_asm(uint64_t x)
{
    if (!__builtin_constant_p(x))
    {
        __asm__ ("rev %w0, %w0" : "+r" (x));
        return x;
    }
    return byte_swap_u64_ref(x);
}

#define byte_swap_u16 byte_swap_u16_asm
#define byte_swap_u32 byte_swap_u32_asm
#define byte_swap_u64 byte_swap_u64_asm
#define TEST_NUM (100000000)

int
main()
{
    volatile uint64_t x;
    uint64_t rv;

    rv = 0;
    for (x = 0; x < TEST_NUM; x++)
    {
        rv ^= byte_swap_u16(x);
        rv ^= byte_swap_u32(x);
        rv ^= byte_swap_u64(x);
    }

    return rv;
}