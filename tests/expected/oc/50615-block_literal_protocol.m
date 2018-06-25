#import <Foundation/Foundation.h>

@interface TestClass : NSObject
@end

@implementation TestClass

- (void)drawSomething:(id<MTLCommandBuffer>)commandBuffer {
    [renderPass performDrawBlock:^(id<MTLRenderCommandEncoder> renderCommandEncoder) {
        screenBlitObject.texture = src;

        // Make sure the pipeline state pixelformat is the same as destination pixel format
        [screenBlitObject updatePipelineState:dst.pixelFormat];

        [screenBlitObject drawWithRenderCommandEncoder:renderCommandEncoder];
    } withTargetTexture:dst andCommandBuffer:commandBuffer];
}

@end
