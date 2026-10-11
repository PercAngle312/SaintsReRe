# Asset-free texture checks

Apply `patches/rexglue-sdk.patch` to the SDK revision in `scripts/setup.ps1`, then run:

```sh
python3 tests/graphics/check_texture_cache.py build/rexglue-sdk
```

Requires Python 3 and a C++20 compiler (`g++` by default; use `--cxx` to select another).
The runner extracts the actual patched `FindOrCreateTexture` and
`IsRangeScaledResolved` method bodies and compiles them with a small backend
fixture. GPU allocation, texture keys and CPU page-map writes are represented
by test doubles; bitmap reads and lookup-cache logic are the SDK code itself.
It runs in a temporary directory with caching both enabled and disabled.

Checks cover bitmap boundaries, range clamping, 1x classification, cache hits,
resolve invalidation, and readers blocked while a CPU writer holds the critical
region between epoch invalidation and page-map updates. Removing the
page-map reader lock causes the synchronization regression check to fail.

GitHub Actions also checks that the **complete** SDK patch applies to the pinned
revision. These checks do not compile the full SDK or game, exercise D3D12,
measure performance, or validate gameplay. No game files or generated game
code are needed or uploaded.
