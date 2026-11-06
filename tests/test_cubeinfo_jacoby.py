"""
Regression tests for issue #25: cubeinfo() always had the Jacoby rule on,
and its format string declared two more integer slots than it had output
pointers for.
"""
import json
import subprocess
import sys
import textwrap
import unittest

import gnubg


class TestCubeInfoArguments(unittest.TestCase):

    def test_existing_positional_forms_unchanged(self):
        ci = gnubg.cubeinfo(2, -1, 0, 5, (1, 2), 0)
        self.assertEqual((ci["cube"], ci["cubeowner"], ci["matchto"], ci["score"]),
                         (2, -1, 5, (1, 2)))
        self.assertEqual(gnubg.cubeinfo(1, -1, 0, 0, (0, 0), 0, 0)["bgv"], 0)

    def test_jacoby_and_beavers_positional(self):
        # the 9th and 10th slots used to write through missing pointers
        ci = gnubg.cubeinfo(1, -1, 0, 0, (0, 0), 0, 0, 0, 1)
        self.assertEqual((ci["jacoby"], ci["beavers"]), (0, 1))
        ci = gnubg.cubeinfo(1, -1, 0, 0, (0, 0), 0, 0, 1, 0)
        self.assertEqual((ci["jacoby"], ci["beavers"]), (1, 0))

    def test_keywords(self):
        ci = gnubg.cubeinfo(cube=2, matchto=0, jacoby=0, beavers=1)
        self.assertEqual((ci["cube"], ci["jacoby"], ci["beavers"]), (2, 0, 1))
        self.assertEqual(gnubg.cube_info(jacoby=0)["jacoby"], 0)

    def test_jacoby_off_values_gammons_with_the_cube_centred(self):
        # with the Jacoby rule a money game with a centred cube has no
        # gammons; without it, gammons count
        on = gnubg.cubeinfo(1, -1, 0, 0, (0, 0), 0, 0, 1)
        off = gnubg.cubeinfo(1, -1, 0, 0, (0, 0), 0, 0, 0)
        self.assertEqual(on["gammonprice"], ((0.0, 0.0), (0.0, 0.0)))
        self.assertGreater(off["gammonprice"][0][0], 0.0)

    def test_bad_arguments(self):
        with self.assertRaises(ValueError):
            gnubg.cubeinfo(bgv=99)
        with self.assertRaises(TypeError):
            gnubg.cubeinfo(nonsense=1)
        with self.assertRaises(TypeError):
            gnubg.cubeinfo(1, -1, 0, 0, (0, 0), 0, 0, 0, 0, 0)  # 11 values


class TestCubeInfoFollowsSetting(unittest.TestCase):

    def test_default_follows_set_jacoby(self):
        # "set jacoby" is global state: check it in a child interpreter
        code = textwrap.dedent(
            """
            import json, gnubg
            default = gnubg.cubeinfo()["jacoby"]
            gnubg.command("set jacoby off")
            off = gnubg.cubeinfo()["jacoby"]
            gnubg.command("set jacoby on")
            on = gnubg.cubeinfo()["jacoby"]
            print(json.dumps([default, off, on]))
            """
        )
        result = subprocess.run([sys.executable, "-c", code], capture_output=True,
                                text=True, timeout=120)
        self.assertEqual(result.returncode, 0, result.stderr)
        default, off, on = json.loads(result.stdout.strip().splitlines()[-1])
        self.assertEqual(default, 1)  # gnubg's default setting
        self.assertEqual(off, 0)
        self.assertEqual(on, 1)


if __name__ == "__main__":
    unittest.main()
