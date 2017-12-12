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

#define ethernet_frame_is_tagged_neon ethernet_frame_is_tagged_neon_2

int
main()
{
    int i, r;

    r = 0;
    for (i = 0; i <= 0xffff; i++)
    {
        assert(ethernet_frame_is_tagged_neon(i) == ethernet_frame_is_tagged_ref(i));
        r |= ethernet_frame_is_tagged_neon(i);
    }

    return r;
}