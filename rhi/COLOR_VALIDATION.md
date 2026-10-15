# Unified color rendering: validation and limits

Validation collected October 13–15, 2026. These results cover the named finite
fixtures, scenes and settings. They do not establish complete PS1 hardware
accuracy or full gameplay parity between OpenGL and Vulkan.

## References and build identities

- **Upstream comparison:** untouched upstream master at the time of testing.
  Its OpenGL 16-bpp mode and Vulkan Native 15-bit Color Rendering option are
  used for the RGB5 arithmetic sweep. Vulkan's legacy default is used for
  the separate default-performance comparison.
- **Earlier optimized rewrite:** the shared color modes with the accepted
  framebuffer, uniform and GLES mapping optimizations, before the later
  line-policy and 32-bpp feedback corrections. Results from that build are
  identified below rather than presented as fresh final-build results.
- **Published 32-bpp predecessor:** the shared modes and line-policy follow-up,
  before the two Silent Hill corrections. This is the failing control in the
  source-equivalence and logo/fade tests; it is not untouched upstream.
- **Banding correction:** the predecessor plus write-provenance clipping and
  no-op-copy handling, before full-range source expansion.
- **Final rendering implementation:** the banding correction plus consistent
  full-range 32-bpp sources and preservation of direct rendered byte colors.
  The prepared series preserves this tested rendering implementation. History
  consolidation changes comments/documentation and a C89 declaration placement;
  the latter has identical Windows release assembly and Android LTO compiler
  output with and without `RHI_DUMP`.

The upstream comparison is the parent of this series. Test binary version
strings are not proof of source identity: some cores were built from a recorded
base plus a frozen source patch. The final 32-bpp Windows core has SHA-256
`b0250c609eb95d8d62376839458bed027f63e7c6a663f5dc619204ad2f0e159d`;
the Android core has SHA-256
`c342d5c47fa869f177ca0086100fffd9f84fd89c2d4f63a80e8233a0497b201b`.
They are not relabeled as newly rebuilt cores after history consolidation.

## Primary accuracy matrix

Common RGB5 conditions: 1x, nearest filtering, PGXP off, SDR, native dithering.
The rewrite uses expanded 16 bpp. The RGB5 reference is independent integer
texture modulation, dithering and blending, with original input words as the
upload oracle. It measures native color codes rather than white expansion.

The 32-bpp tests use SDR, nearest filtering, PGXP off and dithering disabled.
Their reference is an independent nearest-byte blend model. At exact half-code
ties either neighboring byte is allowed; those differences are not errors.

All values are incorrect samples or cases, not percentages of gameplay pixels.
The columns intentionally identify different comparison builds.

| GPU / API | Earlier RGB5 texture/shading/dither: upstream → rewrite / 262,144 | Earlier RGB5 blend: upstream → rewrite / 131,584 | Earlier uploads: upstream → rewrite / 65,536 | Final 32-bpp ten-producer source/blend cases / 51,200 | Logo/fade mismatching captures: published predecessor → final / 22 |
| --- | ---: | ---: | ---: | ---: | ---: |
| NVIDIA / OpenGL | 63,564 → 0 | 50,432 → 0 | 0 → 0 | 0 | 21 → 0 |
| NVIDIA / Vulkan | 0 → 0 | 0 → 0 | 0 → 0 | 0 | 21 → 0 |
| Adreno / OpenGL ES | 67,112 → 0 | 44,342 → 0 | 65,406 → 0 | 0 | 21 → 0 |
| Adreno / Vulkan | 0 → 0 | 0 → 0 | 0 → 0 | 0 | 21 → 0 |
| Mali / OpenGL ES | 58,424 → 0 | 35,386 → 0 | 0 → 0 | 0 | 21 → 0 |
| Mali / Vulkan | 0 → 0 | 25,824 → 0 | 0 → 0 | 0 | 21 → 0 |

The texture domain is 32 gray texture levels × 256 RGB shading triplets ×
16 dither positions × two mask states. It produces 11,159 distinct expected
RGB5 colors, not every possible texture/shading combination. Blend samples
cover 16,385 scenario blocks, four operations and repeated overlapping writes.
Upload checks cover every 16-bit word, including both mask-bit states. GLES
RGBA5551 readback is decoded before comparison with PS1 ABGR1555; the Adreno
upload result was also verified independently in displayed VRAM.

The final ten-producer test covers all 32×32 RGB5 source/background pairs and
opaque drawing plus four blend operations. Producers include uploaded raw and
neutral textures, indexed textures, rendered raw/neutral feedback, copied
feedback, flat/Gouraud triangles, lines and opaque/transparent texture words.
The failing matched 10,240-pair subset improves from 6,322 incorrect cases on
NVIDIA, 6,138 on Adreno and 6,086 on Mali to zero, per renderer.

The expanded final suite has 996,160 case-pair evaluations across all devices,
including repeated values and settings. It reports zero invalid results.
It includes mixed RGB values, Native/16-bpp controls, masked draws, 4x,
bilinear/3-point/JINC2/SABR/xBR constant-color blocks, PGXP color, dithering,
supersampling and requested Vulkan MSAA. Mali falls back to single sampling;
it is not an MSAA validation. NVIDIA/Adreno Vulkan masked-texture tests have
ten legal half-code-tie differences, counted separately from invalid values.
Constant-color filter tests do not establish arbitrary filtered gameplay parity.

## Gameplay and state transitions

- **Silent Hill dark alley, issue #967:** the darkness was allowed to settle
  before measuring both scenes. At 1x, expanded 16 bpp, nearest, PGXP off and
  native dithering on NVIDIA, each static 288-pixel region improves from
  288 differing OpenGL pixels and 112 differing Vulkan pixels to zero against
  software RGB5 expanded to full range. Whole images still differ because
  timing, coverage and game state are not identical to software. This is a
  static-color result, not whole-image parity.
- **Silent Hill logos and fades:** fresh-boot sequences sample 22 frames at
  15-frame intervals across Konami and KCET on all three GPUs and both APIs.
  The final source policy removes the white/background seam throughout the
  tested fades; the test requires white and intermediate brightness samples
  so a blank frame cannot pass.
- **Line coverage:** software-DDA fixtures cover flat and Gouraud lines,
  endpoint reversal, half-step ties and maximum legal spans. The GL gate
  follow-up removes 5,658 differing coverage pixels in the affected NVIDIA
  3,570-line fixture. Recorded final gate fixtures pass on all three GPUs.
  PGXP/upscaled expanded quads remain a separate coverage policy; gameplay
  controls include Vagrant Story dialogue and the user-confirmed RRT4 tunnel.
- **Copies and options:** final tests on all three GPUs/both APIs report zero
  invalid samples in 640 mode/resolution/filter/HD-cycle samples and 2,560
  repeated mixed CPU/GPU copy samples per API. HD menu and session-hotkey
  changes use the same effective replacement policy.
- **HDR:** accepted-HDR source/control and Internal Color Depth override
  comparisons have zero differences in 143,360 compared pixels per API/GPU.
  Initially rejected requests follow SDR policy, with zero invalid samples
  in the 64-sample control. Color Format requires a restart; changing its menu
  value in a running session does not renegotiate output.
- **Lifecycle:** recorded reset, restoration, format changes, latched palettes,
  actual FP16 mirrors, context recreation and allocation/readback recovery
  checks cover the affected paths. Some mobile failure injections remain
  unmeasured. Three synthetic lifecycle cases stall on both the predecessor
  and untouched upstream and remain inconclusive, rather than passing.

## Maintained and build checks

`tools/rhi_interpolation` reports 23,097,578 scalar checks and zero failures
under GNU89 with `-Wall -Wextra -Werror`. The shader-expression guard detects
drift between the C line test and both GLSL implementations. The independent
line reference walks the software-style 32-fractional-bit DDA, including its
initial coordinate bias. Triangle derivative tests share the plane formula;
they are not an independent rasterizer coverage oracle.

The prepared functional commits are checked with native Windows and Android
GNU89 syntax checks of affected shared RHI units, including `RHI_DUMP`. Shader-changing
commit boundaries reproduce their prebuilt variants with shaderc v2023.8,
glslang 11.1.0-882-ga91631b2 and SPIR-V generator `0x000d000b`; binaries are
validated for Vulkan 1.0. Final shader comments do not change generated blobs.
The preparation pass completes 52 syntax checks and 423 shader reproductions
and validations across the changed commit boundaries. The maintained HD-toggle
latch test also passes.

## Performance

These are uncapped emulator throughput measurements, not a game's normal
frame rate. Higher FPS is better. Separate workloads and baselines must not
be combined into a net speed claim. Short runs can vary by a few percent.

The earlier Vulkan-default comparison uses 1x, nearest, SDR, native dithering,
PGXP off, interpreter at 100%, two runs/build and 600 frames with 150 discarded
for warmup. It predates the final 32-bpp refinements. In Crash 3's hub, the
settled Silent Hill alley and the stationary RRT4 tunnel, FPS changes versus
the untouched legacy-default upstream build range from -2.04% to -0.93% on
NVIDIA, -3.71% to -1.99% on Adreno, and -0.88% to +0.70% on Mali. The worst
recorded sample is the Adreno alley: 207.03 → 199.34 FPS (-3.71%). It is not
presented as a fresh final-tip timing.

The final source-expansion comparison below uses the same frozen final
implementation tested in the accuracy audit: 4x nearest, 32 bpp, dithering off,
PGXP off, SDR, Software Framebuffer off, interpreter, Silent Hill's foggy
opening street. Two runs/build in ABBA order; 1,200 frames, 200 warmup.
The baseline already includes the banding correction, so this measures the
incremental expansion fix rather than the full PR versus upstream.

| GPU / API | Banding correction FPS | Final implementation FPS | Change |
| --- | ---: | ---: | ---: |
| NVIDIA / OpenGL | 220.57 | 217.44 | -1.42% |
| NVIDIA / Vulkan | 236.33 | 229.87 | -2.73% |
| Adreno / OpenGL ES | 102.27 | 99.73 | -2.49% |
| Adreno / Vulkan | 119.99 | 119.88 | -0.09%; near 120-FPS display cap |
| Mali / OpenGL ES | 79.63 | 80.05 | +0.52% |
| Mali / Vulkan | 63.03 | 63.17 | +0.24% |

Earlier direct upstream comparisons of the GLES mapping optimization show
35.390 → 44.111 FPS in FF7's opening station (+24.64%) and 72.890 → 84.502 FPS
in Vagrant Story's opening dialogue (+15.93%) on Mali. These selected scenes
use the earlier optimized rewrite, not newly rebuilt final cores. The broader
testing does not establish a universal performance gain or zero overhead.

## Remaining limitations

- Physical HDR presentation/calibration is unmeasured; HDR shader and stored
  data checks are separate from what a display actually shows.
- Arbitrary filtered/PGXP gameplay color parity, all drivers and all games are
  unmeasured. Native-word restoration does not prove preservation of every
  enhanced framebuffer shade across savestates or context loss.
- GL's rendered-palette cache retains RGB5 words in 32 bpp, losing enhanced
  shades that Vulkan can preserve. The NVIDIA palette/latch probe matches
  software native words, including same-CLUT overwrite and cache clear, but
  does not establish complete 32-bpp renderer parity. That specific mobile
  palette probe has not been rerun.
- Pre-existing mask/copy bugs, interlaced presentation and emulated GPU timing
  remain separate. The rejected timing/interlacing experiments are excluded.
- The old NVIDIA shadow-storage issue is already fixed upstream; it is a
  regression control, not a new fix attributed to this series.

Raw GPU fixtures, captures, run configurations and core/host hashes remain
separate development evidence. The maintained interpolation/latch tests and
this validation summary are included in the series.
