#ifndef __FLVDEFS_H__
#define __FLVDEFS_H__

#include <string.h>
#include <stdint.h>


namespace nx
{

struct FlvHeader
{
    unsigned char signatureF = 'F';
    unsigned char signatureL = 'L';
    unsigned char signatureV = 'V';
    unsigned char version = 1; //0x01 for flv version 1
    //typeFlagsReserved, Shall be 0
    unsigned char typeFlagsReserved1: 5; 
    //Audio tags are present
    unsigned char typeFlagsAudio : 1;
    //typeFlagsReserved, Shall be 0
    unsigned char typeFlagsReserved2 : 1; 
    //Video tags are present
    unsigned char typeFlagsVideo : 1; 
    //The length of this header in bytes, usually has a value of 9 for FLV version 1.
    unsigned int dataOffset = 9;
    
    FlvHeader(bool hasAudio, bool hasVideo) {
        typeFlagsReserved1 = 0;
        typeFlagsReserved2 = 0;
        typeFlagsAudio = hasAudio ? 1 : 0;
        typeFlagsVideo = hasVideo ? 1 : 0;
    }
};

/*
Format of SoundData. The following values are defined: 0 = Linear PCM, platform endian
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
Formats 7, 8, 14, and 15 are reserved.
AAC is supported in Flash Player 9,0,115,0 and higher. Speex is supported in Flash Player 10 and higher.
*/
enum SoundFormat {
    LinearPCM_PlatformEndian = 0,
    ADPCM,
    MP3,
    LinearPCM_LittleEndian,
    Nellymoser16kHzMono,
    Nellymoser8kHzMono,
    Nellymoser,
    G711ALawPCM = 7,
    G711MuLawPCM = 8,
    Reserved = 9,
    AAC = 10,
    Speex = 11,
    MP38kHz = 14,
    DeviceSpecificSound = 15
};   

/*
Sampling rate. The following values are defined: 
0 = 5.5 kHz
1 = 11 kHz
2 = 22 kHz
3 = 44 kHz
*/
enum SoundRateType {
    Rate5_5kHz,
    Rate11kHz,
    Rate22kHz,
    Rate44kHz
};

/*
Size of each audio sample. This parameter only pertains to uncompressed formats. Compressed formats always decode to 16 bits internally.
0 = 8-bit samples
1 = 16-bit samples
*/
enum SoundSizeType {
    OneByte,
    TwoByte
};
/*
Mono or stereo sound 0 = Mono sound
1 = Stereo sound
*/
enum SoundType {
    Mono,
    Stereo
};

/*
The following values are defined: 0 = AAC sequence header
1 = AAC raw
*/
enum AACPacketType {
    AACSequenceHeader,
    AACRaw
};

/*
Type of video frame. The following values are defined: 
1 = key frame (for AVC, a seekable frame)
2 = inter frame (for AVC, a non-seekable frame)
3 = disposable inter frame (H.263 only)
4 = generated key frame (reserved for server use only) 
5 = video info/command frame
*/
enum VideoFrameType {
    KeyFrame = 1,
    InterFrame = 2,
    DisposableInterFrame = 3,
    GeneratedKeyFrame = 4,
    CommandFrame = 5
};

/*
Codec Identifier. The following values are defined: 2 = Sorenson H.263
3 = Screen video
4 = On2 VP6
5 = On2 VP6 with alpha channel 6 = Screen video version 2
7 = AVC
*/
enum VideoCodecID {
    SorensonH263 = 2,
    ScreenVideo = 3,
    On2VP6 = 4,
    On2VP6WithAlphaChannel = 5,
    ScreenVideo2 = 6,
    AVC = 7
};

/*
The following values are defined:
0 = AVC sequence header
1 = AVC NALU
2 = AVC end of sequence (lower level NALU sequence ender is not required or supported)
*/
enum AVCPacketType  {
    AVCSequenceHeader = 0,
    AVCNALU = 1,
    AVCEndOfSequence = 2
};

struct MetaData
{
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
    int audiocodecid = 10; //aac
    //Audio bit rate in kilobits per second
	double audiodatarate; // kbps
	//Frequency at which the audio stream is replayed
    int audiosamplerate;
    //Resolution of a single audio sample
	int audiosamplesize;
    //Indicating stereo audio
	bool stereo;

    /*
    2 = Sorenson H.263
    3 = Screen video
    4 = On2 VP6
    5 = On2 VP6 with alpha channel 6 = Screen video version 2
    7 = AVC
    */
	int videocodecid = 7; 
    //Video bit rate in kilobits per second
	double videodatarate; 
    //Number of frames per second
	double framerate; // fps
    //Width of the video in pixels
	int width;
    //Height of the video in pixels
	int height;

    //Total duration of the file in seconds
	double duration;
};


enum FlvTagType {
    Audio = 8,
    Video = 9,
    ScriptData = 18,
};

struct FlvTagHeader
{   
    //Reserved for FMS, should be 0
    unsigned char reserved: 2; 
        /*
    0 = No pre-processing required.
    1 = Pre-processing (such as decryption) of the packet is required before it can be rendered
    */
    unsigned char filter : 1; 
    /*
    tag type: 
    - 8 audio 
    - 9 video
    - 18 script data
    */
    unsigned char tagType : 5;
    //Length of the message. Number of bytes after StreamID to end of tag (Equal to length of the tag – 11)
    unsigned char dataSize[3]; 
    //Time in milliseconds at which the data in this tag applies. This value is relative to the first tag in the FLV file, which always has a timestamp of 0.
    unsigned char timestamp[3]; 
    //Extension of the Timestamp field to form a SI32 value. This field represents the upper 8 bits, while the previous Timestamp field represents the lower 24 bits of the time in milliseconds.
    unsigned char timestampExtended;
    //Always 0.
    unsigned char streamID[3];

    FlvTagHeader(FlvTagType tagType, size_t length, uint32_t timestamp) {
        reserved = 0;
        filter = 0;
        this->tagType = tagType & 0x1F;
        //big endian
        dataSize[0] = (length >> 16) & 0xFF;
        dataSize[1] = (length >> 8) & 0xFF;
        dataSize[2] = length & 0xFF;
        //high 8 bits
        timestampExtended = (length >> 24) & 0xFF;
        //low 24 bits
        this->timestamp[0] = (timestamp >> 16) & 0xFF;
        this->timestamp[1] = (timestamp >> 8) & 0xFF;
        this->timestamp[2] = timestamp & 0xFF;
        //always 0
        memset(streamID, 0, sizeof(streamID));
    }
};

/*
If the SoundFormat indicates AAC, the SoundType should be 1 (stereo) and the SoundRate should be 3 (44 kHz). 
However, this does not mean that AAC audio in FLV is always stereo, 44 kHz data. Instead, the Flash Player ignores these values and extracts the channel and sample rate data is encoded in the AAC bit stream.
*/

struct AudioTagHeader
{
    unsigned char soundFormat : 4;
    unsigned char soundRate : 2;
    unsigned char soundSize : 1;
    unsigned char soundType : 1;
    unsigned char aacPacketType;

    static AudioTagHeader aacTagHeader(bool isSequenceHeader) {
        AudioTagHeader header;
        header.soundFormat = SoundFormat::AAC & 0x0F;
        header.soundRate = SoundRateType::Rate44kHz & 0x03;
        header.soundSize = SoundSizeType::TwoByte & 0x01;
        header.soundType = SoundType::Stereo & 0x01;
        if (isSequenceHeader) {
            header.aacPacketType = AACPacketType::AACSequenceHeader;
        } else {
            header.aacPacketType = AACPacketType::AACRaw;
        }
        return header;
    };

};

struct VideoTagHeader
{
    unsigned char frameType : 4;
    unsigned char codedID : 4;
};

struct AVCVideoTagHeader: public VideoTagHeader
{
    unsigned char avcPacketType;
    unsigned char compositionTime[3];
};


//ref to: https://blog.csdn.net/y_z_hyangmo/article/details/79208275
struct AudioSpecificConfig
{
    unsigned char profile : 5;
    unsigned char sampleRateIndex : 4;
    unsigned char channels : 4;
    //CASpecificConfig
    unsigned char frameLengthFlag : 1;
    unsigned char dependsOnCoreCoder : 1;
    unsigned char extentionFlag : 1;
};


struct AVCDecoderConfigurationRecord
{
    /* data */
};













} // namespace nx










#endif // __FLVDEFS_H__