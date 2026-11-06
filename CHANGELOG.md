## [Unreleased]

## [1.1.0a17] - 2026-11-06
### Added
- `cfevaluate()` (alias `cf_evaluate`): cubeful equities and the cube decision for a position (#23). `evaluate()` stays cubeless, as documented. The REST example's `/evaluate` now also returns `cubeful_equity` and a `cube` section.
- `gnubg.set_output(out, err)`: route gnubg's printed output and error messages to Python callables or file-like objects instead of the process's stdout/stderr (#26).
- Snake_case aliases for all C extension functions (e.g., `find_best_move`, `position_from_id`, `match_checksum`). The original camelCase names remain available for backward compatibility; the snake_case variants are the recommended idiomatic Python API.

### Fixed
- Linux wheels bundle GLib, so `import gnubg` works on systems without it, such as slim and alpine containers (#19).
- `hint()` no longer crashes: the returned objects were released twice (#20).
- `command("new session")` and `hint()` no longer hang. A calculation thread now starts at import, as in the gnubg application (#21).
- `findbestmove()` and `findbestmoves()` return whole moves when bearing off (#22, #18). `movetupletostring()` rejects incomplete move tuples.
- The evaluation cache now accounts for the ply setting, so results at one ply no longer leak into another (#24, a gnubg engine fix).
- `cubeinfo()` follows `set jacoby` and accepts `jacoby`/`beavers` positionally or by keyword. Calling it with 9 or 10 arguments no longer crashes (#25).
- `show()` returns the complete output, including the first line, so `show("board")` is no longer empty (#27, a gnubg engine fix).

### Changed
- macOS wheels are built on macOS 15 and require macOS 15 or later, because GitHub retired the macOS 14 runners.

## [1.1.0a1] - 2025-05-25
### Initial Release
- First public release of the GNUBG Python extension module.
- Includes:
    - Python bindings to GNUBG neural network evaluation (via `ctypes`)
    - Meson build system support for Windows and Linux
    - Exported key C functions from `gnubgmodule.cpp`
    - Example input vector evaluation pipeline
    - Early support for Position ID to input vector generation
