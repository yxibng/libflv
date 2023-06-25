#ifndef __FLV_TAG_H__
#define __FLV_TAG_H__

#include "put_bits.h"

namespace nx {

/**
 * @brief flv_header
 * When write to file, flv_header is 9 bytes.
 */
struct flv_header {
    unsigned char Signature[3] = { 'F', 'L', 'V' };
    unsigned char version      = 1;
    unsigned char : 5; // Reserved, Shall be 0
    unsigned char audioFlag : 1;
    unsigned char : 1; // Reserved, Shall be 0
    unsigned char videoFlag : 1;
    unsigned int  dataOffset = 9;
    flv_header( bool hasAudio, bool hasVideo ) {
        audioFlag = hasAudio ? 1 : 0;
        videoFlag = hasVideo ? 1 : 0;
    }
    void to_buf( unsigned char buf[9] ) {
        PutBitsContext context = PutBitsContext( buf, 9 );
        context.put_bits( 8, Signature[0] );
        context.put_bits( 8, Signature[1] );
        context.put_bits( 8, Signature[2] );
        context.put_bits( 8, version );
        uint8_t reserved = 0;
        context.put_bits( 5, reserved );
        context.put_bits( 1, audioFlag );
        context.put_bits( 1, reserved );
        context.put_bits( 1, videoFlag );
        // big endian dataOffset
        context.put_bits( 8, dataOffset >> 24 & 0xFF );
        context.put_bits( 8, dataOffset >> 16 & 0xFF );
        context.put_bits( 8, dataOffset >> 8 & 0xFF );
        context.put_bits( 8, dataOffset & 0xFF );
    }
};
/**
 * @brief flv_tag_header
 *  When write to file, flv_tag_header is 11 bytes.
 */
struct flv_tag_header {
    enum TagType {
        audio       = 8,
        video       = 9,
        script_data = 18
    };

    unsigned int : 2;        // Reserved, Shall be 0
    unsigned int filter : 1; // encrypted or not
    /*
    8 = audio
    9 = video
    18 = script data
    */
    unsigned int tagType : 5;
    // Number of bytes after StreamID to end of tag (Equal to length of the tag – 11)
    unsigned int dataSize : 24;

    unsigned int timestamp;
    // Always 0.
    unsigned char streamID[3] = { 0 };

    flv_tag_header( TagType tagType, uint32_t dataSize, uint32_t timestamp ) {
        // not encrypted
        this->filter    = 0;
        this->tagType   = tagType;
        this->dataSize  = dataSize;
        this->timestamp = timestamp;
    }

    void to_buf( uint8_t buf[11] ) {
        PutBitsContext context  = PutBitsContext( buf, 11 );
        uint8_t        reserved = 0;
        context.put_bits( 2, reserved );
        context.put_bits( 1, filter );
        context.put_bits( 5, tagType );
        // bit endian data size
        context.put_bits( 8, dataSize >> 16 & 0xFF );
        context.put_bits( 8, dataSize >> 8 & 0xFF );
        context.put_bits( 8, dataSize & 0xFF );
        // timestamp, low 24 bits, big endian
        context.put_bits( 8, timestamp >> 16 & 0xFF );
        context.put_bits( 8, timestamp >> 8 & 0xFF );
        context.put_bits( 8, timestamp & 0xFF );
        // TimestampExtended
        context.put_bits( 8, timestamp >> 24 & 0xFF );
        // streamID
        context.put_bits( 8, streamID[0] );
        context.put_bits( 8, streamID[1] );
        context.put_bits( 8, streamID[2] );
    }
};

/**
 * @brief aac audio tag header.
 * When write to file, flv_aac_audio_tag_header is 2 bytes.
 */
struct flv_aac_audio_tag_header {
    // 10 = aac
    enum SoundFormat {
        AAC = 10
    };
    unsigned char soundFormat : 4;
    /*
    aac is always 3.
    0 = 5.5 kHz
    1 = 11 kHz
    2 = 22 kHz
    3 = 44 kHz
    */
    enum SoundRate {
        Rate5_5 = 0,
        Rate11,
        Rate22,
        Rate44 = 3,
    };
    unsigned char soundRate : 2;
    /*
    Compressed formats always decode to 16 bits internally.
    0 = 8-bit samples
    1 = 16-bit samples
    */
    enum SoundSize {
        Bits8  = 0,
        Bits16 = 1
    };
    unsigned char soundSize : 1;
    /*
    aac is always 1.
    0 = Mono sound
    1 = Stereo sound
    */
    enum SoundType {
        Mono   = 0,
        Stereo = 1
    };
    unsigned char soundType : 1;
    /*
    0 = AAC sequence header
    1 = AAC raw
    */

    enum AACPacketType {
        AACSequenceHeader = 0,
        AACRaw            = 1
    };
    unsigned char aacPacketType;

    flv_aac_audio_tag_header( AACPacketType aacPacketType ) {
        soundFormat         = SoundFormat::AAC;  // aac
        soundRate           = SoundRate::Rate44; // 44kHZ
        soundSize           = SoundSize::Bits16; // 16bit samples
        soundType           = SoundType::Stereo; // Stereo sound
        this->aacPacketType = aacPacketType;
    }

    void to_buf( uint8_t buf[2] ) {
        PutBitsContext context = PutBitsContext( buf, 2 );
        context.put_bits( 4, soundFormat );
        context.put_bits( 2, soundRate );
        context.put_bits( 1, soundSize );
        context.put_bits( 1, soundType );
        context.put_bits( 8, aacPacketType );
    }
};

/**
 * @brief flv_avc_tag_header
 *  When write to file, flv_avc_tag_header is 5 bytes.
 */
struct flv_avc_tag_header {
    /*
    1 = key frame (for AVC, a seekable frame)
    2 = inter frame (for AVC, a non-seekable frame)
    3 = disposable inter frame (H.263 only)
    4 = generated key frame (reserved for server use only)
    5 = video info/command frame
    */

    enum FrameType {
        AVCKeyFrame   = 1,
        AVCInterFrame = 2,
    };

    unsigned char frameType : 4;
    /*
    2 = Sorenson H.263
    3 = Screen video
    4 = On2 VP6
    5 = On2 VP6 with alpha channel 6 = Screen video version 2
    7 = AVC
    */
    enum CodecID {
        AVC = 7
    };

    unsigned char codecID : 4;
    /*
    0 = AVC sequence header
    1 = AVC NALU
    2 = AVC end of sequence (lower level NALU sequence ender is not required or supported)
    */
    enum AVCPacketType {
        AVCSequenceHeader = 0,
        AVCNALU           = 1,
        AVCEndOfSequence  = 2
    };
    unsigned char avcPacketType;
    /*
    IF AVCPacketType == 1
        Composition time offset
    ELSE
        0

    compositionTime = pts - dts
    timestamp in flv_tag_header is dts.
    When there is no b frame, pts = dts, compositionTime = 0.
    */
    signed int compositionTime : 24;

    flv_avc_tag_header( FrameType frameType, AVCPacketType avcPacketType, int compositionTime ) {
        this->frameType       = frameType;
        this->codecID         = CodecID::AVC; // AVC
        this->avcPacketType   = avcPacketType;
        this->compositionTime = compositionTime;
    }

    void to_buf( uint8_t buf[5] ) {
        PutBitsContext context = PutBitsContext( buf, 5 );
        context.put_bits( 4, frameType );
        context.put_bits( 4, codecID );
        context.put_bits( 8, avcPacketType );
        context.put_bits( 24, compositionTime );
    }
};

};     // namespace nx

#endif // __FLV_TAG_H__
