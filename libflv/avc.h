

#ifndef __AVC_H__
#define __AVC_H__

#include <cstdint>
#include <cstring>

namespace nx {

struct H264SPS {
    uint8_t profile_idc;
    uint8_t level_idc;
    // 6 bits flags, 2 bits reserved
    union {
        uint8_t constraint_set_flags;
        struct
        {
            uint8_t constraint_set0_flags : 1;
            uint8_t constraint_set1_flags : 1;
            uint8_t constraint_set2_flags : 1;
            uint8_t constraint_set3_flags : 1;
            uint8_t constraint_set4_flags : 1;
            uint8_t constraint_set5_flags : 1;
            uint8_t reserved : 2;
        };
    };
    uint8_t chroma_format_idc;
    uint8_t bit_depth_luma;
    uint8_t bit_depth_chroma;
};

/// @brief Extract rbsb from ebsp. Remove emulation_prevention_three_byte.
/// @param src  The ebsp pointer.
/// @param src_len  The length of the ebsp buf.
/// @param dst_len The length of the returned rbsp buf.
/// @return The rbsp buf pointer, return NULL if failed.  If not NULL, Should call free to free the memory.
uint8_t *nal_unit_extract_rbsp_from_ebsp( const uint8_t *src, uint32_t src_len, uint32_t *dst_len );

/// @brief Decode sps from nal unit.
/// @param sps the H264SPS struct pointer to be filled with data.
/// @param buf sps nal unit buf.
/// @param buf_size sps nal unit buf length.
/// @return 0: success, <0: failed.
int avc_decode_sps( H264SPS *sps, const uint8_t *buf, int buf_size );

};     // namespace nx

#endif // __AVC_H__