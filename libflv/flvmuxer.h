#ifndef __FLVMUXER_H__
#define __FLVMUXER_H__

#include "avc.h"
#include "flv-tag.h"

namespace nx {
class FlvMuxer {
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
     * @param isKeyFrame  whether buf is keyFrame or not
     */
    void mux_avc( uint8_t *buf, size_t length, uint32_t timestamp, bool isKeyFrame );
};

} // namespace nx

#endif // __FLVMUXER_H__
