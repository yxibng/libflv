#ifndef __FLVMUXER_H__
#define __FLVMUXER_H__

#include "avc.h"
namespace nx {
class FlvMuxer {
private:
    bool  hasVideo;
    bool  hasAudio;
    FILE *flvFile;

    bool aacSequenceHeaderFlag = false;
    bool avcSequenceHeaderFlag = false;

    NaluBuffer *sps = nullptr;
    NaluBuffer *pps = nullptr;
    /**
     * @brief mux data call back
     *
     * @param type  8 - audio, 9 - video, 18 - script data
     * @param data  buf pointer
     * @param bytes  but bytes
     * @param data  timestamp, audio is pts, video is dts
     */
    void onMuxedData( int type, const uint8_t *data, size_t bytes, uint32_t timestamp );

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
     * @brief mux h264 annex-b frame, which is seperated by start code 00 00 00 01.
     * The key frame should contain sps, pps, and IDR nalus.
     *
     * @param buf h264 annex-b buffer, seperated by 00 00 00 01
     * @param length  length of the  h264 buf
     * @param pts  pts of this buffer
     * @param pts  dts of this buffer
     * @param isKeyFrame  whether buf is keyFrame or not
     */
    void mux_avc( uint8_t *buf, size_t length, uint32_t pts, uint32_t dts, bool isKeyFrame );
};

} // namespace nx

#endif // __FLVMUXER_H__
