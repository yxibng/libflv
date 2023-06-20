#ifndef __AAC_H__
#define __AAC_H__

#include <cstdlib>

namespace nx {

// ISO/IEC 13818-7:2004(E)
struct adts_fixed_header {
    unsigned short syncword : 12;         // Syncword, all bits must be set to 1.
    unsigned char  id : 1;                // MPEG Version, set to 0 for MPEG-4 and 1 for MPEG-2.
    unsigned char  layer : 2;             // Layer, always set to 0.
    unsigned char  protection_absent : 1; // Protection absence, set to 1 if there is no CRC and 0 if there is CRC.
    /*
    Profile, the MPEG-4 Audio Object Type minus 1. https://wiki.multimedia.cx/index.php/MPEG-4_Audio#Audio_Object_Types.
    0 Main profile
    1 Low Complexity profile (LC)
    2 Scalable Sampling Rate profile (SSR)
    3 (reserved)
    */
    unsigned char profile : 2;
    /*
    MPEG-4 Sampling Frequency Index (15 is forbidden).
    0: 96000 Hz
    1: 88200 Hz
    2: 64000 Hz
    3: 48000 Hz
    4: 44100 Hz
    5: 32000 Hz
    6: 24000 Hz
    7: 22050 Hz
    8: 16000 Hz
    9: 12000 Hz
    10: 11025 Hz
    11: 8000 Hz
    12: 7350 Hz
    13: Reserved
    14: Reserved
    15: frequency is written explictly
    */
    unsigned char sampling_frequency_index : 4;
    unsigned char private_bit : 1; // Private bit, guaranteed never to be used by MPEG, set to 0 when encoding, ignore when decoding.
    /*
    0: Defined in AOT Specifc Config
    1: 1 channel: front-center
    2: 2 channels: front-left, front-right
    3: 3 channels: front-center, front-left, front-right
    4: 4 channels: front-center, front-left, front-right, back-center
    5: 5 channels: front-center, front-left, front-right, back-left, back-right
    6: 6 channels: front-center, front-left, front-right, back-left, back-right, LFE-channel
    7: 8 channels: front-center, front-left, front-right, side-left, side-right, back-left, back-right, LFE-channel
    8-15: Reserved
    */
    unsigned char channel_configuration : 3;
    unsigned char original_copy : 1; // Originality, set to 1 to signal originality of the audio and 0 otherwise.
    unsigned char home : 1;          // Home, set to 1 to signal home usage of the audio and 0 otherwise.
};                                   // length : 28 bits

struct adts_variable_header {
    unsigned char  copyright_identification_bit : 1;
    unsigned char  copyright_identification_start : 1;
    unsigned short aac_frame_length : 13;                  // Frame length, length of the ADTS frame including headers and CRC check.
    unsigned short adts_buffer_fullness : 11;
    unsigned char  number_of_raw_data_blocks_in_frame : 2; // Number of AAC frames (RDBs (Raw Data Blocks)) in ADTS frame minus 1. For maximum compatibility always use one AAC frame per ADTS frame.
};                                                         // length : 28 bits

struct adts_header {

    enum Profile {
        Main = 0,
        LC   = 1,
        SSR  = 2
    };

    uint8_t sample_rate_index( uint32_t sample_rate ) {
        uint32_t sample_rates[] = {
            96000, // 0x0
            88200, // 0x1
            64000, // 0x2
            48000, // 0x3
            44100, // 0x4
            32000, // 0x5
            24000, // 0x6
            22050, // 0x7
            16000, // 0x8
            12000, // 0x9
            11025, // 0xa
            8000   // 0xb
        };
        uint32_t count = sizeof( sample_rates ) / sizeof( uint32_t );
        for ( int i = 0; i < count; i++ ) {
            if ( sample_rates[i] == sample_rate ) {
                return i;
            }
        }
        // TODO: handle error
        return 0;
    }
    adts_fixed_header    fixed_header;
    adts_variable_header variable_header;

    adts_header( Profile profile, uint32_t sample_rate, uint8_t channelCount, int size ) {
        /*
        refer to https://www.igac.gov.co/sites/all/libraries/ffmpeg/libavformat/adtsenc.c
        static int adts_write_frame_header(AVFormatContext *s, int size);
        */
        fixed_header.syncword              = 0xFFF;
        fixed_header.id                    = 1; // MPEG-4
        fixed_header.layer                 = 0;
        fixed_header.protection_absent     = 1;
        fixed_header.profile               = profile;
        fixed_header.channel_configuration = sample_rate_index( sample_rate );
        fixed_header.private_bit           = 0;
        fixed_header.channel_configuration = channelCount;
        fixed_header.original_copy         = 0;
        fixed_header.home                  = 0;

        variable_header.copyright_identification_bit   = 0;
        variable_header.copyright_identification_start = 0;

        const int ADTS_HEADER_SIZE                         = 7;
        variable_header.aac_frame_length                   = ADTS_HEADER_SIZE + size;
        variable_header.adts_buffer_fullness               = 0x7FF;
        variable_header.number_of_raw_data_blocks_in_frame = 0;
    }

    static void adts_header_to_buf( const adts_header &header, uint8_t buf[7] ) {

        buf[0] = header.fixed_header.syncword >> 4 & 0xFF;

        buf[1] = ( ( header.fixed_header.syncword & 0xF ) << 4 ) |
                 ( ( header.fixed_header.id & 0x01 ) << 3 ) |
                 ( ( header.fixed_header.layer & 0b11 ) << 1 ) |
                 ( header.fixed_header.protection_absent & 0x01 );

        buf[2] = ( header.fixed_header.profile & 0b11 << 6 ) |
                 ( header.fixed_header.sampling_frequency_index & 0xF << 2 ) |
                 ( header.fixed_header.private_bit & 0b1 << 1 ) |
                 ( header.fixed_header.channel_configuration >> 2 & 0b1 );

        buf[3] = ( header.fixed_header.channel_configuration & 0b11 << 6 ) |
                 ( header.fixed_header.original_copy & 0b1 << 5 ) |
                 ( header.fixed_header.home & 0b1 << 4 ) |
                 ( header.variable_header.copyright_identification_bit & 0b1 << 3 ) |
                 ( header.variable_header.copyright_identification_start & 0b1 << 2 ) |
                 ( header.variable_header.aac_frame_length >> 11 & 0b11 );

        buf[4] = header.variable_header.aac_frame_length >> 3 & 0xFF;

        buf[5] = ( header.variable_header.aac_frame_length & 0b111 << 5 ) |
                 ( header.variable_header.adts_buffer_fullness >> 6 & 0x1F );

        buf[6] = ( header.variable_header.adts_buffer_fullness & 0x3F << 2 ) |
                 ( header.variable_header.number_of_raw_data_blocks_in_frame & 0b11 );
    }

    static void parse_adts_header( adts_header &header, const uint8_t buf[7] ) {

        header.fixed_header.syncword = ( buf[0] << 4 ) | ( buf[1] >> 4 & 0xF );    // 12bits

        header.fixed_header.id                = buf[1] >> 3 & 0b1;                 // 1bit
        header.fixed_header.layer             = buf[1] >> 1 & 0b11;                // 2bits
        header.fixed_header.protection_absent = buf[1] & 0b1;                      // 1bit

        header.fixed_header.profile                  = buf[2] >> 6 & 0b11;         // 2bits
        header.fixed_header.sampling_frequency_index = buf[2] >> 2 & 0xF;          // 4bits
        header.fixed_header.private_bit              = buf[2] >> 1 & 0b1;          // 1bits

        header.fixed_header.channel_configuration = ( buf[2] & 0b1 << 2 ) |        // 1bit
                                                    ( buf[3] >> 6 & 0b11 );        // 2bits
        header.fixed_header.original_copy = buf[3] >> 5 & 0b1;                     // 1bit
        header.fixed_header.home          = buf[3] >> 4 & 0b1;                     // 1bit

        header.variable_header.copyright_identification_bit   = buf[3] >> 3 & 0b1; // 1bit
        header.variable_header.copyright_identification_start = buf[3] >> 2 & 0b1; // 1bit

        header.variable_header.aac_frame_length = ( buf[3] & 0b11 << 11 ) |        // 2bits
                                                  ( buf[4] << 3 ) |                // 8bits
                                                  ( buf[5] >> 5 & 0b111 );         // 3bits

        header.variable_header.adts_buffer_fullness = ( buf[5] & 0x1F << 6 ) |     // 5bits
                                                      ( buf[6] >> 2 & 0x3F );      // 6bits;

        header.variable_header.number_of_raw_data_blocks_in_frame = buf[6] & 0b11; // 2bits
    }
};

};     // namespace nx

#endif // __AAC_H__