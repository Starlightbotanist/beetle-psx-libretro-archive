# Packed RHI interpolation tests

Run `make check` with a C compiler. No GPU, core, BIOS or game is required.

The test includes the shipping `rhi_interpolation.h`. It checks descending-X
endpoint packing, zero-length lines, anchor ties, and color planes sampled at
software-style fixed-point DDA pixels for every endpoint pair in [-4,4]^2
(6,561 lines). It also compares 20,000 deterministic triangle inputs against
barycentric derivatives and an 8.12 accumulator, including constant channels,
negative coordinates, reversed winding and wrapping. Degenerate triangles are
skipped, matching their rejection before this helper is called.

These are CPU packing/interpolation tests. They do not execute either shader or
establish GPU coverage, blending, filtering, scanout or complete game accuracy.
