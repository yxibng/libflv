#ifndef __FLVMUXER_H__
#define __FLVMUXER_H__

#include "flv-tag.h"

namespace nx {
class FlvMuxer {
private:
    bool  hasVideo;
    bool  hasAudio;
    FILE *flvFile;

    bool aacSequenceHeaderFlag = false;
    bool avcSequenceHeaderFlag = false;

public:
    FlvMuxer( const char *filePath, bool hasAudio, bool hasVideo );
    void mux_aac( uint8_t *adts, size_t length, uint32_t timestamp );
};

} // namespace nx

#endif // __FLVMUXER_H__
