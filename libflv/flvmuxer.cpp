
#include "flvmuxer.h"
#include <cassert>
#include <vector>

#include "aac.h"
#include "amf.h"
#include "flv_tag.h"

using namespace nx;
using namespace std;

namespace nx {

/**
 * @brief construct metadata tag buf, with 4 bytes tag size
 *
 * @param metaData  FlvMetaData
 * @return vector<uint8_t>  tag buf
 */
static vector<uint8_t> metadata_to_buf( FlvMetaData &metaData ) {
    vector<uint8_t> dst_buf;

    AMF_BUFFER elems;
    uint32_t   count = 0;
    {
        amf_put_named_double( "duration", metaData.duration, elems );
        count += 1;
    }
    {
        amf_put_named_double( "filesize", metaData.filesize, elems );
        count += 1;
    }
    if ( metaData.hasAudio ) {
        amf_put_named_double( "audiocodecid", metaData.audiocodecid, elems );
        amf_put_named_double( "audiosamplerate", metaData.audiosamplerate, elems );
        amf_put_named_bool( "stereo", metaData.stereo, elems );
        amf_put_named_double( "audiodelay", metaData.audiodelay, elems );
        count += 4;
    }

    if ( metaData.hasVideo ) {
        amf_put_named_double( "videocodecid", metaData.videocodecid, elems );
        count += 1;
    }

    AMF_BUFFER amf_buf;
    {
        amf_put_named_ecma_array( "onMetadata", count, elems, amf_buf );
    }
    // construct flv tag
    const int      flv_tag_header_size                 = 11;
    uint32_t       dataSize                            = (uint32_t)amf_buf.size();
    uint32_t       TagSize                             = flv_tag_header_size + dataSize;
    flv_tag_header header                              = flv_tag_header( flv_tag_header::TagType::script_data, dataSize, 0 );
    uint8_t        flv_header_buf[flv_tag_header_size] = { 0 };
    header.to_buf( flv_header_buf );
    {
        // tag header
        for ( int i = 0; i < flv_tag_header_size; i++ ) {
            dst_buf.push_back( flv_header_buf[i] );
        }
        // tag data
        dst_buf.insert( dst_buf.end(), amf_buf.begin(), amf_buf.end() );
        // tag size
        uint8_t *size = (uint8_t *)&TagSize;
        for ( int i = 3; i >= 0; i-- ) {
            dst_buf.push_back( *( size + i ) );
        }
    }
    return dst_buf;
}

}; // namespace nx

FlvMuxer::~FlvMuxer() {
    if ( sps ) delete sps;
    if ( pps ) delete pps;
    endMuxing();
}

FlvMuxer::FlvMuxer( const char *filePath, bool hasAudio, bool hasVideo ) {
    flvFile = fopen( filePath, "wb" );
    if ( !flvFile ) {
        // TODO: handle create file error
        return;
    }

    assert( hasAudio || hasVideo );
    if ( !hasAudio && !hasVideo ) return;

    this->hasAudio = hasAudio;
    this->hasVideo = hasVideo;

    metaData.hasAudio = hasAudio;
    metaData.hasVideo = hasVideo;

    flv_header header = flv_header( hasAudio, hasVideo );
    uint8_t    buf[9] = { 0 };
    header.to_buf( buf );
    // write flv_header
    fwrite( buf, sizeof( buf ), 1, flvFile );
    // write tag 0 size
    uint32_t tagSize = 0;
    fwrite( &tagSize, sizeof( tagSize ), 1, flvFile );

    totalBytes += 13;

    // write metadata
    mux_metadata();
}

void FlvMuxer::mux_aac( uint8_t *adts, size_t length, uint32_t timestamp ) {

    if ( !this->hasAudio ) return;

    // update timestamp
    if ( !this->audioStartTimestamp ) this->audioStartTimestamp = timestamp;
    this->lastAudioTimestamp = timestamp;

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

        // update metadata
        {
            adts_header header       = adts_header::parse_adts_header( adts );
            metaData.audiosamplerate = adts_header::sampleRate( header );
            metaData.stereo          = adts_header::channelCount( header ) == 2 ? 1 : 0;
        }
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
    // update timestamp
    if ( !this->videoStartTimestamp ) this->videoStartTimestamp = dts;
    this->lastVideoTimestamp = dts;

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

    size_t ret = fwrite( data, 1, bytes, flvFile );
    assert( ret == bytes );
    if ( ret < bytes ) {
        // handle write error
        return;
    }

    totalBytes += bytes;
}

void FlvMuxer::mux_metadata() {
    vector<uint8_t> buf = metadata_to_buf( metaData );
    // callback
    this->onMuxedData( flv_tag_header::TagType::script_data, &buf[0], buf.size(), 0 );
}

void FlvMuxer::endMuxing() {
    if ( !flvFile ) return;

    // write eos
    if ( this->hasVideo ) {
        const uint32_t     flv_tag_header_size = 11;
        const uint32_t     flv_avc_header_size = 5;
        const int          dataSize            = flv_avc_header_size;
        const int          TagSize             = flv_tag_header_size + dataSize;
        flv_avc_tag_header avcTagHeader        = flv_avc_tag_header( flv_avc_tag_header::FrameType::AVCKeyFrame, flv_avc_tag_header::AVCPacketType::AVCEndOfSequence, 0 );
        flv_tag_header     tagHeader           = flv_tag_header( flv_tag_header::TagType::video, dataSize, this->lastVideoTimestamp );

        const int buf_size = TagSize + 4;
        uint8_t  *buf      = (uint8_t *)calloc( buf_size, 1 );
        int       offset   = 0;
        // tag header
        tagHeader.to_buf( buf + offset );
        offset += flv_tag_header_size;
        // avc tag header
        avcTagHeader.to_buf( buf + offset );
        offset += flv_avc_header_size;
        // write tag size, big endian
        uint32_t size = htonl( TagSize );
        memcpy( buf + offset, &size, 4 );
        // callback
        this->onMuxedData( flv_tag_header::TagType::video, buf, buf_size, 0 );
        free( buf );
    }
    // update meta data
    {
        uint32_t videoDuration       = lastVideoTimestamp - videoStartTimestamp;
        uint32_t audioDuration       = lastAudioTimestamp - audioStartTimestamp;
        metaData.duration            = std::max( videoDuration, audioDuration ) / 1000;
        metaData.filesize            = totalBytes;
        const long offsetOfScriptTag = 9 + 4;
        int        ret               = fseek( flvFile, offsetOfScriptTag, SEEK_SET );
        assert( ret == 0 );
        vector<uint8_t> buf   = metadata_to_buf( metaData );
        size_t          bytes = fwrite( &buf[0], 1, buf.size(), flvFile );
        assert( bytes == buf.size() );
    }
    // close file
    fclose( flvFile );
    flvFile = nullptr;
}
