"""
Regression tests for issue #21: the module started with no calculation
thread, so anything gnubg queues on its thread pool (the computer's move
in "new session", "hint", ...) waited forever.

Each test runs in a child interpreter with a timeout, because a failure
is a hang and session state is module-global.
"""
import subprocess
import sys
import textwrap
import unittest


def _run(code, timeout):
    try:
        return subprocess.run(
            [sys.executable, "-c", textwrap.dedent(code)],
            capture_output=True,
            text=True,
            timeout=timeout,
            stdin=subprocess.DEVNULL,
        )
    except subprocess.TimeoutExpired as e:
        raise AssertionError(f"gnubg hung (no answer in {timeout}s): {e.stdout!r}")


class TestCalculationThreads(unittest.TestCase):

    def test_a_thread_is_running_after_import(self):
        result = _run(
            """
            import gnubg
            gnubg.command("show threads")
            gnubg.setgnubgid("4HPwATDgc/ABMA:MIGlAAAAAAAA")
            gnubg.command("hint 1")       # queued on the thread pool
            print("DONE", flush=True)
            """,
            timeout=120,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("DONE", result.stdout)
        self.assertNotIn("0 calculation threads", result.stdout)
        self.assertIn("8/5 6/5", result.stdout)

    def test_new_session_returns_when_the_computer_moves_first(self):
        # gnubg moves first in about half of the sessions; eight in a row
        # all but guarantee at least one computer move (ComputerTurn).
        result = _run(
            """
            import gnubg
            for _ in range(8):
                gnubg.command("new session")
            print("DONE", flush=True)
            """,
            timeout=180,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("DONE", result.stdout)


if __name__ == "__main__":
    unittest.main()
