#ifndef GXUTILS_H
#define GXUTILS_H

#include <gccore.h>

extern GXColor gxu_background_color;

extern u8 gxu_clear_buffers;

extern u8 gxu_clear_color_buffer;

extern Mtx44 gxu_projection_matrices[];

extern int gxu_cur_projection_matrix;

extern Mtx gxu_modelview_matrices[];

extern int gxu_cur_modelview_matrix;

extern bool gxu_cull_enabled;

extern u8 gxu_cull_mode;

extern u8 gxu_z_test_enabled;

extern u8 gxu_cur_z_func;

extern u8 gxu_z_write_enabled;

extern u8 gxu_blend_src_value;

extern u8 gxu_blend_dst_value;

extern bool gxu_alpha_test_enabled;

extern u8 gxu_alpha_test_lower;

extern u8 gxu_alpha_test_higher;

extern u8 gxu_cur_vertex_format;

extern u8 gxu_cur_r;

extern u8 gxu_cur_g;

extern u8 gxu_cur_b;

extern u8 gxu_cur_a;

extern f32 gxu_viewport_x;

extern f32 gxu_viewport_y;

extern f32 gxu_viewport_width;

extern f32 gxu_viewport_height;

void GXU_Init(GXRModeObj* rmode, void* framebuffer);

void GXU_EndFrame(void* framebuffer);

void GXU_EnableTexture(void);

void GXU_DisableTexture(void);

void GXU_CallguMtxRotAxisDeg(Mtx mt, guVector* axis, f32 deg);

void GXU_CallGXPosition3f32(f32 x, f32 y, f32 z);

void GXU_CallGXColor4u8(u8 r,u8 g,u8 b,u8 a);

void GXU_CallGXTexCoord2f32(f32 s, f32 t);

void GXU_CallGXEnd(void);

unsigned char* GX_CopyTexRGBA8(unsigned char* src, int width, int height, unsigned char* dst);

unsigned char* GX_CopyTexRGB5A3(unsigned char* src, int width, int height, unsigned char* dst);

unsigned char* GX_CopyTexV8(unsigned char* src, int width, int height, unsigned char* dst);

unsigned char* GX_CopyTexIA4(unsigned char* src, int width, int height, unsigned char* dst);

#endif