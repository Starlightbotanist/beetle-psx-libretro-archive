/* Check the real software decoder/rasterizer through GP0 readback.
 * These fixtures have exactly representable native UV planes,
 * so an independent rational reference specifies native-grid samples. */
#include <stdio.h>
#include <string.h>
#include "../../mednafen/psx/gpu.c"

bool psx_gpu_rasterize_both_fields;
uint8_t psx_gpu_upscale_shift_hw;
int psx_pgxp_color, psx_pgxp_fog, psx_pgxp_2d_tol = -1;
int line_render_mode;
enum rhi_renderer_type rhi_intf_is_type(void) { return RHI_SOFTWARE; }
bool rhi_intf_has_software_renderer(void) { return true; }
#include "test_stubs.h"

static unsigned failures, cases, assertions;
static uint32_t xy(int x, int y) { return (x & 65535) | (uint32_t)(y & 65535) << 16; }
static int64_t cross(int ax, int ay, int bx, int by) { return (int64_t)ax * by - (int64_t)ay * bx; }

int main(void)
{
   static const unsigned orders[6][3] = {{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
   unsigned scale, shape, flip, order, mode, x, y;
   for (scale = 0; scale < 5; scale++)
   {
      memset(&GPU, 0, sizeof(GPU));
      GPU.upscale_shift = scale;
      GPU.vram = VRAM_Alloc(scale);
      if (!GPU.vram) return 2;
      GPU.ClipX1 = 1023;
      GPU.ClipY1 = 511;
      SetTPage(&GPU, 0x11c);
      for (y = 0; y < 256; y++)
         for (x = 0; x < 256; x++)
            texel_put(768+x, 256+y, x | y << 8);
      for (shape = 0; shape < 4; shape++)
         for (flip = 0; flip < 4; flip++)
            for (order = 0; order < 6; order++)
               for (mode = 0; mode < 3; mode++)
               {
                  int vx[3] = {16,shape >= 2 ? 528 : 48,16+(shape == 1 ? 2 : shape == 3 ? 32 : 0)};
                  int vy[3] = {16,16,shape >= 2 ? 496 : 48};
                  int limit = shape >= 2 ? 255 : 31;
                  int u[3] = {0,limit,0}, v[3] = {0,0,limit};
                  uint32_t command[7] = {0x25808080};
                  uint32_t readback[3] = {0xc0000000, 0, 0x200020};
                  uint16_t pixels[1024];
                  unsigned i, checked = 0;
                  if (mode && shape >= 2) continue;
                  cases++;
                  for (i = 0; i < 3; i++)
                  {
                     if (flip & 1) u[i] = limit-u[i];
                     if (flip & 2) v[i] = limit-v[i];
                  }
                  for (y = 16u << scale; y < 48u << scale; y++)
                     memset(GPU.vram + y * (1024u << scale) + (16u << scale), 0,
                           (32u << scale) * sizeof(uint16_t));
                  GPU.DrawTimeAvail = 1000000;
                  InvalidateCache(&GPU);
                  for (i = 0; i < 3; i++)
                  {
                     unsigned k = orders[order][i];
                     command[1+i*2] = xy(vx[k], vy[k]);
                     command[2+i*2] = u[k] | v[k] << 8 | (i == 1 ? 0x11c << 16 : 0);
                  }
                  if (mode == 0)
                     Command_DrawPolygon_NV3_G0_T1_BMopaque_TM0_MO2_ME0_PG0(&GPU, command);
                  else
                  {
                     tri_vertex vertices[3];
                     i_deltas perspective;
                     float iw, uw, vw;
                     int32_t pu, pv;
                     memset(vertices, 0, sizeof(vertices));
                     for (i = 0; i < 3; i++)
                     {
                        unsigned k = orders[order][i];
                        vertices[i].x = vx[k] << scale;
                        vertices[i].y = vy[k] << scale;
                        vertices[i].u = u[k];
                        vertices[i].v = v[k];
                        vertices[i].precise[0] = vertices[i].x;
                        vertices[i].precise[1] = vertices[i].y;
                        vertices[i].precise[2] = mode == 2 ? 1e8f : (float)(8u << k);
                     }
                     if (!CalcIDeltas_Persp(&perspective, vertices, vertices+1, vertices+2)) return 5;
                     iw = 1.0f / vertices[0].precise[2];
                     uw = vertices[0].u * iw;
                     vw = vertices[0].v * iw;
                     PCT_UVBatch(1, 0.5f, &iw, &uw, &vw, 0, 0, 0,
                           perspective.min_u, perspective.max_u, perspective.min_v, perspective.max_v, &pu, &pv);
                     if ((mode == 1 && pu < 0) || (mode == 2 && pu != -1)) return 6;
                     /* A nonuniform valid perspective plane differs from affine.
                      * The next mode uses an observed invalid-reciprocal sentinel
                      * after that success and must reach the affine fallback. */
                     DrawTriangle_g0_t1_BMopaque_TM0_MO2_ME0(&GPU, vertices, true);
                  }
                  readback[1] = xy(16,16);
                  Command_FBRead(&GPU, readback);
                  for (i = 0; i < 512; i++)
                  {
                     uint32_t word = GPU_ReadDMA();
                     pixels[i*2] = word;
                     pixels[i*2+1] = word >> 16;
                  }
                  if (GPU.InCmd != INCMD_NONE) return 3;
                  for (y = 16; y < 48; y++)
                     for (x = 16; x < 48; x++)
                     {
                        int64_t n0 = cross(vx[1]-(int)x, vy[1]-(int)y, vx[2]-(int)x, vy[2]-(int)y);
                        int64_t n1 = cross(vx[2]-(int)x, vy[2]-(int)y, vx[0]-(int)x, vy[0]-(int)y);
                        int64_t n2 = cross(vx[0]-(int)x, vy[0]-(int)y, vx[1]-(int)x, vy[1]-(int)y);
                        unsigned ru, rv;
                        uint16_t expected, actual;
                        if (n0 <= 0 || n1 <= 0 || n2 <= 0) continue;
                        {
                           int64_t den, un, vn;
                           if (mode == 1) { n0 *= 4; n1 *= 2; }
                           den = n0+n1+n2;
                           un = u[0]*n0+u[1]*n1+u[2]*n2;
                           vn = v[0]*n0+v[1]*n1+v[2]*n2;
                           /* Exact half ties are outside the floating perspective
                            * oracle; all affine ties remain checked above. */
                           if (mode == 1 && ((2*un)%(2*den)==den || (2*vn)%(2*den)==den)) continue;
                           ru = (2*un+den)/(2*den);
                           rv = (2*vn+den)/(2*den);
                        }
                        expected = ru | rv << 8;
                        actual = pixels[(y-16)*32+x-16];
                        checked++;
                        assertions++;
                        if (expected != actual)
                        {
                           if (failures < 8)
                              fprintf(stderr,"scale=%u shape=%u flip=%u order=%u mode=%u pixel=%u,%u got=%u expected=%u\n",
                                    1u<<scale,shape,flip,order,mode,x,y,actual,expected);
                           failures++;
                        }
                     }
                  if (checked < 350) return 4;
               }
      free(GPU.vram);
   }
   printf("Software UV native-grid: %u cases, %u pixels, %u failures\n", cases, assertions, failures);
   return failures ? 1 : 0;
}
