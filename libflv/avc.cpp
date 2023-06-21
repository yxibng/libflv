#include "avc.h"
#include "get_bits.h"
#include <memory>
namespace nx {
AVCDecoderConfigurationRecord::AVCDecoderConfigurationRecord( uint8_t *spsNalu, uint16_t spsLength,
                                                              uint8_t *ppsNalu, uint16_t ppsLength ) {

    H264SPS h264sps;
    int     ret = avc_decode_sps( &h264sps, spsNalu, spsLength );
    // TODO: handle error
    assert( ret == 0 );
    if ( ret < 0 ) return;

    this->AVCProfileIndication  = h264sps.profile_idc;
    this->profile_compatibility = h264sps.compatibility;
    this->AVCLevelIndication    = h264sps.level_idc;
    this->lengthSizeMinusOne    = NaluLengthSize - 1;

    spsNalus.emplace_back( spsNalu, spsLength );
    numOfSequenceParameterSets = spsNalus.size();

    ppsNalus.emplace_back( ppsNalu, ppsLength );
    numOfPictureParameterSets = ppsNalus.size();

    std::vector<int> extProfiles{ 100, 110, 122, 244, 44, 83, 86, 118, 128, 138, 139, 134 };

    for ( auto profile : extProfiles ) {
        if ( h264sps.profile_idc == profile ) {
            this->spsExt.chroma_format                = h264sps.chroma_format_idc;
            this->spsExt.bit_depth_luma_minus8        = h264sps.bit_depth_luma_minus8;
            this->spsExt.bit_depth_chroma_minus8      = h264sps.bit_depth_chroma_minus8;
            this->spsExt.numOfSequenceParameterSetExt = 0;
            break;
        }
    }
}

std::vector<uint8_t> AVCDecoderConfigurationRecord::to_buf() {
    std::vector<uint8_t> avc_buf;

    avc_buf.push_back( configurationVersion );
    avc_buf.push_back( AVCProfileIndication );
    avc_buf.push_back( profile_compatibility );
    avc_buf.push_back( AVCLevelIndication );
    // sps count
    {
        uint8_t spsCount = ( 0x3F << 2 ) | ( numOfSequenceParameterSets & 0b11 );
        avc_buf.push_back( spsCount );
    }
    // sps content
    {
        for ( auto &sps : spsNalus ) {
            uint16_t length = sps.size;
            uint8_t *buf    = sps.buf;
            // big endian nalu length
            avc_buf.push_back( length >> 8 & 0xFF );
            avc_buf.push_back( length & 0xFF );
            // nalu content
            for ( uint16_t i = 0; i < length; i++ ) {
                avc_buf.push_back( *( buf + i ) );
            }
        }
    }
    // pps count
    avc_buf.push_back( numOfPictureParameterSets );
    // pps content
    {
        for ( auto &pps : ppsNalus ) {
            uint16_t length = pps.size;
            uint8_t *buf    = pps.buf;
            // big endian nalu length
            avc_buf.push_back( length >> 8 & 0xFF );
            avc_buf.push_back( length & 0xFF );
            // nalu content
            for ( uint16_t i = 0; i < length; i++ ) {
                avc_buf.push_back( *( buf + i ) );
            }
        }
    }

    std::vector<int> extProfiles{ 100, 110, 122, 244, 44, 83, 86, 118, 128, 138, 139, 134 };
    for ( auto profile : extProfiles ) {
        if ( AVCProfileIndication == profile ) {

            uint8_t chroma_format = ( 0x3F << 2 ) | ( spsExt.chroma_format & 0b11 );
            avc_buf.push_back( chroma_format );
            uint8_t bit_depth_luma_minus8 = ( 0x1F << 3 ) | ( spsExt.bit_depth_luma_minus8 & 0b111 );
            avc_buf.push_back( bit_depth_luma_minus8 );
            uint8_t bit_depth_chroma_minus8 = ( 0x1F << 3 ) | ( spsExt.bit_depth_chroma_minus8 & 0b111 );
            avc_buf.push_back( bit_depth_chroma_minus8 );
            avc_buf.push_back( spsExt.numOfSequenceParameterSetExt );
            // sps ext array
            {
                for ( auto &sps : spsExt.spsExtNalus ) {
                    uint16_t length = sps.size;
                    uint8_t *buf    = sps.buf;
                    // big endian nalu length
                    avc_buf.push_back( length >> 8 & 0xFF );
                    avc_buf.push_back( length & 0xFF );
                    // nalu content
                    for ( uint16_t i = 0; i < length; i++ ) {
                        avc_buf.push_back( *( buf + i ) );
                    }
                }
            }
            break;
        }
    }
    return avc_buf;
}

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

void split_nalus( uint8_t *buf, uint32_t size, std::vector<NaluBuffer> &nalus ) {
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
/*
refer to ffmpeg: libavformat/avc.c
int ff_avc_decode_sps(H264SPS *sps, const uint8_t *buf, int buf_size)
*/
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
    // id
    sps->id = bitContext.get_ue_golomb();

    if ( sps->profile_idc == 100 || sps->profile_idc == 110 ||
         sps->profile_idc == 122 || sps->profile_idc == 244 || sps->profile_idc == 44 ||
         sps->profile_idc == 83 || sps->profile_idc == 86 || sps->profile_idc == 118 ||
         sps->profile_idc == 128 || sps->profile_idc == 138 || sps->profile_idc == 139 ||
         sps->profile_idc == 134 ) {
        sps->chroma_format_idc = bitContext.get_ue_golomb();
        if ( sps->chroma_format_idc == 3 ) {
            bitContext.skip_bits( 1 ); // separate_colour_plane_flag
        }
        sps->bit_depth_luma_minus8   = bitContext.get_ue_golomb();
        sps->bit_depth_chroma_minus8 = bitContext.get_ue_golomb();
    }
    else {
        sps->chroma_format_idc       = 1;
        sps->bit_depth_luma_minus8   = 0;
        sps->bit_depth_chroma_minus8 = 0;
    }
    free( rbsp );
    return 0;
}

}; // namespace nx
