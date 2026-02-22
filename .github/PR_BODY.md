<!-- Please ensure your PR title is brief and descriptive for a good changelog entry -->
<!-- Link to issue if there is one -->

## What it solves

- Refactors the Python bindings and build so the GNUBG engine is built as a static library and the `_gnubg` extension links against it.
- Adds missing API: `evaluate()`, `findbestmove()`, `rolloutcontext()`.
- Makes `make test` pass and improves test coverage.
- Fixes wheel builds and CI for Linux, macOS, and Windows (MSYS2/UCRT64).
- Fixes lint (cppcheck, clang-format, cpplint) and macOS readline/libedit build.

## How this PR fixes it

- **Build**: Meson builds static `libgnubg` from GNUBG C sources and `gnubg_lib.c`/stubs; extension module links to it. Python packaging uses meson-python.
- **API**: Implements and exposes `evaluate`, `findbestmove`, `rolloutcontext` in `gnubgmodule.cpp` with tests.
- **CI (Linux)**: Installs readline, glib, gmp, sqlite, libpng; uses `CIBW_BEFORE_BUILD` for manylinux/musllinux; cleans `build/` before cibuildwheel.
- **CI (macOS)**: Homebrew deps, `PKG_CONFIG_PATH`/`LDFLAGS`/`CPPFLAGS`; `MACOSX_DEPLOYMENT_TARGET=14.0` for delocate; readline/libedit guarded in `gnubg_lib.c`.
- **CI (Windows)**: MSYS2 UCRT64 setup; installs gcc, pkg-config, glib2, gmp, readline, sqlite3, libpng, gettext; sets `PATH` (ucrt64/bin + usr/bin for `sh`/credits.sh) and `PKG_CONFIG_PATH`. Meson uses `-mno-mmx`/`-mno-avx`/`-mno-avx2` and `win32_stub` (stub x86intrin.h/emmintrin.h) so winnt.h does not pull in broken MinGW MMX/AVX intrinsics; links `winmm` for sound. Release workflow uses `PYPI_TEST_TOKEN` for TestPyPI.
- **Local Windows build**: `scripts/build-windows-local.ps1` mirrors CI (MSYS2 env, TEMP build dir to avoid Meson lock on network/WSL drives); README “Building on Windows (local)” documents one-time pacman deps and usage.
- **Lint**: cppcheck suppressions, clang-format applied, cpplint on bindings (excludes upstream `gnubg_lib.c`); uninit vars fixed in `gnubg_lib.c`. C++ `extern "C"`/glib/Windows include order fixed for MinGW.

## How to test it

- `make test` (or `pytest tests/`) after building.
- **Windows (local)**: Install MSYS2 + UCRT64 deps (see README), then run `.\scripts\build-windows-local.ps1` (optionally `-Wheel` for cibuildwheel).
- **CI**: Push and check Build & test wheels workflow on ubuntu-latest, macos-14, windows-latest for multiple Python versions.
- Optionally run release workflow (TestPyPI) and install the built wheel.

## Readiness Checklist

### Author/Contributor
- [ ] If documentation is needed for this change, has that been included in this pull request
- [x] Pull request title is brief and descriptive (for a changelog entry)

### Reviewing Maintainer
- [ ] Label as `breaking` if this is a large fundamental change
- [ ] Label as either `automation`, `bug`, `documentation`, `enhancement`, `infrastructure`, or `performance`
