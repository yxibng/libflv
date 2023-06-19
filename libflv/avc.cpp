#include "avc.h"
#include "get_bits.h"
#include <memory>
namespace nx {

// find 00 00 01
uint8_t *avc_find_startcode( uint8_t *p, uint8_t *end ) {
    intptr_t count        = end - p + 1;
    uint8_t  startCode[3] = { 0, 0, 1 };
    int      offset       = 0;
    while ( count >= 3 && offset < count ) {
        if ( startCode[0] == p[offset] && startCode[1] == p[offset + 1] && startCode[2] == p[offset + 2] ) {
            return p;
        }
        else {
            offset++;
        }
    }
    return NULL;
}

void split_nalus( uint8_t *buf, uint32_t size, std::vector<Buffer> &nalus ) {
    uint8_t *startCode = avc_find_startcode( buf, buf + size - 1 );
    if ( !startCode ) return;
    while ( startCode ) {
        // find next start code ptr
        uint8_t *nextStartCode = avc_find_startcode( startCode + 3, buf + size - 1 );
        // nalu start ptr
        uint8_t *naluStart = startCode + 3;
        // nalu end ptr
        uint8_t *naluEnd = nullptr;
        if ( nextStartCode ) {
            naluEnd = nextStartCode - 1;
        }
        else {
            naluEnd = buf + size - 1;
        }

        // remove trailing zero
        while ( *naluEnd == 0 ) {
            naluEnd = naluEnd - 1;
        }
        // save buffer
        intptr_t nalLength = naluEnd - naluStart + 1;
        nalus.emplace_back( naluStart, nalLength );
    }
}

uint8_t *avc_extract_rbsp_from_nalu( const uint8_t *nalu, uint32_t nalu_size, uint32_t *rbsp_size ) {
    uint8_t *rbsp = (uint8_t *)calloc( nalu_size, 1 );
    if ( !rbsp ) return NULL;
    uint32_t i   = 1; // skip nalu header 1 byte
    uint32_t len = 0;
    while ( i + 2 < nalu_size ) {
        // 00 00 03， 03 should be skipped
        if ( !nalu[i] && !nalu[i + 1] && nalu[i + 2] == 3 ) {
            rbsp[len++] = nalu[i++];
            rbsp[len++] = nalu[i++];
            i++; // remove emulation_prevention_three_byte
        }
        else {
            rbsp[len++] = nalu[i++];
        }
    }
    while ( i < nalu_size ) {
        rbsp[len++] = nalu[i++];
    }
    *rbsp_size = len;
    return rbsp;
}

int avc_decode_sps( H264SPS *sps, const uint8_t *sps_nalu, uint32_t sps_nalu_size ) {
    uint32_t dst_len = 0;
    uint8_t *rbsp    = avc_extract_rbsp_from_nalu( sps_nalu, sps_nalu_size, &dst_len );
    if ( !rbsp ) return -1;
    memset( sps, 0, sizeof( *sps ) );

    GetBitContext bitContext = GetBitContext( rbsp, dst_len );
    // profile
    sps->profile_idc = bitContext.get_bits( 8 );
    // compatibility
    sps->compatibility = bitContext.get_bits( 8 );
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
    free( rbsp );
    return 0;
}

}; // namespace nx
