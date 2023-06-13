//
//  flv-header.h
//  libflv
//
//  Created by yxibng on 2023/6/9.
//

#ifndef flv_header_h
#define flv_header_h

#include <cstdint>
#include <stdio.h>

struct flv_header_t {
    const uint8_t FLV[3]   = { 'F', 'L', 'V' };
    uint8_t       version  = 1;
    bool          hasAudio = false;
    bool          hasVideo = false;
    uint32_t      offset   = 9;
    flv_header_t( bool hasAudio, bool hasVideo )
        : hasAudio( hasAudio ), hasVideo( hasVideo ) {}
};

struct flv_tag_header_t {

    enum TagType {
        audio       = 8,
        video       = 9,
        script_data = 18
    };

    bool         encrypted = false;
    enum TagType tagType;
    // Length of the message. Number of bytes after StreamID to end of tag (Equal to length of the tag – 11)
    uint32_t dataSize;
    // Time in milliseconds at which the data in this tag applies. This value is relative to the first tag in the FLV file, which always has a timestamp of 0.
    // Extension of the Timestamp field to form a SI32 value. This field represents the upper 8 bits, while the previous Timestamp field represents the lower 24 bits of the time in milliseconds.
    uint32_t timestamp;
    // Extension of the Timestamp field to form a SI32 value. This field represents the upper 8 bits, while the previous Timestamp field represents the lower 24 bits of the time in milliseconds.
    const uint8_t streamID[3] = { 0, 0, 0 };
};

struct flv_audio_tag_header_t : flv_tag_header_t {

    enum SoundFormat {
        platformEndianPcm = 0,
        littleEndianPcm   = 3,
        aac               = 10,
        speex             = 11,
        mp3_8khz          = 14,
    };

    enum SoundRate {
        rate5_5khz = 0,
        rate11khz  = 1,
        rate22khz  = 2,
        rate44khz  = 3,
    };

    enum SoundSize {
        bits8  = 0,
        bits16 = 1
    };

    enum SoundType {
        mono   = 0,
        stereo = 1
    };

    enum AACPacketType {
        sequenceHeader = 0,
        raw            = 1
    };

    enum SoundFormat soundFormat = aac;
    enum SoundRate   soundRate   = rate44khz;
    enum SoundSize   soundSize   = bits16;
    enum SoundType   soundType   = stereo;
    AACPacketType    aacPacketType;
};

struct flv_video_tag_header : flv_tag_header_t {
    /*
     Type of video frame. The following values are defined:
     1 = key frame (for AVC, a seekable frame)
     2 = inter frame (for AVC, a non-seekable frame)
     3 = disposable inter frame (H.263 only)
     4 = generated key frame (reserved for server use only)
     5 = video info/command frame
     */
    enum FrameType {
        KeyFrame             = 1,
        InterFrame           = 2,
        DisposableInterFrame = 3,
        GeneratedKeyFrame    = 4,
        CommandFrame         = 5
    };

    /*
     Codec Identifier. The following values are defined: 2 = Sorenson H.263
     3 = Screen video
     4 = On2 VP6
     5 = On2 VP6 with alpha channel 6 = Screen video version 2
     7 = AVC
     */
    enum CodecID {
        SorensonH263           = 2,
        ScreenVideo            = 3,
        On2VP6                 = 4,
        On2VP6WithAlphaChannel = 5,
        ScreenVideo2           = 6,
        AVC                    = 7
    };

    /*
     The following values are defined:
     0 = AVC sequence header
     1 = AVC NALU
     2 = AVC end of sequence (lower level NALU sequence ender is not required or supported)
     */
    enum AVCPacketType {
        AVCSequenceHeader = 0,
        AVCNALU           = 1,
        AVCEndOfSequence  = 2
    };

    FrameType     frameType;
    CodecID       codecID;
    AVCPacketType packetType;
    /*
     IF AVCPacketType == 1
        Composition time offset
     ELSE
        0
     */
    int32_t compositionTime;
};

#endif /* flv_header_h */
