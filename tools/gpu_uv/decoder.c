/* Call the shipping command decoder and PGXP lookup, observing the RHI consumer. */
#include <stdio.h>
#include <string.h>
/* The command entry points are static;
   include the actual decoder. LTO
 * discards unrelated execution paths, as in the serializer test. */
#include "../../mednafen/psx/gpu.c"
#include "pgxp/pgxp_value.h"
extern bool FastSaveStates;

static enum rhi_renderer_type observed_renderer;
static unsigned observed_quad_count, observed_triangle_count;
static bool observed_possible_2d;
static uint32_t observed_uv_offset;
static float observed_w[4], observed_xy[8];
static uint16_t observed_uv[8], observed_limits[4];
static uint16_t observed_vram[1024 * 512];
static unsigned cases, assertions, failures;
static bool fractional_xy;

static void check(const char *name, bool passed)
{
   assertions++;
   if (!passed)
   {
      fprintf(stderr, "%s: case=%u\n", name, cases);
      failures++;
   }
}

int psx_pgxp_color, psx_pgxp_fog;
int psx_pgxp_2d_tol = -1;
uint8_t psx_gpu_upscale_shift_hw = 2;
int line_render_mode;

enum rhi_renderer_type rhi_intf_is_type(void)
{
   return observed_renderer;
}

/* Observe the payload received by the renderer, rather than only its cache. */
void rhi_intf_push_quad(float p0x, float p0y, float p0w,
                        float p1x, float p1y, float p1w,
                        float p2x, float p2y, float p2w,
                        float p3x, float p3y, float p3w,
                        uint32_t c0,
                        uint32_t c1,
                        uint32_t c2,
                        uint32_t c3,
                        /* As push_triangle: 4x3 floats or NULL. */
                        const float *precise_rgb,
                        uint16_t t0x, uint16_t t0y,
                        uint16_t t1x, uint16_t t1y,
                        uint16_t t2x, uint16_t t2y,
                        uint16_t t3x, uint16_t t3y,
                        uint16_t min_u, uint16_t min_v,
                        uint16_t max_u, uint16_t max_v,
                        uint16_t texpage_x, uint16_t texpage_y,
                        uint16_t clut_x, uint16_t clut_y,
                        uint8_t texture_blend_mode,
                        uint8_t depth_shift,
                        bool dither,
                        int blend_mode,
                        bool mask_test,
                        bool set_mask,
                        bool is_sprite,
                        bool may_be_2d,
                        uint32_t uv_offset)
{
   observed_quad_count++;
   observed_possible_2d = may_be_2d;
   observed_uv_offset = uv_offset;
   observed_w[0] = p0w;
   observed_w[1] = p1w;
   observed_w[2] = p2w;
   observed_w[3] = p3w;
   observed_xy[0] = p0x;
   observed_xy[1] = p0y;
   observed_xy[2] = p1x;
   observed_xy[3] = p1y;
   observed_xy[4] = p2x;
   observed_xy[5] = p2y;
   observed_xy[6] = p3x;
   observed_xy[7] = p3y;
   observed_uv[0] = t0x;
   observed_uv[1] = t0y;
   observed_uv[2] = t1x;
   observed_uv[3] = t1y;
   observed_uv[4] = t2x;
   observed_uv[5] = t2y;
   observed_uv[6] = t3x;
   observed_uv[7] = t3y;
   observed_limits[0] = min_u;
   observed_limits[1] = min_v;
   observed_limits[2] = max_u;
   observed_limits[3] = max_v;
}

void rhi_intf_push_triangle(float p0x, float p0y, float p0w,
                            float p1x, float p1y, float p1w,
                            float p2x, float p2y, float p2w,
                            uint32_t c0, uint32_t c1, uint32_t c2,
                            /* Optional precise vertex colours: 3 floats per
                             * vertex in 0..~2 scale (1.0 = 0xFF), or NULL to
                             * derive from the packed bytes above. Carries the
                             * GTE's pre-saturation colour, which can exceed
                             * white;
   only meaningful on an fp16 target. */
                            const float *precise_rgb,
                            uint16_t t0x, uint16_t t0y,
                            uint16_t t1x, uint16_t t1y,
                            uint16_t t2x, uint16_t t2y,
                            uint16_t min_u, uint16_t min_v,
                            uint16_t max_u, uint16_t max_v,
                            uint16_t texpage_x, uint16_t texpage_y,
                            uint16_t clut_x, uint16_t clut_y,
                            uint8_t texture_blend_mode,
                            uint8_t depth_shift,
                            bool dither,
                            /* This is really an `enum blending_modes`
                             * but I don't want to deal with enums in the
                             * FFI */
                            int blend_mode,
                            bool mask_test,
                            bool set_mask)
{
   observed_triangle_count++;
}

bool rhi_intf_has_software_renderer(void)
{
   return false;
}

#include "test_stubs.h"

static uint32_t xy(unsigned x, unsigned y)
{
   return x | y << 16;
}

static void shadow(unsigned index, unsigned x, unsigned y, unsigned valid, float w)
{
   PGXP_value value;
   memset(&value, 0, sizeof(value));
   value.x = x + (fractional_xy ? 0.5f : 0.0f);
   value.y = y + (fractional_xy ? 0.5f : 0.0f);
   value.z = w;
   value.value = xy(x, y);
   value.flags = valid;
   PGXP_WriteCB(&value, index);
}

int main(void)
{
   unsigned renderer, kind, restore;
   static const char *names[] = {
      "uniform-valid", "fourth-depth-missing", "fourth-xy-missing",
      "real-varying-depth", "all-metadata-missing", "valid-unit-depth",
      "fourth-depth-missing-xy-snap"
   };
   for (renderer = 0; renderer < 2; renderer++)
      for (kind = 0; kind < 7; kind++)
         for (restore = 0; restore < 3; restore++)
         {
            uint32_t first[7] = {0x2d808080, 0, 31 | 31 << 8, 0, 31 << 8 | 0x11c << 16, 0, 31};
            uint32_t second[2] = {0, 0};
            unsigned i;
            bool expected_2d = kind != 3;
            float expected_w = (kind == 0 || kind == 3) ? 8.0f : 1.0f;
            cases++;
            fractional_xy = kind == 6;
            psx_pgxp_2d_tol = fractional_xy ? 0 : -1;
            memset(&GPU, 0, sizeof(GPU));
            {
               PGXP_value empty;
               unsigned index;
               memset(&empty, 0, sizeof(empty));
               for (index = 0; index < 16; index++)
                  PGXP_WriteCB(&empty, index);
            }
            GPU.vram = observed_vram;
            GPU.SUCV.TWX_AND = GPU.SUCV.TWY_AND = 0xff;
            GPU.ClipX1 = 1023;
            GPU.ClipY1 = 511;
            GPU.DrawTimeAvail = 100000;
            observed_renderer = renderer ? RHI_VULKAN : RHI_OPENGL;
            observed_quad_count = observed_triangle_count = 0;
            first[1] = xy(16, 16);
            first[3] = xy(48, 16);
            first[5] = xy(16, 48);
            shadow(1, 16, 16, kind == 4 ? 0 : VALID_012, kind == 5 ? 1.0f : 8.0f);
            shadow(3, 48, 16, kind == 4 ? 0 : VALID_012, kind == 5 ? 1.0f : 8.0f);
            shadow(5, 16, 48, kind == 4 ? 0 : VALID_012, kind == 5 ? 1.0f : 8.0f);
            Command_DrawPolygon_NV4_G0_T1_BMopaque_TM0_MO2_ME0_PG1(&GPU, first);
            if (GPU.InCmd != INCMD_QUAD || observed_quad_count ||
                GPU.InQuad_F3Vertices[0].precise[2] != ((kind == 4 || kind == 5) ? 1.0f : 8.0f))
               return 2;
            if (restore)
            {
               StateMem saved = {0};
               FastSaveStates = restore == 2;
               if (!GPU_StateAction(&saved, 0, 0))
                  return 4;
               GPU.InCmd = INCMD_NONE;
               GPU.InQuad_F3Vertices[0].precise[2] = -9.0f;
               GPU.off_u = GPU.off_v = 99;
               GPU.may_be_2d = false;
               saved.loc = 0;
               if (!GPU_StateAction(&saved, 1, 0) || GPU.InCmd != INCMD_QUAD ||
                   GPU.InQuad_F3Vertices[0].precise[2] != ((kind == 4 || kind == 5) ? 1.0f : 8.0f) ||
                   GPU.off_u != 1 || GPU.off_v != 1 || !GPU.may_be_2d)
                  return 5;
               free(saved.data);
            }
            second[0] = xy(48, 48);
            shadow(0, 48, 48,
                  (kind == 1 || kind == 6) ? VALID_01 : (kind == 2 || kind == 4) ? 0 : VALID_012,
                  kind == 3 ? 16.0f : kind == 5 ? 1.0f : 8.0f);
            Command_DrawPolygon_NV4_G0_T1_BMopaque_TM0_MO2_ME0_PG1(&GPU, second);
            if (observed_quad_count != 1 || observed_triangle_count || GPU.InCmd != INCMD_NONE)
               return 3;
            check(names[kind], observed_possible_2d == expected_2d);
            check("packed UV correction", observed_uv_offset == (expected_2d ? 0x10001u : 0));
            for (i = 0; i < 4; i++)
               check("submitted depth", observed_w[i] == ((kind == 3 && i == 3) ? 16.0f : expected_w));
            for (i = 0; i < 8; i++)
            {
               static const float expected_xy[] = {16, 16, 48, 16, 16, 48, 48, 48};
               static const uint16_t original_uv[] = {31, 31, 0, 31, 31, 0, 0, 0};
               check("submitted coordinates", observed_xy[i] == expected_xy[i]);
               check("submitted UV", observed_uv[i] == original_uv[i] + (expected_2d ? 1 : 0));
            }
            check("lower U clamp", observed_limits[0] == (expected_2d ? 1 : 0));
            check("lower V clamp", observed_limits[1] == (expected_2d ? 1 : 0));
            check("upper U clamp", observed_limits[2] == 31);
            check("upper V clamp", observed_limits[3] == 31);
         }
   printf("GPU UV decoder: %u cases, %u assertions, %u failures\n", cases, assertions, failures);
   return failures ? 1 : 0;
}
