"""
Regression tests for issue #22 (and #18): findbestmove()/findbestmoves()
cut a move short at its first bear-off step, because gnubg marks both
"off" and "end of move" with -1. A move is a flat tuple of 1-based
(from, to) pairs with 0 = off and 25 = bar.
"""
import unittest

import gnubg

# money game, centred cube, no Jacoby (0-ply, cubeless: fast and stable)
CUBEINFO = gnubg.cubeinfo(1, -1, 0, 0, (0, 0), 0, 0)
EVALCONTEXT = gnubg.evalcontext(0, 0, 1, 0, 0.0)
# opponent's 15 chequers on their 6-13 points: no contact with our home board
RACE_OPPONENT = (0, 0, 0, 0, 0, 5, 0, 3, 0, 2, 0, 0, 5) + (0,) * 12


def steps(move):
    """The (from, to) pairs of a flat move tuple, in a comparable order."""
    return sorted(zip(move[::2], move[1::2]))


class TestBearOffMoves(unittest.TestCase):

    def test_bear_off_after_a_move_inside(self):
        board = gnubg.positionfromid("AwAAMAAAAAAAAA")  # 2 chequers on the 2 point
        move = gnubg.findbestmove(board, CUBEINFO, EVALCONTEXT, (2, 1))
        self.assertEqual(len(move), 4, move)  # was (2,)
        self.assertEqual(steps(move), [(2, 0), (2, 1)])
        self.assertEqual(gnubg.movetupletostring(move, board), "2/1 2/off")

    def test_two_chequers_off(self):
        # 14 chequers on the 1-4 points in a race the mover is winning:
        # 5-3 takes two chequers off (issue #18 got (4,) back)
        board = (RACE_OPPONENT, (4, 2, 4, 4) + (0,) * 21)
        move = gnubg.findbestmove(board, CUBEINFO, EVALCONTEXT, (5, 3))
        self.assertEqual(steps(move), [(3, 0), (4, 0)])
        self.assertEqual(gnubg.movetupletostring(move, board), "4/off 3/off")

    def test_findbestmoves_keeps_whole_moves(self):
        board = (RACE_OPPONENT, (4, 2, 4, 4) + (0,) * 21)
        moves = gnubg.findbestmoves(board, CUBEINFO, EVALCONTEXT, (5, 3))
        self.assertEqual(steps(moves[0]["move"]), [(3, 0), (4, 0)])
        self.assertGreater(len(moves), 1)
        for m in moves:
            self.assertEqual(len(m["move"]), 4, m)  # two dice, two steps
            for frm, to in steps(m["move"]):
                self.assertTrue(1 <= frm <= 25, m)
                self.assertTrue(0 <= to < frm, m)

    def test_doubles_bearing_off_four_chequers(self):
        board = (RACE_OPPONENT, (0, 4) + (0,) * 23)  # four chequers on the 2 point
        move = gnubg.findbestmove(board, CUBEINFO, EVALCONTEXT, (2, 2))
        self.assertEqual(move, (2, 0, 2, 0, 2, 0, 2, 0))

    def test_flat_move_tuple_needs_whole_pairs(self):
        board = gnubg.positionfromid("AwAAMAAAAAAAAA")
        with self.assertRaises(TypeError):
            gnubg.movetupletostring((2,), board)
        self.assertEqual(gnubg.movetupletostring(((2, 1), (2, 0)), board), "2/1 2/off")
        self.assertEqual(gnubg.movetupletostring((2, 1, 2, 0), board), "2/1 2/off")


if __name__ == "__main__":
    unittest.main()
