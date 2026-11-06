"""
Regression tests for gnubg.hint() (issue #20: it segfaulted because the
result dict held references that had already been released).

hint() needs a game in progress, which is module-global state, so each test
runs in its own interpreter.
"""
import json
import subprocess
import sys
import textwrap
import unittest


def _run(code, timeout=120):
    """Run code in a fresh interpreter; return its last stdout line as JSON."""
    result = subprocess.run(
        [sys.executable, "-c", textwrap.dedent(code)],
        capture_output=True,
        text=True,
        timeout=timeout,
    )
    if result.returncode != 0:
        raise AssertionError(
            f"child exited with {result.returncode}\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
        )
    return json.loads(result.stdout.strip().splitlines()[-1])


class TestHint(unittest.TestCase):

    def test_hint_returns_ranked_chequer_plays(self):
        out = _run(
            """
            import gc, json, gnubg
            gnubg.command("set threads 1")
            gnubg.command("new session")
            gnubg.setgnubgid("4HPwATDgc/ABMA:MIGlAAAAAAAA")  # opening 3-1
            first = gnubg.hint(3)
            gc.collect()
            text = repr(first)          # used to crash here (freed objects)
            second = gnubg.hint(3)      # and again on a second call
            gc.collect()
            print(json.dumps({
                "hinttype": first["hinttype"],
                "gnubgid": first["gnubgid"],
                "moves": [h["move"] for h in first["hint"]],
                "equities": [h["equity"] for h in first["hint"]],
                "same": [h["move"] for h in second["hint"]] == [h["move"] for h in first["hint"]],
                "repr_ok": "8/5 6/5" in text,
            }))
            """
        )
        self.assertEqual(out["hinttype"], "chequer")
        self.assertTrue(out["gnubgid"].startswith("4HPwATDgc/ABMA:"))
        self.assertEqual(len(out["moves"]), 3)
        self.assertEqual(out["moves"][0], "8/5 6/5")
        self.assertEqual(out["equities"], sorted(out["equities"], reverse=True))
        self.assertTrue(out["same"])
        self.assertTrue(out["repr_ok"])

    def test_hint_without_game_raises(self):
        out = _run(
            """
            import json, gnubg
            try:
                gnubg.hint()
                print(json.dumps({"raised": False}))
            except RuntimeError as e:
                print(json.dumps({"raised": True, "msg": str(e)}))
            """
        )
        self.assertTrue(out["raised"])


if __name__ == "__main__":
    unittest.main()
