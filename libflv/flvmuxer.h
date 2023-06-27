#ifndef __FLVMUXER_H__
#define __FLVMUXER_H__

#include "avc.h"
namespace nx {

struct FlvMetaData {

    bool hasAudio = false;
    bool hasVideo = false;

    // Total duration of the file in seconds
    double duration;
    // Total size of the file in bytes
    double filesize;
    /*
    0 = Linear PCM, platform endian
    1 = ADPCM
    2 = MP3
    3 = Linear PCM, little endian 4 = Nellymoser 16 kHz mono 5 = Nellymoser 8 kHz mono 6 = Nellymoser
    7 = G.711 A-law logarithmic PCM
    8 = G.711 mu-law logarithmic PCM
    9 = reserved
    10 = AAC
    11 = Speex
    14 = MP3 8 kHz
    15 = Device-specific sound
    */
    double audiocodecid = 10;
    // Audio bit rate in kilobits per second
    double audiodatarate;
    // Resolution of a single audio sample
    double audiosamplesize;
    // indicating stereo audio
    bool stereo;
    // Delay introduced by the audio codec in seconds
    double audiodelay = 0;
    // Frequency at which the audio stream is replayed
    double audiosamplerate;
    /*
    2 = Sorenson H.263
    3 = Screen video
    4 = On2 VP6
    5 = On2 VP6 with alpha channel 6 = Screen video version 2
    7 = AVC
    */
    double videocodecid = 7;
    // Number of frames per second
    double framerate;
    // Width of the video in pixels
    double width;
    // Height of the video in pixels
    double height;
    // Video bit rate in kilobits per second
    double videodatarate;
};

class FlvMuxer {
private:
    FlvMetaData metaData;

    bool hasVideo = false;
    bool hasAudio = false;

    uint32_t audioStartTimestamp = 0;
    uint32_t lastAudioTimestamp  = 0;

    uint32_t videoStartTimestamp = 0;
    uint32_t lastVideoTimestamp  = 0;

    int64_t totalBytes = 0;

    FILE *flvFile = nullptr;

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
     * @param timestamp  timestamp, audio is pts, video is dts
     */
    void onMuxedData( int type, const uint8_t *data, size_t bytes, uint32_t timestamp );

    void mux_metadata();

    void endMuxing();

public:
    ~FlvMuxer();

    FlvMuxer( const char *filePath, bool hasAudio, bool hasVideo );
    /**
     * @brief mux aac adts data
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
     * @param dts  dts of this buffer
     * @param isKeyFrame  whether buf is keyFrame or not
     */
    void mux_avc( uint8_t *buf, size_t length, uint32_t pts, uint32_t dts, bool isKeyFrame );
};

} // namespace nx

#endif // __FLVMUXER_H__
