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
- **CI (Windows)**: MSYS2/UCRT64 setup, installs glib2, gmp, readline, sqlite3, libpng, pkg-config; sets `PATH`/`PKG_CONFIG_PATH` for PowerShell so Meson finds deps. Release workflow uses `PYPI_TEST_TOKEN` for TestPyPI.
- **Lint**: cppcheck suppressions, clang-format applied, cpplint header guard and includes; uninit vars fixed in `gnubg_lib.c`. C++ `extern "C"`/glib/Windows include order fixed for MinGW.

## How to test it

- `make test` (or `pytest tests/`) after building.
- CI: push and check Build & test wheels workflow on ubuntu-latest, macos-14, windows-latest for multiple Python versions.
- Optionally run release workflow (TestPyPI) and install the built wheel.

## Readiness Checklist

### Author/Contributor
- [ ] If documentation is needed for this change, has that been included in this pull request
- [x] Pull request title is brief and descriptive (for a changelog entry)

### Reviewing Maintainer
- [ ] Label as `breaking` if this is a large fundamental change
- [ ] Label as either `automation`, `bug`, `documentation`, `enhancement`, `infrastructure`, or `performance`
