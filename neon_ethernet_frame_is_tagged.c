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

static inline uint32_t
u16x8_zero_byte_mask(uint16x8_t const input)
{
    uint8x16_t vall_one = vdupq_n_u8(0x0);
    uint8x16_t res_values = { 0x01, 0x02, 0x04, 0x08,
                              0x10, 0x20, 0x40, 0x80,
                              0x01, 0x02, 0x04, 0x08,
                              0x10, 0x20, 0x40, 0x80};

    /* input --> [0x80, 0x40, 0x01, 0xf0, ... ] */
    uint8x16_t test_result =
        vreinterpretq_u8_u16(vceqq_u16(input, vreinterpretq_u16_u8(vall_one)));
    uint8x16_t before_merge = vminq_u8(test_result, res_values);
    /*
      before_merge--> [0x80, 0x00, 0x00, 0x10, ... ]
       u8x16 --> [a,b,c,d, e,f,g,h, i,j,k,l, m,n,o,p]
       pair add until we have 2 uint64_t
     */
    uint16x8_t merge1 = vpaddlq_u8(before_merge);
    /* u16x8-->  [a+b,c+d, e+f,g+h, i+j,k+l, m+n,o+p] */
    uint32x4_t merge2 = vpaddlq_u16(merge1);
    /* u32x4-->  [a+b+c+d, e+f+g+h, i+j+k+l, m+n+o+p] */
    uint64x2_t merge3 = vpaddlq_u32(merge2);
    /* u64x2-->  [a+b+c+d+e+f+g+h,  i+j+k+l+m+n+o+p]  */
    return (uint32_t) (vgetq_lane_u64(merge3, 1) << 8) + vgetq_lane_u64(merge3, 0);
}

static inline int
ethernet_frame_is_tagged_x2_neon_2(uint16_t const type0, uint16_t const type1)
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

    return (u16x8_zero_byte_mask(rv) != 0xffff);
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