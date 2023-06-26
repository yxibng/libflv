
#include "flvmuxer.h"
#include <cassert>
#include <vector>

#include "aac.h"
#include "flv_tag.h"

using namespace nx;
using namespace std;

FlvMuxer::~FlvMuxer() {
    if ( sps ) delete sps;
    if ( pps ) delete pps;
}

FlvMuxer::FlvMuxer( const char *filePath, bool hasAudio, bool hasVideo ) {
    flvFile = fopen( filePath, "wb" );
    if ( !flvFile ) {
        // TODO: handle create file error
        return;
    }

    this->hasAudio = hasAudio;
    this->hasVideo = hasVideo;

    flv_header header = flv_header( hasAudio, hasVideo );
    uint8_t    buf[9] = { 0 };
    header.to_buf( buf );
    // write flv_header
    fwrite( buf, sizeof( buf ), 1, flvFile );
    // write tag 0 size
    uint32_t tagSize = 0;
    fwrite( &tagSize, sizeof( tagSize ), 1, flvFile );
}

void FlvMuxer::mux_aac( uint8_t *adts, size_t length, uint32_t timestamp ) {

    if ( !this->hasAudio ) return;

    const int adtsHeaderSize     = 7;
    const int flvTagHeaderSize   = 11;
    const int audioTagHeaderSize = 2;

    assert( length > adtsHeaderSize );
    if ( !aacSequenceHeaderFlag ) {
        const int audioSpecificConfigSize = 2;
        uint32_t  dataSize                = audioTagHeaderSize + audioSpecificConfigSize;
        const int TagSize                 = flvTagHeaderSize + dataSize;
        const int buf_size                = TagSize + 4;
        int       offset                  = 0;

        uint8_t *buf = (uint8_t *)calloc( buf_size, 1 );
        if ( !buf ) return; // no memory
        flv_tag_header tagHeader = flv_tag_header( flv_tag_header::TagType::audio, dataSize, timestamp );
        tagHeader.to_buf( buf + offset );
        offset += flvTagHeaderSize;
        // AAC sequence header
        flv_aac_audio_tag_header audioTagHeader = flv_aac_audio_tag_header( flv_aac_audio_tag_header::AACSequenceHeader );
        audioTagHeader.to_buf( buf + offset );
        offset += audioTagHeaderSize;
        // AudioSpecificConfig
        AudioSpecificConfig config = AudioSpecificConfig( adts );
        config.to_buf( buf + offset );
        offset += audioSpecificConfigSize;
        // write tag size, big endian
        uint32_t size = htonl( TagSize );
        memcpy( buf + offset, &size, 4 );
        // update flag
        aacSequenceHeaderFlag = true;
        // callback
        this->onMuxedData( flv_tag_header::TagType::audio, buf, buf_size, timestamp );
        free( buf );
    }
    else {
        // write aac raw
        const int aacRawSize = (int)length - adtsHeaderSize;
        uint32_t  dataSize   = audioTagHeaderSize + aacRawSize;
        const int TagSize    = flvTagHeaderSize + dataSize;
        const int buf_size   = TagSize + 4;
        int       offset     = 0;

        uint8_t *buf = (uint8_t *)calloc( buf_size, 1 );
        if ( !buf ) return; // no memory
        // flv tag header
        flv_tag_header tagHeader = flv_tag_header( flv_tag_header::TagType::audio, dataSize, timestamp );
        tagHeader.to_buf( buf + offset );
        offset += flvTagHeaderSize;
        // audio tag header
        flv_aac_audio_tag_header audioTagHeader = flv_aac_audio_tag_header( flv_aac_audio_tag_header::AACRaw );
        audioTagHeader.to_buf( buf + offset );
        offset += audioTagHeaderSize;
        // aac raw data
        memcpy( buf + offset, adts + adtsHeaderSize, aacRawSize );
        offset += aacRawSize;
        // tag size, big endian
        uint32_t size = htonl( TagSize );
        memcpy( buf + offset, &size, 4 );
        // callback
        this->onMuxedData( flv_tag_header::TagType::audio, buf, buf_size, timestamp );
        free( buf );
    }
}

void FlvMuxer::mux_avc( uint8_t *buf, size_t length, uint32_t pts, uint32_t dts, bool isKeyFrame ) {

    if ( !this->hasVideo ) return;
    /*
        1. seperate buf to nalus
        2. for each nalu, extract rbsp from nalu
        3. check nalu type to get sps, pps, generate avc sequence header
        4. write avc tags
    */
    vector<NaluBuffer> nalus;
    split_nalus( buf, (uint32_t)length, nalus );
    if ( nalus.empty() ) return;
    const uint32_t flv_tag_header_size = 11;
    const uint32_t flv_avc_header_size = 5;
    if ( !avcSequenceHeaderFlag ) {
        // write avc sequence header
        for ( auto it = nalus.begin(); it != nalus.end(); it++ ) {
            uint8_t *nalu     = it->buf;
            uint8_t  naluType = ( *nalu ) & 0x1F;
            if ( naluType == NaluType::SPS ) {
                if ( !this->sps ) {
                    assert( buf != NULL );
                    this->sps = new NaluBuffer( it->buf, it->size );
                }
            }
            else if ( naluType == NaluType::PPS ) {
                if ( !this->pps ) {
                    this->pps = new NaluBuffer( it->buf, it->size );
                }
            }
        }
        if ( this->sps && this->pps ) {

            AVCDecoderConfigurationRecord avcDecoderConfigurationRecord = AVCDecoderConfigurationRecord(
                sps->buf, sps->size,
                pps->buf, pps->size );
            vector<uint8_t> avc_sequence_header_buf = avcDecoderConfigurationRecord.to_buf();

            flv_avc_tag_header avc_tag_header                       = flv_avc_tag_header( flv_avc_tag_header::AVCKeyFrame, flv_avc_tag_header::AVCSequenceHeader, pts - dts );
            uint8_t            avcTagHeaderBuf[flv_avc_header_size] = { 0 };
            avc_tag_header.to_buf( avcTagHeaderBuf );

            // flv tag header
            const uint32_t dataSize                             = flv_avc_header_size + (uint32_t)avc_sequence_header_buf.size();
            flv_tag_header flvTagHeader                         = flv_tag_header( flv_tag_header::TagType::video, dataSize, dts );
            uint8_t        flvTagHeaderBuf[flv_tag_header_size] = { 0 };
            flvTagHeader.to_buf( flvTagHeaderBuf );

            const uint32_t TagSize  = flv_tag_header_size + dataSize;
            const int      buf_size = TagSize + 4;
            uint8_t       *buf      = (uint8_t *)calloc( buf_size, 1 );
            if ( !buf ) return; // no memory

            int offset = 0;
            memcpy( buf + offset, flvTagHeaderBuf, flv_tag_header_size );
            offset += flv_tag_header_size;
            memcpy( buf + offset, avcTagHeaderBuf, flv_avc_header_size );
            offset += flv_avc_header_size;
            memcpy( buf + offset, &avc_sequence_header_buf[0], avc_sequence_header_buf.size() );
            offset += avc_sequence_header_buf.size();
            // write tag size, big endian
            uint32_t size = htonl( TagSize );
            memcpy( buf + offset, &size, 4 );
            // update flag
            avcSequenceHeaderFlag = true;
            // callback
            this->onMuxedData( flv_tag_header::TagType::video, buf, buf_size, dts );
            free( buf );
        }
    }

    if ( !avcSequenceHeaderFlag ) return;

    {
        // write nalu,annex-b to mp4
        vector<uint8_t> mp4_nulus;
        uint32_t        frameSize = 0;
        for ( auto it = nalus.begin(); it != nalus.end(); it++ ) {
            frameSize += it->size + 4;
        }

        // flv tag header
        const uint32_t dataSize = flv_avc_header_size + frameSize;
        const uint32_t TagSize  = flv_tag_header_size + dataSize;
        const int      buf_size = TagSize + 4;
        uint8_t       *buf      = (uint8_t *)calloc( buf_size, 1 );
        if ( !buf ) return; // no memory

        int            offset                               = 0;
        flv_tag_header flvTagHeader                         = flv_tag_header( flv_tag_header::TagType::video, dataSize, dts );
        uint8_t        flvTagHeaderBuf[flv_tag_header_size] = { 0 };
        flvTagHeader.to_buf( flvTagHeaderBuf );

        memcpy( buf + offset, flvTagHeaderBuf, flv_tag_header_size );
        offset += flv_tag_header_size;

        // avc tag header
        flv_avc_tag_header avc_tag_header                       = flv_avc_tag_header( isKeyFrame ? flv_avc_tag_header::AVCKeyFrame : flv_avc_tag_header::AVCInterFrame, flv_avc_tag_header::AVCNALU, pts - dts );
        uint8_t            avcTagHeaderBuf[flv_avc_header_size] = { 0 };
        avc_tag_header.to_buf( avcTagHeaderBuf );

        memcpy( buf + offset, avcTagHeaderBuf, flv_avc_header_size );
        offset += flv_avc_header_size;

        // mp4 format nalus
        for ( auto it = nalus.begin(); it != nalus.end(); it++ ) {
            uint32_t size = htonl( it->size );
            memcpy( buf + offset, &size, 4 );
            offset += 4;
            memcpy( buf + offset, it->buf, it->size );
            offset += it->size;
        }
        // write tag size, big endian
        uint32_t size = htonl( TagSize );
        memcpy( buf + offset, &size, 4 );
        // callback
        this->onMuxedData( flv_tag_header::TagType::video, buf, buf_size, dts );

        free( buf );
    }
}

void FlvMuxer::onMuxedData( int type, const uint8_t *data, size_t bytes, uint32_t timestamp ) {
    size_t ret = fwrite( data, bytes, 1, flvFile );
    if ( ret < bytes ) {
        // handle write error
    }
}
