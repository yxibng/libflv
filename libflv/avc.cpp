#include "avc.h"
#include "get_bits.h"
#include <memory>
namespace nx {

uint8_t *nal_unit_extract_rbsp_from_ebsp( const uint8_t *src, uint32_t src_len, uint32_t *dst_len ) {
    uint8_t *dst = (uint8_t *)calloc( src_len, 1 );
    uint32_t i, len;
    i = len = 0;
    while ( i + 2 < src_len ) {
        // 00 00 03， 03 should be skipped
        if ( !src[i] && !src[i + 1] && src[i + 2] == 3 ) {
            dst[len++] = src[i++];
            dst[len++] = src[i++];
            i++; // remove emulation_prevention_three_byte
        }
        else {
            dst[len++] = src[i++];
        }
    }

    while ( i < src_len ) {
        dst[len++] = src[i++];
    }

    *dst_len = len;
    return dst;
}

int avc_decode_sps( H264SPS *sps, const uint8_t *buf, int buf_size ) {
    uint32_t dst_len;

    int      offset = 1; // skip 1 byte header
    uint8_t *rbsp   = nal_unit_extract_rbsp_from_ebsp( buf + offset, buf_size - offset, &dst_len );
    if ( !rbsp ) return -1;
    memset( sps, 0, sizeof( *sps ) );

    GetBitContext bitContext = GetBitContext( rbsp, dst_len );
    // profile
    sps->profile_idc = bitContext.get_bits( 8 );
    // flags
    sps->constraint_set0_flags = bitContext.get_bit1();
    sps->constraint_set1_flags = bitContext.get_bit1();
    sps->constraint_set2_flags = bitContext.get_bit1();
    sps->constraint_set3_flags = bitContext.get_bit1();
    sps->constraint_set4_flags = bitContext.get_bit1();
    sps->constraint_set5_flags = bitContext.get_bit1();
    bitContext.skip_bits( 2 );
    // level
    sps->level_idc = bitContext.get_bits( 8 );
    if ( sps->profile_idc == 100 || sps->profile_idc == 110 ||
         sps->profile_idc == 122 || sps->profile_idc == 244 || sps->profile_idc == 44 ||
         sps->profile_idc == 83 || sps->profile_idc == 86 || sps->profile_idc == 118 ||
         sps->profile_idc == 128 || sps->profile_idc == 138 || sps->profile_idc == 139 ||
         sps->profile_idc == 134 ) {
        sps->chroma_format_idc = bitContext.get_ue_golomb();
        if ( sps->chroma_format_idc == 3 ) {
            bitContext.skip_bits( 1 ); // separate_colour_plane_flag
        }
        sps->bit_depth_luma   = bitContext.get_ue_golomb() + 8;
        sps->bit_depth_chroma = bitContext.get_ue_golomb() + 8;
    }
    else {
        sps->chroma_format_idc = 1;
        sps->bit_depth_luma    = 8;
        sps->bit_depth_chroma  = 8;
    }
}

}; // namespace nx
