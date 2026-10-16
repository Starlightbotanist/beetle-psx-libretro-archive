# Unified color rendering: upstream versus PR performance

Direct RetroArch throughput on NVIDIA, Adreno and Mali. Higher FPS is faster; these values are controlled emulator throughput, not normal capped game frame rates.

## Conditions

- Native textured scene: 1x, 16 bpp, native dithering. Upscaled blended scene: 4x, 32 bpp, dithering off.
- SDR, nearest, PGXP off, CPU interpreter at 100%, Software Framebuffer off; no MSAA, supersampling, HD replacements or frontend shader.
- Same saved input and frame count within each cell. NVIDIA uses 4,000 frames/800 warmup; Adreno 1,800/200; Mali 2,400/400. Two runs per build. FPS is the reciprocal of the combined mean frame interval.
- Runs were ordered symmetrically; the reported upstream/PR subset is upstream–PR–PR–upstream. Android emulation threads were pinned to the recorded big core and devices verified awake.
- Upstream Vulkan uses its legacy wide-rendering default with scanout dithering in the native row. The PR uses draw-time RGB5. Equal settings here do not imply equal output.
- Mali ran on battery at 98–100%, with thermal status zero throughout. Exact settings, core/input hashes, run order and repeats are in [RESULTS.json](RESULTS.json).

## Results

| GPU / API | 16 bpp 1x upstream FPS | PR FPS | Change | 32 bpp 4x upstream FPS | PR FPS | Change |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| NVIDIA / OpenGL | 252.98 | 249.26 | -1.47% | 210.52 | 216.47 | +2.83% |
| NVIDIA / Vulkan | 317.35 | 312.70 | -1.47% | 194.99 | 231.55 | +18.75% |
| Adreno / OpenGL ES | 129.40 | 186.37 | +44.03% | 119.87 | 119.57 | -0.25% |
| Adreno / Vulkan | 233.40 | 231.67 | -0.74% | 125.41 | 134.02 | +6.86% |
| Mali / OpenGL ES | 34.29 | 43.26 | +26.17% | 78.19 | 88.25 | +12.86% |
| Mali / Vulkan | 130.37 | 132.68 | +1.77% | 68.67 | 65.13 | -5.15% |

**The remaining measured regression is Mali Vulkan in the 4x/32-bpp workload: 68.67 → 65.13 FPS (−5.15%).**

Small changes remain subject to run variation. Adreno OpenGL ES upscaled results approach 120 FPS, so presentation pacing is not ruled out. These measurements do not isolate shader cost or establish performance across all games, filters, HDR or devices.

Physical HDR presentation remains unmeasured; Mali fell back from requested MSAA in earlier setting checks. The original full-resolution framebuffer-feedback sampling remains intact. No software/interlacing discrepancies were pursued as part of this regression pass.

## Provenance

The archive branch retains a historical source snapshot. This report describes the newer tested rendering implementation identified in RESULTS.json, not an instruction to build the archive branch. The report’s immutable commit link survives topic-branch cleanup. Frozen binaries and source-equivalence checks establish the measured implementation; no new GPU timing was run during history cleanup.
