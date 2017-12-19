#include <assert.h>
#include <stdint.h>

#include <arm_neon.h>

static const uint16_t ETHERNET_TYPE_VLAN = 0x8100;
static const uint16_t ETHERNET_TYPE_DOT1AD = 0x88a8;
static const uint16_t ETHERNET_TYPE_VLAN_9100 = 0x9100;
static const uint16_t ETHERNET_TYPE_VLAN_9200 = 0x9200;

static inline int
ethernet_frame_is_tagged_ref(uint16_t const type)
{
    if ((type == ETHERNET_TYPE_VLAN)
        || (type == ETHERNET_TYPE_DOT1AD)
        || (type == ETHERNET_TYPE_VLAN_9100)
        || (type == ETHERNET_TYPE_VLAN_9200))
        return 1;

    return 0;
}

static inline int
ethernet_frame_is_tagged_x2_ref(uint16_t const type0, uint16_t const type1)
{
    return ethernet_frame_is_tagged_ref(type0)
           || ethernet_frame_is_tagged_ref(type1);
}

static inline int
ethernet_frame_is_tagged_x2_neon(uint16_t const type0, uint16_t const type1)
{
    uint16x8_t const ethertype_mask = { ETHERNET_TYPE_VLAN,
                                        ETHERNET_TYPE_DOT1AD,
                                        ETHERNET_TYPE_VLAN_9100,
                                        ETHERNET_TYPE_VLAN_9200,
                                        /* duplicate for type1 */
                                        ETHERNET_TYPE_VLAN,
                                        ETHERNET_TYPE_DOT1AD,
                                        ETHERNET_TYPE_VLAN_9100,
                                        ETHERNET_TYPE_VLAN_9200};

    uint16x8_t type_vect = vcombine_u16(vdup_n_u16(type0), vdup_n_u16(type1));

    uint16x8_t rv = vceqq_u16(ethertype_mask, type_vect);

    return vaddlvq_u16(rv) != 0;
}

static inline int
ethernet_frame_is_tagged_neon_1(uint16_t const type)
{
    uint16x4_t const ethertype_mask = { ETHERNET_TYPE_VLAN,
                                        ETHERNET_TYPE_DOT1AD,
                                        ETHERNET_TYPE_VLAN_9100,
                                        ETHERNET_TYPE_VLAN_9200};

    uint16x4_t r = vdup_n_u16(type);
    uint16x4_t rv = vceq_u16(ethertype_mask, r);
    return vaddlv_u16(rv) != 0;
}

static inline int
ethernet_frame_is_tagged_neon_2(uint16_t const type)
{
    return ethernet_frame_is_tagged_x2_neon(type, type);
}

static inline int
ethernet_frame_is_tagged_neon_3(uint16_t const type)
{
    uint16x4_t const ethertype_mask = { ETHERNET_TYPE_VLAN,
                                        ETHERNET_TYPE_DOT1AD,
                                        ETHERNET_TYPE_VLAN_9100,
                                        ETHERNET_TYPE_VLAN_9200};

    uint16x4_t r = vdup_n_u16(type);
    r = vceq_u16(ethertype_mask, r);
    return ((uint64_t) r) != 0;
}

#ifndef test_fn
#define test_fn ethernet_frame_is_tagged_neon_2
#endif
#ifndef N
#define N 16
#endif

#define TEST_NUM 1000000

int
main()
{
    int i, rv;
    unsigned j;
    /* padded with 0 */
    uint16_t const table[N] = {ETHERNET_TYPE_VLAN, ETHERNET_TYPE_DOT1AD,
                              ETHERNET_TYPE_VLAN_9100, ETHERNET_TYPE_VLAN_9200,
                              ETHERNET_TYPE_VLAN, ETHERNET_TYPE_DOT1AD,
                              ETHERNET_TYPE_VLAN_9100, ETHERNET_TYPE_VLAN_9200};
    int r[N];

    for (j = 0; j < N; j++)
    {
        r[j] = 0;
    }
    for (i = 0; i <= TEST_NUM; i++)
    {
        for (j = 0; j < N; j++)
        {
            r[j] |= test_fn(table[j], table[N - (j + 1)]);
        }
    }

    rv = 0;
    for (j = 0; j < N; j++)
    {
        rv |= r[j];
    }
    return rv;
}
