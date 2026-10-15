# RHI API and OpenGL 3.3 Renderer

## RHI API

The RHI API is the means by which components of the Beetle PSX libretro core interface with the hardware renderers. The RHI API offers two general classes of functions for the core. The first class consists of various functions used to perform libretro-specific actions such as reading core options or preparing/finalizing the current emulation loop frame. The second class of functions consists of RHI interface functions that the emulated PSX GPU uses to issue commands to the hardware renderer. Not every emulated PSX GPU command has a corresponding RHI interface function, but the set of available functions can be extended or modified as necessary when bugs are discovered and higher accuracy is required.

Each unique hardware renderer will implement RHI interface functions as another layer of function calls, typically but not necessarily one per RHI interface function. The RHI interface should then select the correct function to call based on the currently running hardware renderer.

The RHI API also includes support for dumping RHI API calls to file, which can be utilized for debugging purposes by any renderers that implement RHI playback.

## OpenGL 3.3 Renderer

The OpenGL renderer is currently implemented in `rhi_lib_gl.c` and can be called via the functions exposed in `rhi_lib_gl.h`.

## Color modes

OpenGL and Vulkan share **Internal Color Depth**. **16 bpp** is the default.
These names describe rendering policy, not the monitor output format or the
physical format of every intermediate GPU image.

| Setting | Ordinary PS1 drawing | Filtered/HD drawing and core resolves | SDR display |
| --- | --- | --- | --- |
| Native (Unexpanded) | RGB5 source/result operations | Reduce to RGB5, including the final resolved image | Five-bit levels expanded as q*8; white is 248 |
| 16 bpp | The same RGB5 operations | Preserve extra shades from enhancements | Expand the RGB5 range to full white 255 |
| 32 bpp | Preserve byte color precision | Preserve extra shades | Full byte range, without RGB5 scanout reduction |

Native and 16 bpp have the same 32 ordinary levels per channel. Their white
expansion differs. Native deliberately also reduces enhanced results; 16 bpp
preserves the extra shades produced by filtering, replacement textures and
supported antialiasing/downsampling. An ordinary draw over an enhanced
background still applies its ordinary RGB5 destination/result rules.

**Dithering Pattern** remains independent of color depth. Eligible GP0 drawing
commands add the signed PS1 pattern to the source before saturation and color
reduction. Raw textures bypass modulation and this dither. In 32 bpp, enabling
dither does not impose RGB5 scanout. Native-resolution and internal-resolution
patterns retain their existing choices. OpenGL does not gain Vulkan-only
antialiasing/downsampling settings.

The emulated PS1's actual 24-bit display mode, commonly used for FMV, bypasses
RGB5 reduction and the 248 white limit in all three settings.

Framebuffer-texture feedback retains its existing native-word modulation rules
even in 32 bpp. Its SDR word results use q*8 storage; 16-bpp display expansion
is applied afterwards. Converting color modes preserves whether textures came
from GPU drawing or CPU uploads, so a setting change cannot change these rules.

### HDR

**Color Format** controls frontend output and requires a restart. When the
frontend accepts HDR10, the existing wide HDR rendering policy overrides the
selected internal color mode, preserves its extra precision and disables PS1
dithering. Rejected HDR requests use the selected SDR policy. Internal Color
Depth therefore does not select the display's 8-bit or 10-bit output format.
FP16 rendering is used where supported. If the device cannot render that
format, the existing byte-target fallback retains HDR output and clamps
over-white content.

### Existing configurations

The option key and saved values `16bpp(native)` and `32bpp` are retained.
The former now displays as **16 bpp**; `native` selects **Native (Unexpanded)**.
The separate Vulkan native-color toggle is removed. Re-select Internal Color
Depth when updating a configuration that used that toggle; there is no
automatic migration between the old and new settings.

### Scope and validation

Legacy Vulkan dithering was intentionally a late scanout presentation effect.
This change applies eligible PS1 dithering during drawing and uses scanout for
color expansion. Changing that presentation is intentional; the old effect was
not originally presented as hardware-accurate rendering.

These modes do not establish complete PS1 hardware accuracy. Geometry, texture
sampling, timing and interlacing still have separate limitations. Exact native
line selection remains limited to native settings with PGXP off; expanded
quads retain the enhanced/PGXP coverage needed to avoid geometry gaps.
Pre-existing mask/copy defects are separate from this color policy.

Arithmetic fixtures, canonical VRAM comparisons and displayed gameplay
comparisons test different contracts. A passing finite suite is not a claim
that every game, enhancement or driver matches original hardware.

## Building

The RHI API and OpenGL renderer are components of the Beetle PSX libretro core. To build with OpenGL support, run `make HAVE_OPENGL=1` in the repository's top level directory. To build with all possible hardware renderers, instead run `make HAVE_HW=1`. To build with dump support, additionally pass `RHI_DUMP=1`.

## Coding Style

The preferred coding style for the rhi subdirectory is the libretro coding style. See: https://docs.libretro.com/development/coding-standards/

## Credits

The OpenGL renderer was originally authored by simias as a port/plugin of Rustation's GL renderer to Beetle PSX.
