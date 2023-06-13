
#include "flvmuxer.h"
#include <cassert>

using namespace nx;

namespace nx {

// struct AudioSpecificConfig {
//     uint32_t profile : 5;
//     uint32_t sampleRateIndex : 4;
//     uint32_t channelConfiguration : 4;
//     uint32_t frameLengthFlag : 1;
//     uint32_t dependsOnCoreCoder : 1;
//     uint32_t extentionFlag : 1;
// };

int flv_write_header( FILE *file, const flv_header_t &flv_header ) {
    // F,L,V
    fwrite( &flv_header.FLV, sizeof( flv_header.FLV ), 1, file );
    // version
    fwrite( &flv_header.version, 1, 1, file );
    // audio / video flag
    uint8_t flag = 0;
    if ( flv_header.hasAudio ) flag |= 1 << 2;
    if ( flv_header.hasVideo ) flag |= 1;
    fwrite( &flag, 1, 1, file );
    // data offset
    fwrite( &flv_header.offset, sizeof( flv_header.offset ), 1, file );
    // tag size 0
    uint32_t tagSize0 = 0;
    fwrite( &tagSize0, sizeof( tagSize0 ), 1, file );
    return 9 + 4;
}

int flv_write_tag_header( FILE *file, const flv_tag_header_t tagHeader ) {
    // write tag type
    uint8_t tagType = tagType = tagHeader.tagType;
    fwrite( &tagType, 1, 1, file );
    /*
    4bytes dataSize =  AudioTagHeader 2 bytes +  AAC sequence header 2 bytes
    */
    uint32_t dataSize = tagHeader.dataSize;
    // big endian 3 bytes dataSize
    {
        uint8_t buf[3] = { 0 };
        buf[0]         = ( dataSize >> 16 ) & 0xFF;
        buf[1]         = ( dataSize >> 8 ) & 0xFF;
        buf[2]         = ( dataSize >> 0 ) & 0xFF;
        fwrite( buf, 1, 3, file );
    }
    // timestamp
    uint32_t timestamp = tagHeader.timestamp;
    {
        uint8_t buf[4] = { 0 };
        // timestamp 3 bytes
        buf[0] = ( timestamp >> 16 ) & 0xFF;
        buf[1] = ( timestamp >> 8 ) & 0xFF;
        buf[2] = ( timestamp >> 0 ) & 0xFF;
        // TimestampExtended 1 bytes
        buf[3] = ( timestamp >> 24 ) & 0xFF;
        fwrite( buf, 1, 4, file );
    }
    // streamID 3 bytes
    fwrite( &tagHeader.streamID, sizeof( tagHeader.streamID ), 1, file );
    return 11;
}

int flv_write_aac_tag_header( FILE *file, const flv_audio_tag_header_t &tagHeader ) {
    int tagHeaderSize = flv_write_tag_header( file, tagHeader );
    // audio tag header 2 bytes
    uint8_t audioTagHeader[2] = { 0 };
    {
        uint8_t soundFormat = tagHeader.soundFormat;
        uint8_t soundRate   = tagHeader.soundRate;
        uint8_t soundSize   = tagHeader.soundSize;
        uint8_t soundType   = tagHeader.soundType;
        audioTagHeader[0]   = ( ( soundFormat & 0xF ) << 4 ) |
                            ( ( soundRate & 0x03 ) << 2 ) |
                            ( ( soundSize & 0x01 ) << 1 ) |
                            ( soundType & 0x01 );
        audioTagHeader[1] = tagHeader.aacPacketType;
        fwrite( &audioTagHeader, 2, 1, file );
    }
    return tagHeaderSize + 2;
}

int flv_write_aac_audio_specific_config_tag( FILE *file, const flv_aac_audio_specific_config_tag_t &audioSpecificConfigTag ) {

    // write tag header
    const flv_audio_tag_header_t &tagHeader     = audioSpecificConfigTag.audioTagHeader;
    int                           tagHeaderSize = flv_write_aac_tag_header( file, tagHeader );
    // AudioSpecificConfig 2 bytes
    uint8_t audioSpecificConfig[2] = { 0 };
    {
        // profile 5bits +  sampleRateIndex 3 bits
        audioSpecificConfig[0] = ( audioSpecificConfigTag.aacAudioSpecificConfig.profile << 3 ) | ( audioSpecificConfigTag.aacAudioSpecificConfig.sampleRateIndex >> 1 & 0x07 );
        // sampleRateIndex 1 bit + channelConfiguration 4bits +  frameLengthFlag 1 bit
        audioSpecificConfig[1] = ( audioSpecificConfigTag.aacAudioSpecificConfig.sampleRateIndex & 0x01 << 7 ) |
                                 ( audioSpecificConfigTag.aacAudioSpecificConfig.channels & 0x0F << 3 ) |
                                 ( audioSpecificConfigTag.aacAudioSpecificConfig.frameLengthFlag & 0x01 << 2 ) |
                                 ( audioSpecificConfigTag.aacAudioSpecificConfig.dependsOnCoreCoder & 0x01 << 1 ) |
                                 ( audioSpecificConfigTag.aacAudioSpecificConfig.extentionFlag & 0x01 );

        fwrite( &audioSpecificConfig, 2, 1, file );
    }

    // write tagSize
    uint32_t tagSize          = tagHeaderSize + 2;
    uint32_t bitEndianTagSize = ntohl( tagHeaderSize + 4 );
    fwrite( &bitEndianTagSize, 4, 1, file );
    return tagSize + 4;
}

int flv_write_aac_raw_tag( FILE *file, const flv_aac_raw_tag_t &aacRaw ) {
    // write tag header
    const flv_audio_tag_header_t &tagHeader     = aacRaw.audioTagHeader;
    int                           tagHeaderSize = flv_write_aac_tag_header( file, tagHeader );
    // write audio data
    fwrite( aacRaw.aacRaw, aacRaw.aacRawSize, 1, file );
    // write tag size
    uint32_t tagSize          = tagHeaderSize + aacRaw.aacRawSize;
    uint32_t bitEndianTagSize = ntohl( tagHeaderSize + 4 );
    fwrite( &bitEndianTagSize, 4, 1, file );
    return tagSize + 4;
}
}; // namespace nx

FlvMuxer::FlvMuxer( const char *filePath, bool hasAudio, bool hasVideo ) {
    flv_header_t flv_header = flv_header_t( hasAudio, hasVideo );
    flvFile                 = fopen( filePath, "wb" );
}

void FlvMuxer::mux_aac( uint8_t *adts, size_t length, uint32_t timestamp ) {

    if ( !aacSequenceHeaderFlag ) {
        // write aac sequence header
        aacSequenceHeaderFlag = true;
        uint8_t *adtsHeader   = adts;

        uint8_t profile                = ( ( adtsHeader[2] >> 6 ) & 0x03 ) + 1;
        uint8_t sample_frequence_index = ( adtsHeader[2] >> 2 ) & 0x0F;
        uint8_t channel_configuration  = ( ( adtsHeader[2] & 0x01 ) << 2 ) | ( ( adts[3] >> 6 ) & 0x03 );

        flv_aac_audio_specific_config_tag_t audioSpecificConfigTag = flv_aac_audio_specific_config_tag_t( profile, sample_frequence_index, channel_configuration, timestamp );
        flv_write_aac_audio_specific_config_tag( flvFile, audioSpecificConfigTag );
        return;
    }

    // write aac raw
    flv_aac_raw_tag_t rawTag = flv_aac_raw_tag_t( adts + 7, length - 7, timestamp );
    flv_write_aac_raw_tag( flvFile, rawTag );
}