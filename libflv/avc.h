

#ifndef __AVC_H__
#define __AVC_H__

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

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
struct H264SPS {
    uint8_t id;
    uint8_t profile_idc;
    uint8_t compatibility;
    uint8_t level_idc;
    uint8_t chroma_format_idc;
    uint8_t bit_depth_luma_minus8;
    uint8_t bit_depth_chroma_minus8;

    uint32_t pic_width_in_mbs        = 0;
    uint32_t pic_height_in_map_units = 0;

    uint32_t frame_crop_left_offset   = 0; /* pixels */
    uint32_t frame_crop_right_offset  = 0; /* pixels */
    uint32_t frame_crop_top_offset    = 0; /* pixels */
    uint32_t frame_crop_bottom_offset = 0; /* pixels */

    void get_resolution( uint32_t &width, uint32_t &height ) {
        const int MACROBLOCK_SIZE = 16;
        width                     = MACROBLOCK_SIZE * pic_width_in_mbs - frame_crop_left_offset - frame_crop_right_offset;
        height                    = MACROBLOCK_SIZE * pic_height_in_map_units - frame_crop_top_offset - frame_crop_bottom_offset;
    }
};

template <typename T>
struct Buffer {
    uint8_t *buf;
    T        size;
    Buffer( uint8_t *buf, T size ) {
        this->buf  = (uint8_t *)malloc( size );
        this->size = size;
        memcpy( this->buf, buf, size );
    }
    Buffer( const Buffer &buffer ) {
        this->buf  = (uint8_t *)malloc( buffer.size );
        this->size = buffer.size;
        memcpy( this->buf, buffer.buf, buffer.size );
    }
    Buffer &operator=( const Buffer &buffer ) {
        this->buf  = (uint8_t *)malloc( buffer.size );
        this->size = buffer.size;
        memcpy( this->buf, buffer.buf, buffer.size );
        return *this;
    }
    Buffer( Buffer &&buffer ) {
        this->buf  = buffer.buf;
        this->size = buffer.size;

        buffer.buf  = nullptr;
        buffer.size = 0;
    }
    ~Buffer() {
        if ( buf ) free( buf );
    }
};
using NaluBuffer = Buffer<uint32_t>;

/*
ISO/IEC 14496-15:2010(E) 5.2.4.1.1 Syntax (p16)

aligned(8) class AVCDecoderConfigurationRecord {
    unsigned int(8) configurationVersion = 1;
    unsigned int(8) AVCProfileIndication;
    unsigned int(8) profile_compatibility;
    unsigned int(8) AVCLevelIndication;
    bit(6) reserved = '111111'b;
    unsigned int(2) lengthSizeMinusOne;
    bit(3) reserved = '111'b;

    unsigned int(5) numOfSequenceParameterSets;
    for (i=0; i< numOfSequenceParameterSets; i++) {
        unsigned int(16) sequenceParameterSetLength ;
        bit(8*sequenceParameterSetLength) sequenceParameterSetNALUnit;
    }

    unsigned int(8) numOfPictureParameterSets;
    for (i=0; i< numOfPictureParameterSets; i++) {
        unsigned int(16) pictureParameterSetLength;
        bit(8*pictureParameterSetLength) pictureParameterSetNALUnit;
    }

    if( profile_idc == 100 || profile_idc == 110 ||
        profile_idc == 122 || profile_idc == 144 )
    {
        bit(6) reserved = '111111'b;
        unsigned int(2) chroma_format;
        bit(5) reserved = '11111'b;
        unsigned int(3) bit_depth_luma_minus8;
        bit(5) reserved = '11111'b;
        unsigned int(3) bit_depth_chroma_minus8;
        unsigned int(8) numOfSequenceParameterSetExt;
        for (i=0; i< numOfSequenceParameterSetExt; i++) {
            unsigned int(16) sequenceParameterSetExtLength;
            bit(8*sequenceParameterSetExtLength) sequenceParameterSetExtNALUnit;
        }
    }
}
*/
struct AVCDecoderConfigurationRecord {

    // Annex-B to MP4, 00 00 00 01 to 4 bytes nalu length
    static const int NaluLengthSize = 4;

    using sps_pps_buf = Buffer<uint16_t>;

    uint8_t configurationVersion = 1;
    uint8_t AVCProfileIndication;
    uint8_t profile_compatibility;
    uint8_t AVCLevelIndication;
    uint8_t : 6;                                             // reserved, all 1
    uint8_t lengthSizeMinusOne : 2;
    uint8_t : 3;                                             // reserved, all 1
    uint8_t                  numOfSequenceParameterSets : 5; // sps count
    std::vector<sps_pps_buf> spsNalus;                       // sps array
    uint8_t                  numOfPictureParameterSets;      // pps count
    std::vector<sps_pps_buf> ppsNalus;                       // pps array
    struct SPSExt {
        /*
       when profile_idc is one of  [100, 110, 122, 244, 44, 83, 86, 118, 128, 138, 139, 134], the following varibles is valid.
        */
        uint8_t : 6;                         // reserved, all 1
        uint8_t chroma_format : 2;           // chroma_format
        uint8_t : 5;                         // reserved, all 1
        uint8_t bit_depth_luma_minus8 : 3;   // bit_depth_luma_minus8
        uint8_t : 5;                         // reserved, all 1
        uint8_t bit_depth_chroma_minus8 : 3; // bit_depth_chroma_minus8

        uint8_t                  numOfSequenceParameterSetExt;
        std::vector<sps_pps_buf> spsExtNalus; // sps ext array
    };
    SPSExt spsExt;

    AVCDecoderConfigurationRecord( uint8_t *spsNalu, uint16_t spsLength,
                                   uint8_t *ppsNalu, uint16_t ppsLength );

    std::vector<uint8_t> to_buf();
};

/**
 * @brief  find 00 00 01 from [p, end]
 *
 * @param p start pointer
 * @param end  end pointer
 * @return uint8_t* the start code (00, 00, 01) pointer, null if not found.
 */
uint8_t *avc_find_startcode( uint8_t *p, uint8_t *end );
/**
 * @brief split a avc frame to nal units.
 *
 * @param buf the avc frame buf.
 * @param size the avc frame size.
 * @param nalus nal units splited from the avc frame.
 */
void split_nalus( uint8_t *buf, uint32_t size, std::vector<NaluBuffer> &nalus );
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
