# gnubg Python package – examples

This directory contains example projects showing how to use the [gnubg](https://github.com/StonesAndDice/gnubg-pypi) Python package in real applications. It is installed with the package (e.g. under `site-packages/gnubg/examples/`).

## Examples

| Example | Description |
|--------|-------------|
| [rest-api](rest-api/) | RESTful API for best move and cube/position evaluation |

## Requirements

- Python 3.10+
- The `gnubg` package must be installed (from PyPI or built from this repository).

## Running examples

After installing gnubg, the examples are available in the package directory. To find them:

```python
import gnubg
import os
examples_dir = os.path.join(os.path.dirname(gnubg.__file__), 'examples')
```

Or from the command line (example for rest-api):

```bash
cd "$(python -c "import gnubg, os; print(os.path.join(os.path.dirname(gnubg.__file__), 'examples', 'rest-api'))")"
pip install -r requirements.txt
flask --app app run --no-reload
```

## Contributing

See the main [CONTRIBUTING.md](https://github.com/StonesAndDice/gnubg-pypi/blob/main/CONTRIBUTING.md). New example ideas are tracked in [issue #6](https://github.com/StonesAndDice/gnubg-pypi/issues/6).
