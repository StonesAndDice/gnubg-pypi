"""
Comprehensive tests for gnubg module functions.
These tests verify the Python bindings work correctly.
"""
import unittest
import gnubg


class TestGnubgCoreFunctions(unittest.TestCase):
    """Test core gnubg functions."""

    def test_board_function(self):
        """Test board() function."""
        result = gnubg.board()
        # Should return None if no game is active, or a tuple structure
        if result is not None:
            self.assertIsInstance(result, tuple)
            self.assertEqual(len(result), 2)


class TestPositionIDFunctions(unittest.TestCase):
    """Test position ID conversion functions."""

    def setUp(self):
        """Set up test board - standard backgammon starting position."""
        # Standard starting position for backgammon
        # Player 0: 2 on point 24, 5 on point 13, 3 on point 8, 5 on point 6
        # Player 1: 2 on point 1, 5 on point 12, 3 on point 17, 5 on point 19
        self.start_board = (
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        )
        
        # Known position ID for starting position
        self.start_position_id = "4HPwATDgc/ABMA"

    def test_positionid_exists(self):
        """Test that positionid function exists."""
        self.assertTrue(hasattr(gnubg, 'positionid'))
        self.assertTrue(callable(gnubg.positionid))

    def test_positionid_with_board(self):
        """Test positionid with a board argument."""
        pos_id = gnubg.positionid(self.start_board)
        self.assertIsInstance(pos_id, str)
        self.assertGreater(len(pos_id), 0)
        # Position IDs are typically 14 characters
        self.assertEqual(len(pos_id), 14)

    def test_positionfromid_exists(self):
        """Test that positionfromid function exists."""
        self.assertTrue(hasattr(gnubg, 'positionfromid'))
        self.assertTrue(callable(gnubg.positionfromid))

    def test_positionfromid_with_id(self):
        """Test positionfromid with a position ID."""
        board = gnubg.positionfromid(self.start_position_id)
        self.assertIsInstance(board, tuple)
        self.assertEqual(len(board), 2)
        self.assertEqual(len(board[0]), 25)
        self.assertEqual(len(board[1]), 25)

    def test_positionid_roundtrip(self):
        """Test that board -> ID -> board roundtrip works."""
        # Convert board to ID
        pos_id = gnubg.positionid(self.start_board)
        # Convert ID back to board
        board = gnubg.positionfromid(pos_id)
        # Convert back to ID again
        pos_id2 = gnubg.positionid(board)
        # IDs should match
        self.assertEqual(pos_id, pos_id2)

    def test_positionkey_exists(self):
        """Test that positionkey function exists."""
        self.assertTrue(hasattr(gnubg, 'positionkey'))
        self.assertTrue(callable(gnubg.positionkey))

    def test_positionkey_with_board(self):
        """Test positionkey with a board argument."""
        key = gnubg.positionkey(self.start_board)
        self.assertIsInstance(key, tuple)
        self.assertEqual(len(key), 10)
        # All elements should be integers
        for k in key:
            self.assertIsInstance(k, int)

    def test_positionfromkey_exists(self):
        """Test that positionfromkey function exists."""
        self.assertTrue(hasattr(gnubg, 'positionfromkey'))
        self.assertTrue(callable(gnubg.positionfromkey))

    def test_positionkey_roundtrip(self):
        """Test that board -> key -> board roundtrip works."""
        # Convert board to key
        key = gnubg.positionkey(self.start_board)
        # Convert key back to board
        board = gnubg.positionfromkey(key)
        # Convert back to key again
        key2 = gnubg.positionkey(board)
        # Keys should match
        self.assertEqual(key, key2)

    def test_invalid_position_id(self):
        """Test that invalid position ID raises ValueError."""
        with self.assertRaises(ValueError):
            gnubg.positionfromid("INVALID_ID")

    def test_invalid_board_format(self):
        """Test that invalid board format raises TypeError."""
        with self.assertRaises(TypeError):
            gnubg.positionid("not a board")
        
        with self.assertRaises(TypeError):
            gnubg.positionid((1, 2, 3))  # Wrong size

    def test_invalid_key_format(self):
        """Test that invalid key format raises TypeError."""
        with self.assertRaises(TypeError):
            gnubg.positionfromkey("not a key")
        
        with self.assertRaises(TypeError):
            gnubg.positionfromkey((1, 2, 3))  # Wrong size


class TestMETMatchIDGnubgID(unittest.TestCase):
    """Test met(), matchid(), and gnubgid() (from TODO)."""

    def setUp(self):
        """Standard board and match context for gnubgid/matchid."""
        self.start_board = (
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        )
        # 5-point match, cube 2, centered, player 0 to move, score 0-0, not Crawford
        self.cubeinfo = gnubg.cubeinfo(2, -1, 0, 5, (0, 0), 0)
        # Player 0 on roll, not resigned, not doubled, playing, dice (1, 2)
        self.posinfo = gnubg.posinfo(0, 0, 0, 1, (1, 2))

    def test_met_exists(self):
        """Test that met function exists."""
        self.assertTrue(hasattr(gnubg, 'met'))
        self.assertTrue(callable(gnubg.met))

    def test_met_default(self):
        """Test met() with no args returns list of 3 tables."""
        tables = gnubg.met()
        self.assertIsInstance(tables, list)
        self.assertEqual(len(tables), 3)
        self.assertIsInstance(tables[0], list)
        self.assertIsInstance(tables[1], list)
        self.assertIsInstance(tables[2], list)

    def test_met_with_maxscore(self):
        """Test met(maxscore) with explicit match length."""
        tables = gnubg.met(3)
        self.assertIsInstance(tables, list)
        self.assertEqual(len(tables), 3)

    def test_met_invalid_maxscore(self):
        """Test met() with invalid maxscore raises ValueError."""
        with self.assertRaises(ValueError):
            gnubg.met(-1)

    def test_matchid_exists(self):
        """Test that matchid function exists."""
        self.assertTrue(hasattr(gnubg, 'matchid'))
        self.assertTrue(callable(gnubg.matchid))

    def test_matchid_with_cubeinfo_posinfo(self):
        """Test matchid(cubeinfo, posinfo) returns match ID string."""
        mid = gnubg.matchid(self.cubeinfo, self.posinfo)
        self.assertIsInstance(mid, str)
        self.assertGreater(len(mid), 0)

    def test_matchid_cubeinfo_requires_posinfo(self):
        """Test matchid(cubeinfo) without posinfo raises TypeError."""
        with self.assertRaises(TypeError):
            gnubg.matchid(self.cubeinfo)

    def test_gnubgid_exists(self):
        """Test that gnubgid function exists."""
        self.assertTrue(hasattr(gnubg, 'gnubgid'))
        self.assertTrue(callable(gnubg.gnubgid))

    def test_gnubgid_with_board_cubeinfo_posinfo(self):
        """Test gnubgid(board, cubeinfo, posinfo) returns positionid:matchid."""
        gid = gnubg.gnubgid(self.start_board, self.cubeinfo, self.posinfo)
        self.assertIsInstance(gid, str)
        self.assertIn(':', gid)
        pos_part, match_part = gid.split(':', 1)
        self.assertEqual(pos_part, gnubg.positionid(self.start_board))
        self.assertEqual(match_part, gnubg.matchid(self.cubeinfo, self.posinfo))

    def test_gnubgid_requires_all_three_if_board_given(self):
        """Test gnubgid(board) without cubeinfo/posinfo raises TypeError."""
        with self.assertRaises(TypeError):
            gnubg.gnubgid(self.start_board)
        with self.assertRaises(TypeError):
            gnubg.gnubgid(self.start_board, self.cubeinfo)


class TestEvaluateFindbestmoveRolloutcontext(unittest.TestCase):
    """Test evaluate(), findbestmove(), and rolloutcontext() APIs."""

    def setUp(self):
        self.start_board = (
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        )
        self.cubeinfo = gnubg.cubeinfo(2, -1, 0, 0, (0, 0), 0)
        self.evalcontext = gnubg.evalcontext(0, 2, 1, 0, 0.0)

    def test_evaluate_exists(self):
        """Test that evaluate function exists."""
        self.assertTrue(hasattr(gnubg, 'evaluate'))
        self.assertTrue(callable(gnubg.evaluate))

    def test_evaluate_returns_six_floats(self):
        """Test evaluate(board, cubeinfo, evalcontext) returns tuple of 6 floats. Engine init at import."""
        out = gnubg.evaluate(self.start_board, self.cubeinfo, self.evalcontext)
        self.assertIsInstance(out, (list, tuple))
        self.assertEqual(len(out), 6)
        for i, v in enumerate(out):
            self.assertIsInstance(v, (int, float), f"evaluate[{i}] should be numeric")

    def test_findbestmove_exists(self):
        """Test that findbestmove function exists."""
        self.assertTrue(hasattr(gnubg, 'findbestmove'))
        self.assertTrue(callable(gnubg.findbestmove))

    def test_findbestmove_with_dice(self):
        """Test findbestmove(board, cubeinfo, evalcontext, dice) returns move tuple. Engine init at import."""
        move = gnubg.findbestmove(self.start_board, self.cubeinfo, self.evalcontext, (6, 1))
        self.assertIsInstance(move, (list, tuple))
        self.assertGreater(len(move), 0)

    def test_findbestmove_requires_dice_when_no_game(self):
        """Test findbestmove without dice and no game raises ValueError."""
        with self.assertRaises(ValueError):
            gnubg.findbestmove(self.start_board, self.cubeinfo, self.evalcontext)

    def test_rolloutcontext_exists(self):
        """Test that rolloutcontext function exists."""
        self.assertTrue(hasattr(gnubg, 'rolloutcontext'))
        self.assertTrue(callable(gnubg.rolloutcontext))

    def test_rolloutcontext_default(self):
        """Test rolloutcontext() with no args returns dict with expected keys."""
        rc = gnubg.rolloutcontext()
        self.assertIsInstance(rc, dict)
        self.assertIn('cubeful', rc)
        self.assertIn('trials', rc)
        self.assertIn('seed', rc)
        self.assertIn('std-limit', rc)
        self.assertIn('jsd-limit', rc)

    def test_rolloutcontext_custom(self):
        """Test rolloutcontext with custom args."""
        rc = gnubg.rolloutcontext(1, 1, 0, 1, 0, 0, 10, 1, 1, 0, 500, 42, 100, 0, 5, 100, 0.02, 2.0)
        self.assertEqual(rc['trials'], 500)
        self.assertEqual(rc['seed'], 42)
        self.assertAlmostEqual(rc['std-limit'], 0.02)
        self.assertAlmostEqual(rc['jsd-limit'], 2.0)


# Note: classify, cubeinfo, posinfo, evalcontext, parsemove, movetupletostring,
# luckrating, errorrating are covered in test_phase2_phase3.py.


if __name__ == '__main__':
    unittest.main()
