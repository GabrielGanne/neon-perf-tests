#include <assert.h>
#include <stdint.h>
#include <x86intrin.h>

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
ethernet_frame_is_tagged_x2_sse(uint16_t const type0, uint16_t const type1)
{
    const __m128i ethertype_mask = _mm_set_epi16(ETHERNET_TYPE_VLAN,
            ETHERNET_TYPE_DOT1AD,
            ETHERNET_TYPE_VLAN_9100,
            ETHERNET_TYPE_VLAN_9200,
            /* duplicate for type1 */
            ETHERNET_TYPE_VLAN,
            ETHERNET_TYPE_DOT1AD,
            ETHERNET_TYPE_VLAN_9100,
            ETHERNET_TYPE_VLAN_9200);

    __m128i r = _mm_set_epi16(type0, type0, type0, type0,
            type1, type1, type1, type1);
    r = _mm_cmpeq_epi16(ethertype_mask, r);
    return !_mm_test_all_zeros(r, r);
}

static inline int
ethernet_frame_is_tagged_sse_1(uint16_t const type)
{
    const __m128i ethertype_mask = _mm_set_epi16(ETHERNET_TYPE_VLAN,
            ETHERNET_TYPE_DOT1AD,
            ETHERNET_TYPE_VLAN_9100,
            ETHERNET_TYPE_VLAN_9200,
            /* duplicate last one to fill register */
            ETHERNET_TYPE_VLAN_9200,
            ETHERNET_TYPE_VLAN_9200,
            ETHERNET_TYPE_VLAN_9200,
            ETHERNET_TYPE_VLAN_9200);

    __m128i r = _mm_set1_epi16(type);
    r = _mm_cmpeq_epi16(ethertype_mask, r);
    return !_mm_test_all_zeros(r, r);
}

static inline int
ethernet_frame_is_tagged_sse_2(uint16_t const type)
{
    return ethernet_frame_is_tagged_x2_sse(type, type);
}

#define ethernet_frame_is_tagged_sse ethernet_frame_is_tagged_sse_2

int
main()
{
    int i, r;

    r = 0;
    for (i = 0; i <= 0xffff; i++)
    {
        assert(ethernet_frame_is_tagged_sse(i) == ethernet_frame_is_tagged_ref(i));
        r |= ethernet_frame_is_tagged_sse(i);
    }

    return r;
}