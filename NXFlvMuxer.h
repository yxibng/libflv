//
//  NXFlvMuxer.h
//  NXLive
//
//  Created by yxibng on 2023/4/26.
//  Copyright © 2023 Neuxnet. All rights reserved.
//

#import "NXFrame.h"
#import "NXLiveOptions.h"
#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM( NSInteger, NXFlvMuxerErrorCode ) {
    NXFlvMuxerErrorFailedToCreateFile,
    NXFlvMuxerErrorWriteDataError
};

@class NXFlvMuxer;
@protocol NXFlvMuxerDelegate < NSObject >
@optional
- (void)flvMuxer:(NXFlvMuxer *)flvMuxer didOccurError:(NXFlvMuxerErrorCode)errorCode;
@end

@interface NXFlvMuxer : NSObject

@property ( nonatomic, weak ) id< NXFlvMuxerDelegate > delegate;
- (void)startWithPath:(NSString *)path captureType:(NXLiveCaptureTypeMask)captureType;
- (void)muxFrame:(NXFrame *)frame;
- (void)suspendMuxingWithCompletion:(nullable void ( ^)( NSString *_Nullable path ))callback;
- (void)stopMuxingWithCompletion:(nullable void ( ^)( NSString *_Nullable path ))callback;
@end

@interface NXFlvMuxer ( NX_Err )
- (void)handleWriteError:(int)errorNo;
- (void)handleCreateFileError;
@end

@interface NXFlvMuxer ( NX )
- (void)onMuxingDataSize:(size_t)size dataType:(int)dataType;
@end

NS_ASSUME_NONNULL_END
