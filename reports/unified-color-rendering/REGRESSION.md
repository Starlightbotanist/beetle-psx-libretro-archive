# Color rendering regression controls

Reference: pre-optimization color rewrite, source-equivalent to
669549d2ccd286fdfd5a8b3940b2aa3aa510f554. Candidate: final optimized rewrite,
source-equivalent to37d48b35a5b8d9fd18e6a208593d6382626907cf. These are frozen
binaries; history cleanup changes neither rendering code nor shader blobs.

## Captured output

Sixty scenes, both APIs, SDR/nearest/PGXP off:1x16bpp/native dithering and
4x32bpp/dither off. Each pair uses the same GPU, input state and ending frame.
Selected controls cover Native1x, internal dithering4x16, and logicalHDR4x
with bilinear filtering and PGXP color/fog, including Vulkan true multipass.

| GPU | Primary pairs | Selected mode pairs | Other sampled pairs | Differing displayed pixels / native VRAM words |
| --- | ---: | ---: | ---: | ---: |
| NVIDIA | 240 | 84 | 48 intermediate frames | 0 / 0 |
| Adreno | 240 | 84 | 4 longer visible captures | 0 / 0 |
| Mali | 240 | 84 | None | 0 / 0 |

Primary NVIDIA runs use600frames/60warmup; Android240/40 except Mali
Chrono Cross600/40. Selected controls use240frames. Every native VRAM
comparison covers524,288words. Across1,024pairs, all game-memory and
accepted core/input identity checks pass. Available packed HDR outputs
also match, with symmetric availability and exact byte-length checks.

These are end-frame/sample comparisons, not continuous video or independent
hardware accuracy. Existing software disagreements remain separately
unclassified; software/interlacing bugs were not pursued.

## Finite arithmetic and spatial controls

Fresh Mali checks cover2,033,352 modulation, dither, byte-blend, HD/filter and
live-transition samples with zero incorrect results under their recorded
arithmetic references. These are mixed finite domains, not distinct colors
or a universal accuracy claim. Maintained interpolation/line tests report
23,097,578checks and zero failures.

The earlier full-resolution feedback fix remains intact. A fresh NVIDIA
nearest/raw feedback fixture at2x/4x/8x, all three color modes and both APIs
has zero source-versus-copy RGB differences in all18cases, preserving detail
inside native-pixel blocks. Fresh mobile/PGXP/HDR runtime coverage of this
specific spatial fixture is unmeasured; shared gates were inspected.

Physical HDR presentation remains unmeasured. Filtered fixtures do not prove
arbitrary filtered gameplay parity. Mask/copy defects and reduced precision
in OpenGL's rendered32-bpp palette cache remain separate limitations.
