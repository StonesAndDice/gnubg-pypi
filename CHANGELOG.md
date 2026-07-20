## [Unreleased]
### Added
- Snake_case aliases for all C extension functions (e.g., `find_best_move`, `position_from_id`, `match_checksum`). Original camelCase names remain available for backward compatibility. Snake_case variants are the recommended idiomatic Python API.

## [1.1.0a1] - 2025-05-25
### Initial Release
- First public release of the GNUBG Python extension module.
- Includes:
    - Python bindings to GNUBG neural network evaluation (via `ctypes`)
    - Meson build system support for Windows and Linux
    - Exported key C functions from `gnubgmodule.cpp`
    - Example input vector evaluation pipeline
    - Early support for Position ID to input vector generation
