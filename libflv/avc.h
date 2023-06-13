

#ifndef __AVC_H__
#define __AVC_H__

#include <cstdint>
#include <cstring>

namespace nx {

// IOS_IEC_14496-15-AVC-Format
struct AVCDecoderConfigurationRecord {
    struct SPS {
        uint16_t sequenceParameterSetLength;
        uint8_t *sequenceParameterSetNALUnit;
    };
    
    struct PPS {
        uint16_t pictureParameterSetLength;
        uint8_t *pictureParameterSetNALUnit;
    };
    unsigned char version;
    unsigned char profileIndication;
    unsigned char profileCompatibility;
    unsigned char levelIndication;
    // reserved = ‘111111’
    unsigned char reserved6 : 6;
    unsigned char lengthSizeMinusOne : 2;
    // reserved = ‘111’
    unsigned char reserved3 : 3;
    // usually 1
    unsigned char numOfSequenceParameterSets : 5;
    // sps
    SPS sps;
    // usually 1
    unsigned char numOfPictureParameterSets;
    // pps
    PPS pps;
    
    /*
     ref to : https://en.wikipedia.org/wiki/Advanced_Video_Coding#Profiles
     profile_idc:
     Baseline Profile (BP, 66)
     Main Profile (MP, 77)
     Extended Profile (XP, 88)
     High Profile (HiP, 100)
     High 10 Profile (Hi10P, 110)
     High 4:2:2 Profile (Hi422P, 122)
     High 4:4:4 Predictive Profile (Hi444PP, 244)
     */
};






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
