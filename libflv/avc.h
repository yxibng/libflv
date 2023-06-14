

#ifndef __AVC_H__
#define __AVC_H__

#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace nx {

// Rec. ITU-T H.264 (02/2014)
// Table 7-1 - NAL unit type codes, syntax element categories, and NAL unit type classes
// #define H264_NAL_IDR				5 // Coded slice of an IDR picture
// #define H264_NAL_SEI				6 // Supplemental enhancement information
// #define H264_NAL_SPS				7 // Sequence parameter set
// #define H264_NAL_PPS				8 // Picture parameter set
// #define H264_NAL_AUD				9 // Access unit delimiter
// #define H264_NAL_SPS_EXTENSION		13 // Access unit delimiter
// #define H264_NAL_PREFIX				14 // Prefix NAL unit
// #define H264_NAL_SPS_SUBSET			15 // Subset sequence parameter set
// #define H264_NAL_XVC				20 // Coded slice extension(SVC/MVC)
// #define H264_NAL_3D					21 // Coded slice extension for a depth view component or a 3D-AVC texture view component
enum NaluType {
    IDR = 5,
    SEI = 6,
    SPS = 7,
    PPS = 8
};

// IOS_IEC_14496-15-AVC-Format
struct AVCDecoderConfigurationRecord {
    struct SPS {
        uint16_t sequenceParameterSetLength  = 0;
        uint8_t *sequenceParameterSetNALUnit = nullptr;
        SPS( uint8_t *nalu, uint16_t size ) {
            sequenceParameterSetLength  = size;
            sequenceParameterSetNALUnit = (uint8_t *)malloc( size );
            memcpy( sequenceParameterSetNALUnit, nalu, size );
        }
        ~SPS() {
            if ( sequenceParameterSetNALUnit ) {
                free( sequenceParameterSetNALUnit );
            }
        }
    };

    struct PPS {
        uint16_t pictureParameterSetLength;
        uint8_t *pictureParameterSetNALUnit;
        PPS( uint8_t *nalu, uint16_t size ) {
            pictureParameterSetLength  = size;
            pictureParameterSetNALUnit = (uint8_t *)malloc( size );
            memcpy( pictureParameterSetNALUnit, nalu, size );
        }
        ~PPS() {
            if ( pictureParameterSetNALUnit ) {
                free( pictureParameterSetNALUnit );
            }
        }
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
    SPS *sps;
    // usually 1
    unsigned char numOfPictureParameterSets;
    // pps
    PPS *pps;

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

    void fillSps( uint8_t *sps, uint16_t size ) {
        this->sps = new SPS( sps, size );
    }

    void fillPps( uint8_t *pps, uint16_t size ) {
        this->pps = new PPS( pps, size );
    }

    ~AVCDecoderConfigurationRecord() {
        if ( sps ) delete sps;
        if ( pps ) delete pps;
    }
};

struct H264SPS {
    uint8_t profile_idc;
    uint8_t compatibility;
    uint8_t level_idc;
    uint8_t chroma_format_idc;
    uint8_t bit_depth_luma;
    uint8_t bit_depth_chroma;
};

/**
 * @brief Extract rbsb from nalu. Remove emulation_prevention_three_byte and nalu header.
 *
 * @param nalu  nalu pointer
 * @param nalu_size nalu length
 * @param rbsp_size The length of the returned rbsp buf.
 * @return uint8_t*  The rbsp buf pointer, return NULL if failed.  If not NULL, should be freed by the caller.
 */
uint8_t *avc_extract_rbsp_from_nalu( const uint8_t *nalu, uint32_t nalu_size, uint32_t *rbsp_size );

/// @brief Decode sps from nal unit.
/// @param sps the H264SPS struct pointer to be filled with data.
/// @param sps_nalu sps nal unit buf.
/// @param sps_nalu_size sps nal unit buf length.
/// @return 0: success, <0: failed.
int avc_decode_sps( H264SPS *sps, const uint8_t *sps_nalu, uint32_t sps_nalu_size );

};     // namespace nx

#endif // __AVC_H__
