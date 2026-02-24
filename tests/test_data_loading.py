"""
Tests that the gnubg package loads its bundled data (weights, bearoff) correctly
without requiring GNUBG_DATA_DIR to be set. Data is resolved relative to the
installed package so it works "off the bat" after pip install.
"""
import os
import subprocess
import sys
import unittest


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


if __name__ == "__main__":
    unittest.main()
