//
//  NXFlvMuxer.m
//  NXLive
//
//  Created by yxibng on 2023/4/26.
//  Copyright © 2023 Neuxnet. All rights reserved.
//

#import "NXFlvMuxer.h"
#import "NXLiveAudioFrame.h"
#import "NXLiveVideoFrame.h"

#import "aac.h"
#import "flvmuxer.h"

namespace nx {

//8 - audio, 9 - video, 18 - script data
enum FlvDataType {
    Header = 0,
    Audio = 8,
    Video = 9,
    MetaData = 18,
};

struct FlvFileWriter : public FlvMuxerDataHandler {
  private:
    FILE *flvFile = nullptr;

  public:
    ~FlvFileWriter() {
        onEndMuxing();
    }
    FlvFileWriter( const char *filePath, void *context ) {
        flvFile = fopen( filePath, "wb" );
        if ( !flvFile ) {
            // handle create file error
            NXFlvMuxer *muxer = (__bridge NXFlvMuxer *)context;
            [muxer handleCreateFileError];
            return;
        }
        this->context = context;
    }

    virtual void onMuxedFlvHeader( void *context, uint8_t *data, size_t bytes ) override {
        if ( !flvFile ) return;
        size_t bytes_write = fwrite( data, 1, bytes, flvFile );
        if ( bytes != bytes_write ) {
            // handle write error
            int err = ferror( flvFile );
            NXFlvMuxer *muxer = (__bridge NXFlvMuxer *)context;
            [muxer handleWriteError:err];
            onEndMuxing();
            return;
        }

        {
            NXFlvMuxer *muxer = (__bridge NXFlvMuxer *)context;
            [muxer onMuxingDataSize:bytes dataType:FlvDataType::Header];
        }
    }
    virtual void onMuxedData( void *context, int type, const uint8_t *data, size_t bytes, uint32_t timestamp ) override {
        if ( !flvFile ) return;
        size_t bytes_write = fwrite( data, 1, bytes, flvFile );
        if ( bytes != bytes_write ) {
            // handle write error
            int err = ferror( flvFile );
            NXFlvMuxer *muxer = (__bridge NXFlvMuxer *)context;
            [muxer handleWriteError:err];
            onEndMuxing();
            return;
        }

        {
            NXFlvMuxer *muxer = (__bridge NXFlvMuxer *)context;
            [muxer onMuxingDataSize:bytes dataType:type];
        }
    }

    virtual void onUpdateMuxedData( void *context, size_t offsetFromStart, const uint8_t *data, size_t bytes ) override {
        if ( !flvFile ) return;
        int ret = fseek( flvFile, offsetFromStart, SEEK_SET );
        if ( ret != 0 ) {
            // handle seek failed
            int err = ferror( flvFile );
            NXFlvMuxer *muxer = (__bridge NXFlvMuxer *)context;
            [muxer handleWriteError:err];
            onEndMuxing();
            return;
        }
        size_t bytes_write = fwrite( data, 1, bytes, flvFile );
        if ( bytes != bytes_write ) {
            // handle write error
            int err = ferror( flvFile );
            NXFlvMuxer *muxer = (__bridge NXFlvMuxer *)context;
            [muxer handleWriteError:err];
            onEndMuxing();
            return;
        }
        // back to file end
        fseek( flvFile, 0, SEEK_END );
    }
    virtual void onEndMuxing() override {
        if ( !flvFile ) return;
        fclose( flvFile );
        flvFile = nullptr;
    }
};
}; // namespace nx

@interface NXFlvMuxer () {
  @public
    std::shared_ptr< nx::FlvMuxer > _muxer;
    std::shared_ptr< nx::FlvFileWriter > _fileWriter;
    BOOL _running;
    NSString *_filePath;
    BOOL _hasContentInFile;
}
@end

@implementation NXFlvMuxer

- (void)dealloc {
    [self endMuxing];
}

- (void)startWithPath:(NSString *)path captureType:(NXLiveCaptureTypeMask)captureType {
    if ( _running ) return;
    NSAssert( path != nil, @"NXFlvMuxer file path can not be nil." );
    _filePath = [path copy];
    int hasAudio = ( captureType & NXLiveInputMaskAudio ) || ( captureType & NXLiveCaptureMaskAudio ) || ( captureType & NXLiveScreenMaskAudio );
    int hasVideo = ( captureType & NXLiveInputMaskVideo ) || ( captureType & NXLiveCaptureMaskVideo ) || ( captureType & NXLiveScreenMaskVideo );
    _fileWriter = std::make_shared< nx::FlvFileWriter >( [path cStringUsingEncoding:NSUTF8StringEncoding], (__bridge void *)self );
    _muxer = std::make_shared< nx::FlvMuxer >( hasAudio, hasVideo, _fileWriter );
    _running = YES;
}

- (void)muxFrame:(NXFrame *)frame {
    if ( !_running ) return;
    if ( !frame ) return;
    if ( [frame isKindOfClass:NXLiveAudioFrame.class] ) {
        // audio
        uint8_t adtsHeader[7] = { 0 };
        auto header = nx::adts_header( nx::adts_header::Profile::LC, 48000, 1, (int)frame.data.length );
        header.to_buf( adtsHeader );
        NSMutableData *data = [NSMutableData dataWithBytes:adtsHeader length:7];
        [data appendData:frame.data];
        uint32_t pts = (uint32_t)frame.timestamp;
        _muxer->mux_aac( (uint8_t *)data.bytes, data.length, pts );
    }

    if ( [frame isKindOfClass:NXLiveVideoFrame.class] ) {
        // video
        uint8_t annexbHeader[] = { 0x00, 0x00, 0x00, 0x01 };
        NSMutableData *data = [NSMutableData data];
        NXLiveVideoFrame *videoFrame = (NXLiveVideoFrame *)frame;
        if ( videoFrame.isKeyFrame && videoFrame.sps && videoFrame.pps ) {
            [data appendBytes:&annexbHeader length:sizeof( annexbHeader )];
            [data appendData:videoFrame.sps];
            [data appendBytes:&annexbHeader length:sizeof( annexbHeader )];
            [data appendData:videoFrame.pps];
        }
        [data appendBytes:&annexbHeader length:sizeof( annexbHeader )];
        [data appendData:videoFrame.data];
        uint32_t pts = (uint32_t)frame.timestamp;
        _muxer->mux_avc( (uint8_t *)data.bytes, data.length, pts, pts, videoFrame.isKeyFrame );
    }
}

- (void)endMuxing {
    if (!_running) return;
    _fileWriter.reset();
    _muxer.reset();
    _running = NO;
    if ( !_hasContentInFile ) {
        [[NSFileManager defaultManager] removeItemAtPath:_filePath error:nil];
        _filePath = nil;
    }
}

- (void)suspendMuxingWithCompletion:(void ( ^)( NSString *_Nonnull ))callback {
    NSString *filePath = _hasContentInFile ? _filePath : nil;
    if ( callback ) callback( filePath );
}

- (void)stopMuxingWithCompletion:(void ( ^)( NSString *_Nonnull ))callback {
    [self endMuxing];
    NSString *filePath = _hasContentInFile ? _filePath : nil;
    if ( callback ) callback( filePath );
    _hasContentInFile = NO;
}

@end

@implementation NXFlvMuxer ( NX_Err )

- (void)handleWriteError:(int)errorNo {
    if ( !_running ) return;
    LOGE( "NXFlvMuxer:  failed to write data to flv file, ferror is %d", errorNo );
    if ( [self.delegate respondsToSelector:@selector( flvMuxer:didOccurError: )] ) {
        [self.delegate flvMuxer:self didOccurError:NXFlvMuxerErrorWriteDataError];
    }
    _running = NO;
}

- (void)handleCreateFileError {
    LOGE( "NXFlvMuxer:  failed to create flv file at path %@", _filePath );
    if ( [self.delegate respondsToSelector:@selector( flvMuxer:didOccurError: )] ) {
        [self.delegate flvMuxer:self didOccurError:NXFlvMuxerErrorFailedToCreateFile];
    }
}

@end

@implementation NXFlvMuxer ( NX )

- (void)onMuxingDataSize:(size_t)size dataType:(int)dataType {
    if (dataType == nx::FlvDataType::Audio || dataType == nx::FlvDataType::Video) {
        _hasContentInFile = YES;
    }
}

@end
