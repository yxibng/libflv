#include "avc.h"
#include <memory>

namespace nx {

uint8_t *nal_unit_extract_rbsp( const uint8_t *src, uint32_t src_len, uint32_t *dst_len, int header_len ) {

    uint8_t *dst = (uint8_t *)malloc( src_len );
    uint32_t i, len;
    i = len = 0;
    while ( i < src_len && i < header_len ) {
        dst[i++] = src[len++];
    }
    while ( i + 2 < src_len ) {
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
    uint8_t *rbsp = nal_unit_extract_rbsp( buf + 1, buf_size - 1, &dst_len, 0 );
    if ( !rbsp ) return -1;

    memset( sps, 0, sizeof( *sps ) );

    sps->profile_idc = uint8_t( *rbsp );
    rbsp++;
}

}; // namespace nx
