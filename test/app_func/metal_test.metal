#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct metal_vtx {
   vector_float2 pos;
   vector_float4 color;
};

struct metal_raster_data {
   float4 clip_pos [[position]];
   float4 color;
};

vertex metal_raster_data vs(uint vtx_id [[vertex_id]], constant struct metal_vtx * vertices [[buffer(0)]], constant vector_uint2 * viewport_size_ptr [[buffer(1)]], constant vector_float4 * color_offset_ptr [[buffer(2)]]) {
   struct metal_raster_data out;

   float2 pixel_pos = vertices[vtx_id].pos.xy;
   vector_float2 viewport_size = vector_float2(*viewport_size_ptr);
   out.clip_pos.xy = pixel_pos / (viewport_size / 2.0) - vector_float2(1.0, 1.0);
   out.clip_pos.z = 0.0;
   out.clip_pos.w = 1.0;

   vector_float4 color_offset = vector_float4(*color_offset_ptr);
   out.color = vertices[vtx_id].color;
   out.color.r = fract(out.color.r + color_offset.x);

   return out;
}

fragment float4 fs(struct metal_raster_data in [[stage_in]]) {
   return in.color;
}
