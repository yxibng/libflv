#ifndef __FLVMUXER_H__
#define __FLVMUXER_H__

#include "avc.h"
#include "flv-tag.h"

namespace nx {
class FlvMuxer {

public:
    struct Buffer {
        uint8_t *buf;
        uint32_t size;
        Buffer( uint8_t *buf, uint32_t size )
            : buf( (uint8_t *)malloc( size ) ), size( size ) {
            memcpy( this->buf, buf, size );
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

private:
    bool  hasVideo;
    bool  hasAudio;
    FILE *flvFile;

    bool aacSequenceHeaderFlag = false;
    bool avcSequenceHeaderFlag = false;

    Buffer *sps = nullptr;
    Buffer *pps = nullptr;

public:
    ~FlvMuxer();

    FlvMuxer( const char *filePath, bool hasAudio, bool hasVideo );
    /**
     * @brief
     *
     * @param adts  aac with adts header
     * @param length length of the adts buffer
     * @param timestamp  timestamp of this buffer
     */
    void mux_aac( uint8_t *adts, size_t length, uint32_t timestamp );
    /**
     * @brief
     *
     * @param buf h264 annex-b buffer, seperated by 00 00 00 01
     * @param length  length of the  h264 buf
     * @param timestamp  timestamp of this buffer
     */
    void mux_avc( uint8_t *buf, size_t length, uint32_t timestamp );
};

} // namespace nx

#endif // __FLVMUXER_H__
