"""
Tests that the gnubg package loads its bundled data (weights, bearoff) correctly
without requiring GNUBG_DATA_DIR to be set. Data is resolved relative to the
installed package so it works "off the bat" after pip install.
"""
import os
import subprocess
import sys
import unittest

try:
    import gnubg
    _gnubg_available = True
except ImportError:
    _gnubg_available = False


def _gnubg_data_dir():
    """Return the path to the gnubg package data directory (same layout as in the wheel)."""
    import gnubg
    # Package root is the dir containing __init__.py; data is in data/ next to it
    pkg_root = os.path.dirname(os.path.abspath(gnubg.__file__))
    return os.path.join(pkg_root, "data")


class TestDataLoadingWithoutEnv(unittest.TestCase):
    """Import and data loading work without GNUBG_DATA_DIR set."""

    def test_import_succeeds_without_gnubg_data_dir(self):
        """Import gnubg in a clean env (no GNUBG_DATA_DIR) must succeed."""
        env = os.environ.copy()
        env.pop("GNUBG_DATA_DIR", None)
        result = subprocess.run(
            [sys.executable, "-c", "import gnubg; print('ok')"],
            env=env,
            capture_output=True,
            text=True,
            timeout=30,
        )
        self.assertEqual(result.returncode, 0, msg=result.stderr or result.stdout)
        self.assertIn("ok", result.stdout)

    def test_evaluate_uses_loaded_weights(self):
        """After import, evaluate() works (weights and bearoff loaded from package data)."""
        import gnubg

        board = (
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        )
        cubeinfo = gnubg.cubeinfo(2, -1, 0, 0, (0, 0), 0)
        ec = gnubg.evalcontext(0, 2, 1, 0, 0.0)
        out = gnubg.evaluate(board, cubeinfo, ec)
        self.assertIsInstance(out, (list, tuple), "evaluate should return sequence of 6 floats")
        self.assertEqual(len(out), 6)
        for i, v in enumerate(out):
            self.assertIsInstance(v, (int, float), f"evaluate[{i}] should be numeric")

    def test_findbestmove_uses_loaded_weights(self):
        """After import, findbestmove() works (engine fully initialized from package data)."""
        import gnubg

        board = (
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        )
        cubeinfo = gnubg.cubeinfo(2, -1, 0, 0, (0, 0), 0)
        ec = gnubg.evalcontext(0, 2, 1, 0, 0.0)
        move = gnubg.findbestmove(board, cubeinfo, ec, (6, 1))
        self.assertIsInstance(move, (list, tuple), "findbestmove should return a move tuple")
        self.assertGreater(len(move), 0)


@unittest.skipUnless(_gnubg_available, "gnubg not installed (install wheel to run wheel data asset tests)")
class TestWheelDataAssets(unittest.TestCase):
    """
    Assert that the data assets required by the package are present in the
    installed package (e.g. wheel). If the wheel is built without these files,
    these tests fail.
    """

    # Required data files installed by meson (install_data + custom_target gnubg.wd)
    REQUIRED_DATA_FILES = [
        "gnubg.weights",
        "gnubg_ts0.bd",
        "gnubg_os0.bd",
        "gnubg_os.db",
    ]

    # At least one of these must exist (gnubg.wd is built at wheel build time)
    NEURALNET_WEIGHTS = ("gnubg.wd", "gnubg.weights")

    # Subdir installed by install_subdir (e.g. met/)
    REQUIRED_DATA_SUBDIRS = [
        os.path.join("met", "Kazaross-XG2.xml"),
    ]

    def test_data_dir_exists(self):
        """Package data directory exists next to the installed package."""
        data_dir = _gnubg_data_dir()
        self.assertTrue(
            os.path.isdir(data_dir),
            f"Package data directory should exist: {data_dir}",
        )

    def test_required_data_files_in_wheel(self):
        """All required data files are present in the installed package (wheel)."""
        data_dir = _gnubg_data_dir()
        self.assertTrue(os.path.isdir(data_dir), f"Data dir missing: {data_dir}")
        missing = []
        for name in self.REQUIRED_DATA_FILES:
            path = os.path.join(data_dir, name)
            if not os.path.isfile(path):
                missing.append(name)
        self.assertEqual(
            missing,
            [],
            f"Required data files missing from package: {missing} (data_dir={data_dir})",
        )

    def test_neuralnet_weights_in_wheel(self):
        """Either gnubg.wd or gnubg.weights is present (neural net can load)."""
        data_dir = _gnubg_data_dir()
        self.assertTrue(os.path.isdir(data_dir), f"Data dir missing: {data_dir}")
        found = [
            name
            for name in self.NEURALNET_WEIGHTS
            if os.path.isfile(os.path.join(data_dir, name))
        ]
        self.assertTrue(
            len(found) >= 1,
            f"At least one of {self.NEURALNET_WEIGHTS} must exist in {data_dir}",
        )

    def test_required_data_subdirs_in_wheel(self):
        """Required data subdir files (e.g. met/) are present in the wheel."""
        data_dir = _gnubg_data_dir()
        self.assertTrue(os.path.isdir(data_dir), f"Data dir missing: {data_dir}")
        missing = []
        for rel_path in self.REQUIRED_DATA_SUBDIRS:
            path = os.path.join(data_dir, rel_path)
            if not os.path.isfile(path):
                missing.append(rel_path)
        self.assertEqual(
            missing,
            [],
            f"Required data subdir files missing: {missing} (data_dir={data_dir})",
        )


if __name__ == "__main__":
    unittest.main()
