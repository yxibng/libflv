//
//  flv-tag.h
//  libflv
//
//  Created by yxibng on 2023/6/9.
//

#ifndef flv_tag_h
#define flv_tag_h

#include "flv-header.h"
#include <cstdlib>
#include <cstring>

struct flv_aac_audio_specific_config_tag_t {

    struct aac_audio_specific_config {
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
        uint8_t profile = 2;

        /*
         sampling_frequency_index in aac adts header:
         0x0 - 96000
         0x1 - 88200
         0x2 - 64000
         0x3 - 48000
         0x4 - 44100
         0x5 - 32000
         0x6 - 24000
         0x7 - 22050
         0x8 - 16000
         0x9 - 12000
         0xa - 11025
         0xb - 8000

         can read from adts header
         */
        uint8_t sampleRateIndex = 3;
        /*
         1 - mono
         2 - stereo
         3 - center front, left, right
         4 - center front, rear, left, right
         */
        uint8_t channels = 1;
        // CASpecificConfig
        /*
         For all General Audio Object Types except AAC SSR and ER AAC LD:
         If set to “0” a 1024/128 lines IMDCT is used and frameLength is set to 1024, if set to “1” a 960/120 line IMDCT is used and frameLength is set to 960.
         For ER AAC LD:
         If set to “0” a 512 lines IMDCT is used and frameLength is set to 512, if set to “1” a 480 line IMDCT is used and frameLength is set to 480.
         For AAC SSR:
         Must be set to “0”. A 256/32 lines IMDCT is used. Note: The actual number of lines for the IMDCT (first or second value) is distinguished by the value of window_sequence.

         So set 0 for aac-lc.
         */
        uint8_t frameLengthFlag = 0;
        /* does not depend on core coder, set 0 */
        uint8_t dependsOnCoreCoder = 0;
        /* is not extension, set 0 */
        uint8_t extentionFlag = 0;
    };

    struct flv_audio_tag_header_t    audioTagHeader;
    struct aac_audio_specific_config aacAudioSpecificConfig;

    flv_aac_audio_specific_config_tag_t( uint8_t profile, uint8_t sampleRateIndex, uint8_t channels, uint32_t timestamp ) {

        audioTagHeader.timestamp = timestamp;
        // audio tag header 2 + audio specific config 2
        audioTagHeader.dataSize = 4;
        audioTagHeader.tagType  = flv_tag_header_t::TagType::audio;

        audioTagHeader.soundFormat = flv_audio_tag_header_t::aac;
        audioTagHeader.soundRate   = flv_audio_tag_header_t::rate44khz;
        audioTagHeader.soundSize   = flv_audio_tag_header_t::bits16;
        audioTagHeader.soundType   = flv_audio_tag_header_t::stereo;

        aacAudioSpecificConfig.profile         = profile;
        aacAudioSpecificConfig.sampleRateIndex = sampleRateIndex;
        aacAudioSpecificConfig.channels        = channels;
    }
};

struct flv_aac_raw_tag_t {
    struct flv_audio_tag_header_t audioTagHeader;
    uint8_t                      *aacRaw;
    uint32_t                      aacRawSize;
    flv_aac_raw_tag_t( uint8_t *aacRaw, uint32_t aacRawSize, uint32_t timestamp ) {

        audioTagHeader.tagType   = flv_tag_header_t::TagType::audio;
        audioTagHeader.timestamp = timestamp;
        // audio tag header 2 + aac raw size
        audioTagHeader.dataSize = 2 + aacRawSize;

        audioTagHeader.soundFormat = flv_audio_tag_header_t::aac;
        audioTagHeader.soundRate   = flv_audio_tag_header_t::rate44khz;
        audioTagHeader.soundSize   = flv_audio_tag_header_t::bits16;
        audioTagHeader.soundType   = flv_audio_tag_header_t::stereo;
        this->aacRaw               = (uint8_t *)calloc( 1, aacRawSize );
        memcpy( this->aacRaw, aacRaw, aacRawSize );
        this->aacRawSize = aacRawSize;
    }
    ~flv_aac_raw_tag_t() {
        free( this->aacRaw );
    }
};

struct flv_avc_sequence_header_tag_t {
    struct flv_video_tag_header videoTagHeader;

    flv_avc_sequence_header_tag_t( uint8_t *buf, uint32_t size, uint32_t timestamp ) {
        videoTagHeader.tagType   = flv_tag_header_t::TagType::video;
        videoTagHeader.timestamp = timestamp;
        // video tag header
        videoTagHeader.dataSize = 2 + aacRawSize;
    }
};

struct flv_avc_nalu_tag_t {
    struct flv_video_tag_header videoTagHeader;
};

#endif /* flv_tag_h */
