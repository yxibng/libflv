
#include "flvmuxer.h"
#include <cassert>
#include <vector>

using namespace nx;
using namespace std;

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

int flv_write_avc_sequence_header_tag( FILE *file, const flv_avc_sequence_header_tag_t &avcSequenceHeaderTag ) {

    return 0;
}

// find 00 00 01
uint8_t *avc_find_startcode( uint8_t *p, uint8_t *end ) {
    intptr_t count        = end - p + 1;
    uint8_t  startCode[3] = { 0, 0, 1 };
    int      offset       = 0;
    while ( count >= 3 && offset < count ) {
        if ( startCode[0] == p[offset] &&
             startCode[1] == p[offset + 1] &&
             startCode[2] == p[offset + 2] ) {
            return p;
        }
        else {
            offset++;
        }
    }
    return NULL;
}

void split_nalus( uint8_t *buf, uint32_t size, vector<FlvMuxer::Buffer> &nalus ) {

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

vector<uint8_t> avc_decoder_congiguraton_record( FlvMuxer::Buffer *sps, FlvMuxer::Buffer *pps ) {
    vector<uint8_t> buf;

    H264SPS h264SPS;
    int     ret = avc_decode_sps( &h264SPS, sps->buf, sps->size );
    if ( ret < 0 ) return buf;

    // configurationVersion
    uint8_t version               = 1;
    uint8_t profile               = h264SPS.profile_idc;
    uint8_t profile_compatibility = h264SPS.compatibility;

    buf.push_back( version );
    buf.push_back( profile );
    buf.push_back( profile_compatibility );

    // 6bits reserved 111111 + 2 bits lengthSizeMinusOne
    uint8_t NALUnitLength      = 4;
    uint8_t lengthSizeMinusOne = ( 0b111111 << 2 ) | ( ( NALUnitLength - 1 ) & 0b11 );

    buf.push_back( lengthSizeMinusOne );

    // 3bit reserved 111 + 5bits numOfSequenceParameterSets
    uint8_t numOfSequenceParameterSets = ( 0b111 << 5 ) | 0b00001;
    buf.push_back( numOfSequenceParameterSets );

    // sps length, big endian
    buf.push_back( ( sps->size >> 8 ) & 0XFF );
    buf.push_back( sps->size & 0XFF );
    // sps buf
    for ( int i = 0; i < sps->size; i++ ) {
        buf.push_back( *( sps->buf ) );
    }
    // pps length, big endian
    buf.push_back( ( pps->size >> 8 ) & 0XFF );
    buf.push_back( pps->size & 0XFF );
    // pps buf
    for ( int i = 0; i < pps->size; i++ ) {
        buf.push_back( *( pps->buf ) );
    }

    if ( profile == 100 || profile == 110 ||
         profile == 122 || profile == 244 || profile == 44 ||
         profile == 83 || profile == 86 || profile == 118 ||
         profile == 128 || profile == 138 || profile == 139 ||
         profile == 134 ) {
        // 6 bits reserved 111111, 2 bits chroma_format
        uint8_t chroma_format = ( 0b111111 << 2 ) | ( h264SPS.chroma_format_idc & 0b11 );
        // 5 bits reserved 11111, 3 bits bit_depth_luma_minus8
        uint8_t bit_depth_luma_minus8 = ( 0b11111 << 3 ) | ( ( h264SPS.bit_depth_luma - 8 ) & 0b111 );
        // 5 bits reserved 11111, 3 bits bit_depth_chroma_minus8
        uint8_t bit_depth_chroma_minus8 = ( 0b11111 << 3 ) | ( ( h264SPS.bit_depth_chroma - 8 ) & 0b111 );
        // numOfSequenceParameterSetExt
        uint8_t numOfSequenceParameterSetExt = 0;

        buf.push_back( chroma_format );
        buf.push_back( bit_depth_luma_minus8 );
        buf.push_back( bit_depth_chroma_minus8 );
        buf.push_back( numOfSequenceParameterSetExt );
    }
    return buf;
}

}; // namespace nx

FlvMuxer::~FlvMuxer() {
    if ( sps ) delete sps;
    if ( pps ) delete pps;
}

FlvMuxer::FlvMuxer( const char *filePath, bool hasAudio, bool hasVideo ) {
    flv_header_t flv_header = flv_header_t( hasAudio, hasVideo );
    flvFile                 = fopen( filePath, "wb" );
}

void FlvMuxer::mux_aac( uint8_t *adts, size_t length, uint32_t timestamp ) {

    assert( length > 7 );
    if ( !aacSequenceHeaderFlag ) {
        // write aac sequence header
        aacSequenceHeaderFlag = true;
        uint8_t *adtsHeader   = adts;

        uint8_t                             profile                = ( ( adtsHeader[2] >> 6 ) & 0x03 ) + 1;
        uint8_t                             sample_frequence_index = ( adtsHeader[2] >> 2 ) & 0x0F;
        uint8_t                             channel_configuration  = ( ( adtsHeader[2] & 0x01 ) << 2 ) | ( ( adts[3] >> 6 ) & 0x03 );
        flv_aac_audio_specific_config_tag_t audioSpecificConfigTag = flv_aac_audio_specific_config_tag_t( profile, sample_frequence_index, channel_configuration, timestamp );
        flv_write_aac_audio_specific_config_tag( flvFile, audioSpecificConfigTag );
    }
    // write aac raw
    flv_aac_raw_tag_t rawTag = flv_aac_raw_tag_t( adts + 7, length - 7, timestamp );
    flv_write_aac_raw_tag( flvFile, rawTag );
}

void FlvMuxer::mux_avc( uint8_t *buf, size_t length, uint32_t timestamp ) {

    /*
        1. seperate buf to nalus
        2. for each nalu, extract rbsp from nalu
        3. check nalu type to get sps, pps, generate avc sequence header
        4. write avc tags
    */
    vector<Buffer> nalus;
    split_nalus( buf, length, nalus );
    if ( nalus.empty() ) return;

    if ( !avcSequenceHeaderFlag ) {
        // write avc sequence header
        for ( auto it = nalus.begin(); it != nalus.end(); it++ ) {
            uint8_t *nalu     = it->buf;
            uint8_t  naluType = ( *nalu ) & 0b11111;
            if ( naluType == NaluType::SPS ) {
                if ( !this->sps ) {
                    this->sps = new Buffer( it->buf, it->size );
                }
            }
            else if ( naluType == NaluType::PPS ) {
                if ( !this->pps ) {
                    this->pps = new Buffer( it->buf, it->size );
                }
            }
        }

        if ( this->sps && this->pps ) {
            // construct avc sequence header
            vector<uint8_t>  avcAudioDecoderConfigurationRecord = avc_decoder_congiguraton_record( this->sps, this->pps );
            const uint32_t   avcSequenceHeaderSize              = 2;
            flv_tag_header_t tagHeader;
            tagHeader.tagType   = flv_tag_header_t::TagType::video;
            tagHeader.timestamp = timestamp;
            tagHeader.dataSize  = avcSequenceHeaderSize + avcAudioDecoderConfigurationRecord.size();
            // write tag header
            flv_write_tag_header( flvFile, tagHeader );
            // write avc tag header
            {
                // 4bits frametype = 1, 4bits codecID = 7
                uint8_t buf = ( 1 << 4 ) | 7;
                fwrite( &buf, 1, 1, flvFile );
                // avcpacket type = 0
                uint8_t avcPacketType = 0;
                fwrite( &avcPacketType, 1, 1, flvFile );
            }

            // write AvcAudioDecoderConfigurationRecord
            for ( uint8_t val : avcAudioDecoderConfigurationRecord ) {
                fwrite( &val, 1, 1, flvFile );
            }
            avcSequenceHeaderFlag = true;
        }
        // write nalu,annex-b to mp4

        return;
    }
}
