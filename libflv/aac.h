#ifndef __AAC_H__
#define __AAC_H__

#include <cstdlib>

namespace nx {

// ISO/IEC 13818-7:2004(E)
struct adts_fixed_header {
    unsigned short syncword : 12;        // Syncword, all bits must be set to 1.
    unsigned char id : 1;                // MPEG Version, set to 0 for MPEG-4 and 1 for MPEG-2.
    unsigned char layer : 2;             // Layer, always set to 0.
    unsigned char protection_absent : 1; // Protection absence, set to 1 if there is no CRC and 0 if there is CRC.
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
    unsigned char copyright_identification_bit : 1;
    unsigned char copyright_identification_start : 1;
    unsigned short aac_frame_length : 13; // Frame length, length of the ADTS frame including headers and CRC check.
    unsigned short adts_buffer_fullness : 11;
    unsigned char number_of_raw_data_blocks_in_frame : 2; // Number of AAC frames (RDBs (Raw Data Blocks)) in ADTS frame minus 1. For maximum compatibility always use one AAC frame per ADTS frame.
};                                                        // length : 28 bits

struct adts_header {

    enum Profile {
        Main = 0,
        LC = 1,
        SSR = 2
    };
    adts_fixed_header fixed_header;
    adts_variable_header variable_header;

    uint8_t sample_rate_index(uint32_t sample_rate);
    adts_header() = default;
    adts_header(Profile profile, uint32_t sample_rate, uint8_t channelCount, int aac_frame_length);
    void to_buf(uint8_t buf[7]);
    static void adts_header_to_buf(const adts_header &header, uint8_t buf[7]);
    static adts_header parse_adts_header(const uint8_t buf[7]);
};
/**
 * @brief AAC AudioSpecificConfig
 * Refer to ISO 14496-3-2009. 2 bytes when write to file.
 */
struct AudioSpecificConfig {

    /*
    AAC_Main        = 1,
    AAC_LC          = 2,
    AAC_SSR         = 3,
    AAC_LTP         = 4,
    AAC_Scalable    = 6,
    ER_AAC_LC       = 17,
    ER_AAC_LTP      = 19,
    ER_AAC_Scalable = 20,
    ER_AAC_LD       = 23,
     */
    uint32_t audioObjectType : 5;
    uint32_t samplingFrequencyIndex : 4;
    uint32_t channelConfiguration : 4;
    // GASpecificConfig()
    struct
    {
        /*
        For all General Audio Object Types except AAC SSR and ER AAC LD:
        If set to “0” a 1024/128 lines IMDCT is used and frameLength is set to 1024, if set to “1” a 960/120 line IMDCT is used and frameLength is set to 960.
        For ER AAC LD:
        If set to “0” a 512 lines IMDCT is used and frameLength is set to 512, if set to “1” a 480 line IMDCT is used and frameLength is set to 480.
        For AAC SSR:
        Must be set to “0”. A 256/32 lines IMDCT is used. Note: The actual number of lines for the IMDCT (first or second value) is distinguished by the value of window_sequence.

        So set 0 for aac-lc.
         */
        uint32_t frameLengthFlag : 1;
        /* does not depend on core coder, set 0 */
        uint32_t dependsOnCoreCoder : 1;
        /* is not extension, set 0 */
        uint32_t extensionFlag : 1;
    };
    AudioSpecificConfig() = default;
    AudioSpecificConfig(uint8_t adts_header_buf[7]);
    void to_buf(uint8_t buf[2]);
};

}; // namespace nx

#endif // __AAC_H__
