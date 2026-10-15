# Color rendering validation

Tested on Windows/NVIDIA and Android/Adreno and Mali, using OpenGL/OpenGL ES
and Vulkan. Results cover the fixtures and settings below, not complete PS1
hardware accuracy or whole-game parity.

## Accuracy

The main 32-bpp fixture checks ten color sources, every 32×32 RGB5
source/background pair, opaque drawing and all four blend operations.
Conditions: 1x, nearest filtering, SDR, PGXP off and dithering off.

| GPU | OpenGL: incorrect cases / tested | Vulkan: incorrect cases / tested |
| --- | ---: | ---: |
| NVIDIA | 0 / 51,200 | 0 / 51,200 |
| Adreno | 0 / 51,200 | 0 / 51,200 |
| Mali | 0 / 51,200 | 0 / 51,200 |

The broader suite reports **zero invalid results in 996,160 case-pair
evaluations**, including repeated values and settings. It covers:

- Native, 16 bpp and 32 bpp; mixed RGB values and masked draws.
- Uploads, indexed textures, framebuffer feedback, copies and color-mode changes.
- 1x and 4x, PGXP color, HD menu/hotkey changes, filtering and supersampling.
- HDR acceptance/fallback, retained palettes, reset/restoration and selected
  allocation/readback/context-recovery paths.

RGB5 checks use independent integer formulas; byte-color checks allow either
neighbor at exact half-code ties. Filter fixtures use constant-color blocks,
so they do not establish arbitrary filtered gameplay parity. HDR checks cover
shader/data behavior, not physical display calibration.

## Build and regression checks

- **23,097,578** maintained interpolation/line checks; zero failures.
- **52** Windows/Android GNU89 syntax checks, including `RHI_DUMP`.
- **423** shader reproductions and validations across changed commit boundaries;
  **83** distinct final variants. Shaderc v2023.8; Vulkan 1.0 validation.
- Shader-expression drift guard and HD-toggle latch tests pass.

## Performance

See the [archived performance report](https://github.com/Starlightbotanist/beetle-psx-libretro-archive/blob/8f89fd1f10c8bb3b7dfb1f4162c2b0ac920561dc/reports/unified-color-rendering/PERFORMANCE.md) for direct
upstream-versus-PR FPS comparisons, settings and measurement limits.

## Limits

Physical HDR presentation remains unmeasured. Some HD/failure-injection checks
were Windows-only.
Mali falls back from MSAA. Some lifecycle probes remain inconclusive.
Native-word restoration does not prove preservation of every enhanced shade.
GL's rendered-palette cache still reduces extra 32-bpp shades to RGB5; existing
mask/copy defects and timing/interlacing differences remain separate.
