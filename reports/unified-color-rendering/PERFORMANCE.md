# Unified color rendering: upstream versus PR performance

Direct RetroArch measurements on Windows/NVIDIA, Odin 2/Adreno and Pixel 6/Mali.
Values are emulator throughput, not a game’s normal frame rate. Higher FPS is better;
positive percentages mean faster throughput. These selected workloads do not establish
performance across all games or settings.

## Conditions

- **Native textured workload:** 1x, 16 bpp, native dithering.
- **Upscaled blended workload:** 4x, 32 bpp, dithering off.
- Both: SDR, nearest, PGXP off, Software Framebuffer off, interpreter at 100%;
  no MSAA, supersampling, HD replacements or frontend shaders.
- Same saved inputs and frame counts; isolated configurations. Upstream Vulkan’s
  native-color toggle is off, so the native row compares its legacy default with
  the PR’s shared 16-bpp policy. This is a performance comparison, not equal output.
- Two repeats per build/cell in upstream–PR–PR–upstream order. Normally 1,200
  frames with 200 warmup frames discarded. NVIDIA upscaled Vulkan uses 6,000/1,000;
  Adreno upscaled OpenGL ES uses 2,400/400 to confirm the larger differences.
- Android devices verified awake; emulation thread pinned to the recorded big core.
  FPS is derived from mean frame time. Exact hashes and per-run values are in
  [RESULTS.json](RESULTS.json).

## Results

Cells show **FPS (ms/frame)**; percentages compare the PR directly with upstream.

| GPU / API | 16 bpp 1x: upstream | 16 bpp 1x: PR | Change | 32 bpp 4x: upstream | 32 bpp 4x: PR | Change |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| NVIDIA / OpenGL | 260.86 (3.833) | 261.17 (3.829) | +0.12% | 208.89 (4.787) | 217.02 (4.608) | +3.89% |
| NVIDIA / Vulkan | 318.53 (3.139) | 313.34 (3.191) | -1.63% | 198.20 (5.045) | 232.20 (4.307) | +17.16% |
| Adreno / OpenGL ES | 134.70 (7.424) | 184.98 (5.406) | +37.33% | 120.02 (8.332) | 99.34 (10.067) | -17.23% |
| Adreno / Vulkan | 231.46 (4.320) | 230.14 (4.345) | -0.57% | 125.37 (7.976) | 120.01 (8.333) | -4.28% |
| Mali / OpenGL ES | 36.85 (27.140) | 43.98 (22.739) | +19.35% | 78.60 (12.722) | 80.33 (12.448) | +2.20% |
| Mali / Vulkan | 133.08 (7.514) | 132.85 (7.527) | -0.17% | 68.31 (14.639) | 62.06 (16.114) | -9.15% |

**The larger confirmed loss is Adreno OpenGL ES at 4x/32 bpp:**
120.02 → 99.34 FPS (-17.23%).
It persists in the longer repeats and should be considered before submitting the PR.

Small differences near 1–3% remain subject to run variation. Adreno’s upscaled
OpenGL ES upstream result and Vulkan PR result cluster near 120 FPS; frontend
presentation pacing is not ruled out. These are observed RetroArch throughput
differences, not isolated shader costs. No HDR performance claim is made.

## Provenance

The upstream source is the PR base. Frozen PR binaries have verified equivalent
rendering code; later documentation changes do not alter the measured implementation.
Core hashes, source identities, frame counts, settings and repeat spreads are retained
in RESULTS.json. Runtime tests, not compiler or arithmetic tests, supply these timings.
The report and matching source snapshot are retained in the archive repository;
links to this report use its exact commit and survive topic-branch cleanup.
