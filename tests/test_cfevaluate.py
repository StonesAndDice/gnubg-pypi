"""
Regression tests for issue #23: cubeful evaluation. evaluate() returns the
cubeless equity (as upstream documents it); cfevaluate() (ported from
upstream's module) returns the cubeful equities and the cube decision.
"""
import unittest

import gnubg

# money game, centred cube, player 0 on roll
MONEY = gnubg.cubeinfo(1, -1, 0, 0, (0, 0), 0, 0)
CUBEFUL_0PLY = gnubg.evalcontext(1, 0, 1, 0, 0.0)
CUBELESS_0PLY = gnubg.evalcontext(0, 0, 1, 0, 0.0)

# a race the player on roll leads: gnubg's verdict is "Double, take"
RACE_DOUBLE_TAKE = "AQAASAAAAAAAAA"


class TestCfEvaluate(unittest.TestCase):

    def test_exists_with_snake_case_alias(self):
        self.assertTrue(callable(gnubg.cfevaluate))
        self.assertIs(gnubg.cf_evaluate, gnubg.cfevaluate)

    def test_shape(self):
        board = gnubg.positionfromid("4HPwATDgc/ABMA")
        result = gnubg.cfevaluate(board, MONEY, CUBEFUL_0PLY)
        self.assertEqual(len(result), 6)
        for x in result[:4]:
            self.assertIsInstance(x, float)
        self.assertIsInstance(result[4], int)
        self.assertIsInstance(result[5], str)

    def test_double_take(self):
        board = gnubg.positionfromid(RACE_DOUBLE_TAKE)
        optimal, no_double, double_take, double_pass, code, text = gnubg.cfevaluate(
            board, MONEY, CUBEFUL_0PLY
        )
        self.assertEqual(text, "Double, take")
        self.assertEqual(code, 0)  # DOUBLE_TAKE in gnubg's cubedecision enum
        self.assertEqual(double_pass, 1.0)
        self.assertGreater(double_take, no_double)
        self.assertAlmostEqual(optimal, double_take, places=6)

    def test_cubeful_differs_from_cubeless_evaluate(self):
        # evaluate() is cubeless whatever the context says (as upstream);
        # the cubeful number is cfevaluate's
        board = gnubg.positionfromid(RACE_DOUBLE_TAKE)
        cubeless = gnubg.evaluate(board, MONEY, CUBELESS_0PLY)[5]
        self.assertAlmostEqual(gnubg.evaluate(board, MONEY, CUBEFUL_0PLY)[5], cubeless, places=6)
        optimal = gnubg.cfevaluate(board, MONEY, CUBEFUL_0PLY)[0]
        self.assertGreater(abs(optimal - cubeless), 0.1)

    def test_defaults_without_a_game(self):
        # no cubeinfo / evalcontext: a money game with the cube centred and
        # the cube decision settings; must give a real decision
        board = gnubg.positionfromid(RACE_DOUBLE_TAKE)
        self.assertEqual(gnubg.cfevaluate(board)[5], "Double, take")

    def test_bad_arguments(self):
        with self.assertRaises(TypeError):
            gnubg.cfevaluate("not a board")
        with self.assertRaises(ValueError):
            gnubg.cfevaluate(gnubg.positionfromid(RACE_DOUBLE_TAKE), {"nope": 1})


if __name__ == "__main__":
    unittest.main()
