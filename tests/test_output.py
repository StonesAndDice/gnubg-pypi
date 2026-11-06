"""
Regression tests for issue #26: gnubg wrote straight to the process's
stdout/stderr (file descriptors 1 and 2), which corrupts programs that
speak a protocol on stdout. gnubg.set_output() routes it to Python.

Output routing is global state, and the point is what reaches fd 1 and
fd 2, so each test runs in a child interpreter and inspects the child's
real stdout/stderr.
"""
import json
import subprocess
import sys
import textwrap
import unittest

import gnubg


def _run(code):
    result = subprocess.run(
        [sys.executable, "-c", textwrap.dedent(code)],
        capture_output=True,
        text=True,
        timeout=120,
    )
    if result.returncode != 0:
        raise AssertionError(f"child failed:\n{result.stdout}\n{result.stderr}")
    return result


class TestSetOutput(unittest.TestCase):

    def test_nothing_reaches_fd_1_or_2_while_routed(self):
        result = _run(
            """
            import io, json, sys, gnubg
            out, err = io.StringIO(), []
            gnubg.set_output(out, err.append)    # a file-like and a callable
            gnubg.command("set jacoby off")      # normal output
            gnubg.command("frobnicate")          # an error message
            gnubg.command("set threads 1")
            sys.stdout.write(json.dumps({"out": out.getvalue(), "err": "".join(err)}) + "\\n")
            """
        )
        # the child's real stdout holds only its own JSON line
        lines = result.stdout.splitlines()
        self.assertEqual(len(lines), 1, result.stdout)
        captured = json.loads(lines[0])
        self.assertIn("Will not use the Jacoby rule", captured["out"])
        self.assertIn("number of threads has been set to 1", captured["out"])
        self.assertIn("frobnicate", captured["err"])
        self.assertNotIn("frobnicate", result.stderr)

    def test_default_and_reset_use_the_c_streams(self):
        result = _run(
            """
            import io, sys, gnubg
            gnubg.command("set jacoby off")     # default: fd 1
            sys.stdout.flush()
            gnubg.set_output(io.StringIO(), io.StringIO())
            gnubg.command("set jacoby on")      # routed away
            gnubg.set_output()                  # back to the C streams
            gnubg.command("set jacoby off")
            gnubg.command("frobnicate")
            """
        )
        self.assertEqual(result.stdout.count("Will not use the Jacoby rule"), 2)
        self.assertNotIn("Will use the Jacoby rule", result.stdout)
        self.assertIn("frobnicate", result.stderr)

    def test_redirect_to_stderr_keeps_stdout_clean(self):
        result = _run(
            """
            import sys, gnubg
            gnubg.set_output(sys.stderr, sys.stderr)
            gnubg.command("set jacoby off")
            print("PROTOCOL", flush=True)
            """
        )
        self.assertEqual(result.stdout, "PROTOCOL\n")
        self.assertIn("Will not use the Jacoby rule", result.stderr)

    def test_a_failing_target_does_not_break_the_command(self):
        result = _run(
            """
            import gnubg
            def broken(text):
                raise RuntimeError("target failed")
            gnubg.set_output(broken)
            gnubg.command("set jacoby off")     # must still run
            gnubg.set_output()
            print("DONE")
            """
        )
        self.assertIn("DONE", result.stdout)
        self.assertIn("target failed", result.stderr)  # reported as unraisable

    def test_rejects_targets_that_cannot_take_text(self):
        with self.assertRaises(TypeError):
            gnubg.set_output(42)
        with self.assertRaises(TypeError):
            gnubg.set_output(err=object())
        gnubg.set_output()  # and leaves the defaults in place


if __name__ == "__main__":
    unittest.main()
