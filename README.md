<h1 align="center">
<img src="https://raw.githubusercontent.com/StonesAndDice/gnubg-pypi/refs/heads/main/img/banner.png">

GNUBG
</h1>

[![PyPI Downloads](https://img.shields.io/pypi/dm/gnubg.svg?label=PyPI%20downloads)](https://pypi.org/project/gnubg/)
[![Conda Downloads](https://img.shields.io/conda/dn/conda-forge/gnubg.svg?label=Conda%20downloads)](https://anaconda.org/conda-forge/gnubg/)
[![GitHub issues](https://img.shields.io/github/issues/StonesAndDice/gnubg-pypi.svg)](https://github.com/StonesAndDice/gnubg-pypi/issues)
[![License](https://img.shields.io/badge/license-GPL%20v2-blue.svg)](#license)
[![Stack Overflow](https://img.shields.io/badge/stackoverflow-Ask%20questions-blue.svg)](https://stackoverflow.com/questions/tagged/gnubg)

GNUBG Python bindings bring the powerful GNUBG backgammon engine to Python 3.

## Quick Start

`gnubg` is a native Python extension module that wraps the GNU Backgammon engine, so you can call 
the same position-analysis and cube-decision routines that power the GUI from any Python 3.8–3.13 script or application. It’s ideal for batch processing, data-science workflows, or building custom tools and UIs.

### Installation

```bash
pip install gnubg
````

### Getting Started

```python
import gnubg

# Load the engine (weights, bear-off tables, etc. are initialized)
gnubg.initnet()

# Convert a position key (20-char A–Z or 14-char Base64) to a board
board = gnubg.boardfromkey("4HPwATDgc/ABMA")

# Evaluate win/gammon/backgammon probabilities and equity at 4 plies
win, gamm, bg, equity = gnubg.probabilities(board, 4)

print(f"Win: {win:.3f}, Gammon: {gamm:.3f}, Backgammon: {bg:.3f}, Equity: {equity:.3f}")
```

That’s all you need to get up and running! For detailed API docs, advanced build options, and configuration, see the 
sections below or visit the full documentation on [ReadTheDocs](https://gnubg.readthedocs.io/en/latest/).

**Data files:** Weights, bearoff tables, and match-equity data are included in the package and loaded from the directory next to the compiled extension (`gnubg/data`). No environment variable is required for normal installs. To override the location (e.g. for a custom build), set `GNUBG_DATA_DIR` to the directory containing `gnubg.weights`.

* **ReadTheDocs** [https://gnubg.readthedocs.io/en/latest/](https://gnubg.readthedocs.io/en/latest/)
* **Website:** [https://www.gnu.org/software/gnubg/](https://www.gnu.org/software/gnubg/)
* **Original Documentation:** [http://www.gnubg.org/documentation/doku.php?id=gnu_backgammon_faq](http://www.gnubg.org/documentation/doku.php?id=gnu_backgammon_faq)
* **Mailing list:** [https://lists.gnu.org/mailman/listinfo/gnubg](https://lists.gnu.org/mailman/listinfo/gnubg)
* **Source code:** [https://github.com/StonesAndDice/gnubg-pypi](https://github.com/StonesAndDice/gnubg-pypi)
* **GNUBG Project:** [https://git.savannah.gnu.org/cgit/gnubg/gnubg-nn.git](https://git.savannah.gnu.org/cgit/gnubg/gnubg-nn.git)
* **Contributing:** [https://savannah.gnu.org/people/](https://savannah.gnu.org/people/)
* **Credits:** [https://git.savannah.gnu.org/cgit/gnubg.git/tree/credits.sh](https://git.savannah.gnu.org/cgit/gnubg.git/tree/credits.sh)

It provides:

* **Engine initialization & data loading** (neural-net weights, opening-book, bear-off tables)
* **Position classification** (`classify`) & **public-evaluation best move** (`pub_best_move`)
* **Board ↔ ID conversions** (`board_from_position_id`, `board_from_position_key`, `key_of_board`, `position_id`)
* **Dice utilities** (`roll`) & **cube utilities** (`best_move`, `pub_eval_score`)
* **Bear-off tools** (`bearoff_id_2_pos`, `bearoff_probabilities`)
* **Legal-move enumeration** (`moves`) & **probabilistic evaluation** (`probabilities`)
* **Monte-Carlo rollouts** (`rollout`, `cubeful_rollout`)
* **Equity lookup** (`equities.value(xAway, oAway)`)
* **Runtime engine tuning** via the `set` submodule

## 🧪 Platform Compatibility

| Python Version | Linux x86\_64<br>(glibc ≥ 2.17) | Linux i686<br>(glibc ≥ 2.12) | macOS universal2  | Windows x86\_64 |
|----------------| ------------------------------- | ---------------------------- |-------------------|-----------------|
| **3.14**       | ✅                               | ✅                            | ✅ (macOS ≥ 10.14) | ✅               |
| **3.13**       | ✅                               | ✅                            | ✅ (macOS ≥ 10.14) | ✅               |
| **3.12**       | ✅                               | ✅                            | ✅ (macOS ≥ 10.14) | ✅               |
| **3.11**       | ✅                               | ✅                            | ✅ (macOS ≥ 10.9)  | ✅               |
| **3.10**       | ✅                               | ✅                            | ✅ (macOS ≥ 10.9)  | ✅               |

### Notes:

* ✅ = Built and available
* ❌ = Not built
* macOS universal2 = Supports both ARM64 and x86-64 architectures

## Building on Windows (local)

To reproduce the Windows CI build locally and debug failures:

1. **Install MSYS2** (e.g. from [msys2.org](https://www.msys2.org/)), then in an **MSYS2 UCRT64** shell install dependencies:

   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-pkg-config \
     mingw-w64-ucrt-x86_64-glib2 mingw-w64-ucrt-x86_64-gmp \
     mingw-w64-ucrt-x86_64-readline mingw-w64-ucrt-x86_64-sqlite3 \
     mingw-w64-ucrt-x86_64-libpng mingw-w64-ucrt-x86_64-gettext
   ```

2. In **PowerShell** (from the repo root), run the build script (same steps as `.github/actions/build-wheel` on `windows-latest`):

   ```powershell
   .\scripts\build-windows-local.ps1
   ```

   This runs `meson setup` / `meson compile` only. To also build wheels with cibuildwheel:

   ```powershell
   .\scripts\build-windows-local.ps1 -Wheel
   ```

   If MSYS2 is not in `C:\msys64`, set `$env:MSYS2_ROOT` before running (e.g. `$env:MSYS2_ROOT = "D:\msys64"`).

## Testing

The **gnubg** package has basic unit testing. After installation, run:

```bash
python3 -m unittest discover -s gnubg.tests
```
## AI-Assisted Development

Parts of this project were developed with the assistance of generative AI tools.

Specifically, the following models were used:

- **GPT-4o** (OpenAI ChatGPT)
- **o4-mini-high** (OpenAI ChatGPT)

These models were used to assist with code generation, documentation drafting, and architectural guidance. All outputs were reviewed and curated by a human before inclusion.

> ⚠️ **Disclaimer:**  
> Although human-reviewed, some AI-generated content may contain mistakes, inaccuracies, or outdated practices. Contributors and users should critically assess all code, comments, and documentation. We welcome corrections and improvements via pull requests or issues.

## Code of Conduct

Please read the [Code of Conduct](https://github.com/StonesAndDice/gnubg-pypi/blob/main/CONDUCT.md) to learn how to interact positively.

## Contributing

Your expertise and enthusiasm are welcome! You can contribute by:

* Reviewing and testing pull requests
* Reporting and triaging issues
* Improving documentation, tutorials, and examples
* Enhancing engine parameters or submodules
* Maintaining website or branding assets
* Translating materials
* Assisting with outreach and onboarding
* Writing grant proposals or helping with fundraising

For more information, see our [Contributing Guide](https://github.com/StonesAndDice/gnubg-pypi/blob/main/CONTRIBUTING.md). If you’re unsure where to start, open an issue or join the discussion on our mailing list!

## Acknowledgments

This project builds upon the extensive work of the GNU Backgammon (GNUBG) community. Specifically the 
[pygnubg](https://git.savannah.gnu.org/cgit/gnubg/gnubg-nn.git/tree/py) program developed by Joseph Heled.

We express our gratitude to all contributors who have dedicated their time and expertise to the development of GNUBG.

- **AUTHORS.md**: A list of primary contributors to the **gnubg** project can be found [here](https://github.com/StonesAndDice/gnubg-pypi/blob/main/AUTHORS.md).
- **GNUBG credits.sh**: For a comprehensive list of contributors to the core GNUBG project, please refer to the [credits.sh](https://git.savannah.gnu.org/cgit/gnubg.git/tree/credits.sh) file.

We also thank the broader GNUBG community, including testers, translators, and mailing list participants, for their invaluable support.

