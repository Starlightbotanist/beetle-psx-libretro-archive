# Vulkan pipeline cache tests

The test extracts production persistence code and uses deterministic Vulkan
and filesystem doubles. It needs no game data or GPU.

From the repository root:

```
python3 tools/vulkan_cache/test_persistence.py
```

The optional Windows mode requires WSL, MinGW-w64 and Windows interop. It
cross-compiles and runs the native Windows locking and replacement tests:

```
python3 tools/vulkan_cache/test_persistence.py --windows
```

Coverage includes SYSTEM/SAVE merging, concurrent writers, process locks,
invalid data, I/O failures, atomic replacement, bounded quarantine and
unchanged-data suppression. Real devices remain necessary for driver and
performance testing.
