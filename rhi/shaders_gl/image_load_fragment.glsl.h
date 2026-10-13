#include "shaders_common.h"

static const char *image_load_fragment = GLSL_FRAGMENT(
      uniform highp sampler2D fb_texture;
      uniform uint internal_upscaling;
      // 1: RGB5 normalized upload to q*8; 2: q*8 readback to RGB5 storage.
      uniform uint color_conversion;
      // Uploads rebuild the derived stencil from the authoritative mask bit.
      // 0: color copy, 1: protected pixels for the stencil-only pass.
      uniform uint mask_pass;
      in vec2 frag_fb_coord;
      out vec4 frag_color;

      // Read a pixel in VRAM
      vec4 vram_get_pixel(int x, int y) {
	return texelFetch(fb_texture, ivec2(x, y) * int(internal_upscaling), 0);
      }

      void main() {
      // GLES 5551 note: color reinterpretation can be done here
      frag_color = vram_get_pixel(int(frag_fb_coord.x), int(frag_fb_coord.y));
      if (mask_pass != 0u && frag_color.a <= 0.5)
         discard;
      if (color_conversion == 1u)
         frag_color.rgb = floor(frag_color.rgb * 31.0 + vec3(0.5)) * (8.0 / 255.0);
      if (color_conversion == 2u)
         frag_color.rgb = clamp(floor(frag_color.rgb * (255.0 / 8.0) +
               vec3(0.001)), vec3(0.0), vec3(31.0)) / 31.0;
      }
);
