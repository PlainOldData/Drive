#ifdef __APPLE__

#include <drive/app.h>
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <simd/simd.h>


struct metal_vtx {
    vector_float2 pos;
    vector_float4 color;
};


id<MTLDevice> metal_device;
vector_float4 color_offset = { 0 };
CAMetalLayer * metal_layer = 0;
id<MTLCommandQueue> cmd_queue;
id<MTLRenderPipelineState> pipeline;
NSUInteger frame_count = 0;
id<MTLLibrary> shader_lib;
id<MTLFunction> vs;
id<MTLFunction> fs;


struct metal_vtx vtx_array[] = {
        { .pos = { 100.0f,-100.0f, }, .color = { 0.0f, 0.5f, 1.0f, 1.0f, }, },
        { .pos = {-100.0f,-100.0f, }, .color = { 0.0f, 1.0f, 0.5f, 1.0f, }, },
        { .pos = { 100.0f, 100.0f, }, .color = { 1.0f, 0.0f, 0.5f, 1.0f, }, },

        { .pos = { 100.0f, 100.0f, }, .color = { 1.0f, 0.0f, 0.5f, 1.0f, }, },
        { .pos = {-100.0f, 100.0f, }, .color = { 1.0f, 0.5f, 0.5f, 1.0f, }, },
        { .pos = {-100.0f,-100.0f, }, .color = { 0.0f, 1.0f, 0.5f, 1.0f, }, },
};


void* test_device() {
        static int once = 1;
        if(once) {
                metal_device = MTLCreateSystemDefaultDevice();
                once = 0;
        }
        return metal_device;
}


void test_setup(struct drv_app_ctx *ctx)
{
        struct drv_app_data data = {0};
        drv_app_result app_ok = drv_app_data_get(ctx, &data);
        assert(app_ok == DRV_APP_RESULT_OK);
        
        metal_layer= (CAMetalLayer*)data.layer;
        
        cmd_queue = [metal_device newCommandQueue];
    
        NSError * metal_err = nil;

        const char * shader_lib_src = "\n"
        "#include <metal_stdlib>\n"
        "#include <simd/simd.h>\n"
        "\n"
        "using namespace metal;\n"
        "\n"
        "struct metal_vtx {\n"
        "   vector_float2 pos;\n"
        "   vector_float4 color;\n"
        "};\n"
        "\n"
        "struct metal_raster_data {\n"
        "   float4 clip_pos [[position]];\n"
        "   float4 color;\n"
        "};\n"
        "\n"
        "vertex metal_raster_data vs(uint vtx_id [[vertex_id]], constant struct metal_vtx * vertices [[buffer(0)]], constant vector_uint2 * viewport_size_ptr [[buffer(1)]], constant vector_float4 * color_offset_ptr [[buffer(2)]]) {\n"
        "   struct metal_raster_data out;\n"
        "\n"
        "   float2 pixel_pos = vertices[vtx_id].pos.xy;\n"
        "   vector_float2 viewport_size = vector_float2(*viewport_size_ptr);\n"
        "   out.clip_pos.xy = pixel_pos / (viewport_size / 2.0) - vector_float2(1.0, 1.0);\n"
        "   out.clip_pos.z = 0.0;\n"
        "   out.clip_pos.w = 1.0;\n"
        "\n"
        "   vector_float4 color_offset = vector_float4(*color_offset_ptr);"
        "   out.color = vertices[vtx_id].color;\n"
        "   out.color.r = fract(out.color.r + color_offset.x);\n"
        "\n"
        "   return out;\n"
        "}\n"
        "\n"
        "fragment float4 fs(struct metal_raster_data in [[stage_in]]) {\n"
        //"   return in.color;\n"
        " float4 c; c.x = 0; c.y = 1; c.z = 0; c.w = 1;"
        " return c;"
        "}\n"
        "";
        
        NSString * shader_lib_str = [NSString stringWithUTF8String:shader_lib_src];

        shader_lib = [metal_device newLibraryWithSource:shader_lib_str options:nil error:&metal_err];
        
        if(shader_lib == nil) {
                NSLog(@"%@", metal_err);
        }

        vs = [shader_lib newFunctionWithName:@"vs"];
        fs = [shader_lib newFunctionWithName:@"fs"];

        MTLRenderPipelineDescriptor * pipeline_desc = [[MTLRenderPipelineDescriptor alloc] init];
        pipeline_desc.vertexFunction = vs;
        pipeline_desc.fragmentFunction = fs;
        pipeline_desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

        pipeline = [metal_device newRenderPipelineStateWithDescriptor:pipeline_desc error:&metal_err];
        if(pipeline == nil) {
                NSLog(@"%@", metal_err);
        }
}


void test_tick()
{
        vector_uint2 view_size = { 640, 360 };
        
//        uint32_t view_width = (uint32_t)metal_view.bounds.size.width;
//        uint32_t view_height = (uint32_t)metal_view.bounds.size.height;
        
//        if(view_size[0] != view_width || view_size[1] != view_height) {
//                view_size[0] = view_width;
//                view_size[1] = view_height;
                metal_layer.drawableSize = (CGSize) { (CGFloat)view_size.x, (CGFloat)view_size.y, };
//
//                NSLog(@"%u, %u\n", view_size.x, view_size.y);
//            }

        id<CAMetalDrawable> drawable = [metal_layer nextDrawable];

        if(drawable == nil) {
        NSLog(@"drawable == nil");
        }

        id<MTLTexture> texture = drawable.texture;

        id<MTLCommandBuffer> cmd_buf = [cmd_queue commandBuffer];

        MTLRenderPassDescriptor * render_desc = [MTLRenderPassDescriptor renderPassDescriptor];
        render_desc.colorAttachments[0].texture     = texture;
        render_desc.colorAttachments[0].loadAction  = MTLLoadActionClear;
        render_desc.colorAttachments[0].storeAction = MTLStoreActionStore;
        render_desc.colorAttachments[0].clearColor  = MTLClearColorMake(0.2, 0.2, 0.22, 0.0);

        id<MTLRenderCommandEncoder> render_encoder = [cmd_buf renderCommandEncoderWithDescriptor:render_desc];
        [render_encoder setViewport:(MTLViewport) { 0.0, 0.0, (double)view_size[0], (double)view_size[1], 0.1, 1000}];
        [render_encoder setRenderPipelineState:pipeline];
        [render_encoder setVertexBytes:vtx_array length:sizeof(vtx_array) atIndex:0];
        [render_encoder setVertexBytes:&view_size length:sizeof(view_size) atIndex:1];
        [render_encoder setVertexBytes:&color_offset length:sizeof(color_offset) atIndex:2];
        [render_encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount: 6];
        [render_encoder endEncoding];
        
//        [render_encoder release];
//        [render_desc release];

        [cmd_buf presentDrawable:drawable];
        [cmd_buf commit];

        [cmd_buf waitUntilScheduled];
//         [cmd_buf waitUntilCompleted];

//        [cmd_buf release];

        color_offset.x += 0.01;
        if(color_offset.x > 1.0) {
            color_offset.x -= 1.0;
        }
}


void test_shutdown()
{

}

#endif
