"""
Regression tests for issue #24: cubeful evaluations depended on what had
been evaluated before. gnubg's cube efficiency depends on the ply of the
whole evaluation, but the evaluation cache was keyed on the plies left at
each node, so the 1-ply nodes of a 2-ply search were later returned for
1-ply requests (and vice versa). Fixed in gnubg's EvalKey() (eval.c).
"""
import unittest

import gnubg

OPENING = gnubg.positionfromid("4HPwATDgc/ABMA")
MONEY = gnubg.cubeinfo(1, -1, 0, 0, (0, 0), 0)


def best_score(plies, cubeful=1):
    ec = gnubg.evalcontext(cubeful, plies, 1, 1, 0.0)
    return gnubg.findbestmoves(OPENING, MONEY, ec, (3, 1))[0]["score"]


class TestEvaluationCacheIsPlyAware(unittest.TestCase):

    def setUp(self):
        gnubg.command("clear cache")

    def tearDown(self):
        gnubg.command("clear cache")

    def test_one_ply_after_two_ply(self):
        fresh = best_score(1)
        gnubg.command("clear cache")
        best_score(2)
        self.assertAlmostEqual(best_score(1), fresh, places=6)

    def test_two_ply_after_one_ply(self):
        fresh = best_score(2)
        gnubg.command("clear cache")
        best_score(1)
        self.assertAlmostEqual(best_score(2), fresh, places=6)

    def test_one_and_two_ply_differ(self):
        # the opening 3-1 is one of the positions where they do; without
        # a difference the tests above would not test anything
        one = best_score(1)
        gnubg.command("clear cache")
        self.assertGreater(abs(best_score(2) - one), 1e-3)

    def test_cubeless_unaffected(self):
        fresh = best_score(1, cubeful=0)
        gnubg.command("clear cache")
        best_score(2, cubeful=0)
        self.assertAlmostEqual(best_score(1, cubeful=0), fresh, places=6)


if __name__ == "__main__":
    unittest.main()
