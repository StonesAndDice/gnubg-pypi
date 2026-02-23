Contributing Guide for gnubg (gnubg-pypi)
=========================================

Thank you for considering contributing to **gnubg**, the Python bindings
for the GNUBG backgammon engine (repository: gnubg-pypi).

This project integrates with the upstream GNUBG C/C++ engine, maintained
separately by the GNU Backgammon team.

You can contribute to either:

1. **The GNUBG Project (C/C++ Core)**
2. **The gnubg-pypi Project (Python Bindings)**

Please read the relevant section(s) below based on your area of interest.

-----------------------------------------
1. Contributing to the GNUBG Core Project
-----------------------------------------

The upstream GNUBG engine is maintained at:
- Source: https://git.savannah.gnu.org/cgit/gnubg/gnubg.git
- Contributor registration: https://savannah.gnu.org/people/

If you'd like to contribute to core engine features, neural network logic,
or C/C++ code:
- Familiarize yourself with the existing codebase, especially files like
  `analyze.cc`, `danalyze.cc`, and `bearoff.c`.
- Discuss proposed changes via the mailing list:  
  https://lists.gnu.org/mailman/listinfo/gnubg
- Follow GNU project coding and license guidelines (GPL v2).

---------------------------------------------
2. Contributing to gnubg-pypi (Python API)
------------------------------------------

The **gnubg** package (gnubg-pypi repo) exposes core GNUBG functionality to Python.

GitHub Repo: https://github.com/StonesAndDice/gnubg-pypi

You can contribute by:

- **Reporting Bugs:**  
  File an issue at https://github.com/StonesAndDice/gnubg-pypi/issues

- **Writing Code:**
    - Fork the repository and create a feature branch.
    - Write clear, tested, and documented code.
    - Follow PEP8 conventions.
    - Submit a pull request with a clear explanation of your change.

- **Testing & Validation:**
    - Run: `python3 -m unittest discover -s gnubg.tests`
    - Add new unit tests if you're introducing new features.

- **Improving Documentation:**
    - Propose updates to the README or inline docstrings.
    - Suggest usage examples or tutorials.

- **Helping with Outreach or Translations:**  
  We welcome translated materials, packaging help, and educational outreach.

--------------------------------
Building from source (WSL / editable install)
--------------------------------

The build uses **Meson** (>= 1.7.0) and **Ninja** (>= 1.8.2). Both must be on your
``PATH`` when running ``pip install`` and when Python later runs the editable
rebuild (e.g. on first ``import gnubg``).

**1. Install Meson and Ninja (recommended: system packages in WSL)**

- Ubuntu/Debian: ``sudo apt update && sudo apt install meson ninja-build``
- Ensure versions: ``meson --version`` (e.g. 1.8.x or 1.10.x), ``ninja --version`` (e.g. 1.10+).

**2. If you see "incompatible with the current version" or "Could not detect Ninja"**

- Remove the build tree so Meson reconfigures from scratch:
  ``rm -rf build/``
- Put **both** ``meson`` and ``ninja`` on ``PATH`` (e.g. install with ``apt`` above).
  Passing only ``NINJA=...`` may not be enough for the editable loader’s rebuild.

**3. Optional: use a virtualenv so Meson version is consistent**

- ``python3 -m venv .venv && source .venv/bin/activate``
- ``pip install meson ninja`` (or rely on system ``meson``/``ninja`` if on ``PATH``).
- ``pip install -e . --no-build-isolation``

**4. WSL-only: if ``ninja`` is not found (e.g. PATH points at Windows)**

- **Option A:** ``sudo apt install ninja-build`` (recommended).
- **Option B:** Download [Ninja for Linux](https://github.com/ninja-build/ninja/releases),
  put the binary in e.g. ``.build-tools/ninja``, and add that directory to ``PATH``
  before running pip: ``export PATH="$PWD/.build-tools:$PATH"`` then ``pip install -e .``.

--------------------------------
Licensing and Code Ownership
--------------------------------

All contributions must be compatible with GPL v2.

By submitting a pull request, you affirm that your contributions are your own
original work, or that you have permission to contribute them under the
project's license.

---------------------
Need Help?
---------------------

Please reach out via:
- GitHub Issues: https://github.com/StonesAndDice/gnubg-pypi/issues
- Mailing List: https://lists.gnu.org/mailman/listinfo/gnubg

Thank you for contributing to the GNUBG community!
