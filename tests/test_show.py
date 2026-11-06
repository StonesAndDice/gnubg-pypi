"""
Regression tests for issue #27: show() lost the first line of its output
(gnubg's outputl() dropped it while the capture buffer was still empty),
so show("board") returned "". Fixed in gnubg's output.c.
"""
import json
import subprocess
import sys
import textwrap
import unittest

import gnubg


class TestShow(unittest.TestCase):

    def test_version_keeps_its_first_line(self):
        text = gnubg.show("version")
        self.assertTrue(text.startswith("GNU Backgammon "), repr(text[:80]))
        self.assertIn("AUTHORS", text)

    def test_single_line_results(self):
        # one outputl() call each: used to come back empty
        self.assertEqual(gnubg.show("crawford"), "Crawford rule is not used in money sessions.")
        self.assertTrue(gnubg.show("cube"))

    def test_two_line_result_keeps_both(self):
        lines = gnubg.show("jacoby").splitlines()
        self.assertEqual(len(lines), 2, lines)
        self.assertIn("Jacoby", lines[0])

    def test_board(self):
        # needs a position set up (global state): child interpreter
        code = textwrap.dedent(
            """
            import json, gnubg
            gnubg.setgnubgid("4HPwATDgc/ABMA:MIGlAAAAAAAA")
            first = gnubg.show("board")
            second = gnubg.show("board")
            print(json.dumps([first, second]))
            """
        )
        result = subprocess.run([sys.executable, "-c", code], capture_output=True,
                                text=True, timeout=120)
        self.assertEqual(result.returncode, 0, result.stderr)
        first, second = json.loads(result.stdout.strip().splitlines()[-1])
        self.assertIn("Position ID: 4HPwATDgc/ABMA", first)
        self.assertIn("Match ID   : MIGlAAAAAAAA", first)
        self.assertIn("+13-14-15-16-17-18------19-20-21-22-23-24-+", first)
        self.assertEqual(first, second)


if __name__ == "__main__":
    unittest.main()
