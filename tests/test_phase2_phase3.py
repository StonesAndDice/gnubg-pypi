"""
Tests for Phase 2 and Phase 3 functions.
Tests cubeinfo, posinfo, evalcontext, classify, ratings, and move functions.
"""
import unittest
import gnubg


class TestPhase2Functions(unittest.TestCase):
    """Test Phase 2: Evaluation setup functions."""

    def test_cubeinfo_exists(self):
        """Test that cubeinfo function exists."""
        self.assertTrue(hasattr(gnubg, 'cubeinfo'))
        self.assertTrue(callable(gnubg.cubeinfo))

    def test_cubeinfo_default(self):
        """Test cubeinfo with default parameters."""
        ci = gnubg.cubeinfo()
        self.assertIsInstance(ci, dict)
        # Check required keys
        self.assertIn('cube', ci)
        self.assertIn('cubeowner', ci)
        self.assertIn('move', ci)
        self.assertIn('matchto', ci)
        self.assertIn('score', ci)
        self.assertIn('crawford', ci)

    def test_cubeinfo_custom(self):
        """Test cubeinfo with custom parameters."""
        # Money game: cube=2, owner=-1 (centered), player 0 to move
        ci = gnubg.cubeinfo(2, -1, 0, 0, (0, 0), 0)
        self.assertEqual(ci['cube'], 2)
        self.assertEqual(ci['cubeowner'], -1)
        self.assertEqual(ci['move'], 0)
        self.assertEqual(ci['matchto'], 0)  # Money game

    def test_posinfo_exists(self):
        """Test that posinfo function exists."""
        self.assertTrue(hasattr(gnubg, 'posinfo'))
        self.assertTrue(callable(gnubg.posinfo))

    def test_posinfo_default(self):
        """Test posinfo with default parameters."""
        pi = gnubg.posinfo()
        self.assertIsInstance(pi, dict)
        # Check required keys
        self.assertIn('turn', pi)
        self.assertIn('resigned', pi)
        self.assertIn('doubled', pi)
        self.assertIn('gamestate', pi)
        self.assertIn('dice', pi)

    def test_posinfo_custom(self):
        """Test posinfo with custom parameters."""
        # Player 0's turn, no resignation, not doubled, playing, dice (3,4)
        pi = gnubg.posinfo(0, 0, 0, 1, (3, 4))
        self.assertEqual(pi['turn'], 0)
        self.assertEqual(pi['resigned'], 0)
        self.assertEqual(pi['doubled'], 0)
        self.assertEqual(pi['gamestate'], 1)
        self.assertEqual(pi['dice'], (3, 4))

    def test_evalcontext_exists(self):
        """Test that evalcontext function exists."""
        self.assertTrue(hasattr(gnubg, 'evalcontext'))
        self.assertTrue(callable(gnubg.evalcontext))

    def test_evalcontext_default(self):
        """Test evalcontext with default parameters."""
        ec = gnubg.evalcontext()
        self.assertIsInstance(ec, dict)
        # Check required keys
        self.assertIn('cubeful', ec)
        self.assertIn('plies', ec)
        self.assertIn('deterministic', ec)
        self.assertIn('prune', ec)
        self.assertIn('noise', ec)

    def test_evalcontext_custom(self):
        """Test evalcontext with custom parameters."""
        # 2-ply, deterministic, no cubeful
        ec = gnubg.evalcontext(0, 2, 1, 0, 0.0)
        self.assertEqual(ec['cubeful'], 0)
        self.assertEqual(ec['plies'], 2)
        self.assertEqual(ec['deterministic'], 1)
        self.assertEqual(ec['prune'], 0)
        self.assertEqual(ec['noise'], 0.0)

    def test_classify_exists(self):
        """Test that classify function exists."""
        self.assertTrue(hasattr(gnubg, 'classify'))
        self.assertTrue(callable(gnubg.classify))

    def test_classify_starting_position(self):
        """Test classify on starting position."""
        start_board = (
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        )
        classification = gnubg.classify(start_board)
        self.assertIsInstance(classification, int)
        # Starting position should be a contact position
        self.assertGreaterEqual(classification, 0)

    def test_luckrating_exists(self):
        """Test that luckrating function exists."""
        self.assertTrue(hasattr(gnubg, 'luckrating'))
        self.assertTrue(callable(gnubg.luckrating))

    def test_luckrating_values(self):
        """Test luckrating with various values."""
        # Very unlucky
        rating = gnubg.luckrating(-0.1)
        self.assertIsInstance(rating, int)
        self.assertGreaterEqual(rating, 0)
        self.assertLessEqual(rating, 5)
        
        # Neutral
        rating = gnubg.luckrating(0.0)
        self.assertIsInstance(rating, int)
        
        # Very lucky
        rating = gnubg.luckrating(0.1)
        self.assertIsInstance(rating, int)

    def test_errorrating_exists(self):
        """Test that errorrating function exists."""
        self.assertTrue(hasattr(gnubg, 'errorrating'))
        self.assertTrue(callable(gnubg.errorrating))

    def test_errorrating_values(self):
        """Test errorrating with various values."""
        # Supernatural (very low error)
        rating = gnubg.errorrating(0.001)
        self.assertIsInstance(rating, int)
        self.assertGreaterEqual(rating, 0)
        self.assertLessEqual(rating, 7)
        
        # Average
        rating = gnubg.errorrating(0.01)
        self.assertIsInstance(rating, int)
        
        # Awful (high error)
        rating = gnubg.errorrating(0.1)
        self.assertIsInstance(rating, int)


class TestPhase3Functions(unittest.TestCase):
    """Test Phase 3: Move functions."""

    def test_parsemove_exists(self):
        """Test that parsemove function exists."""
        self.assertTrue(hasattr(gnubg, 'parsemove'))
        self.assertTrue(callable(gnubg.parsemove))

    def test_parsemove_simple(self):
        """Test parsemove with simple move."""
        # Parse "8/5 6/5" - two checkers moving
        moves = gnubg.parsemove("8/5 6/5")
        self.assertIsInstance(moves, tuple)
        self.assertEqual(len(moves), 2)
        # Each move should be a tuple of (from, to)
        self.assertEqual(moves[0], (8, 5))
        self.assertEqual(moves[1], (6, 5))

    def test_parsemove_with_dashes(self):
        """Test parsemove with dashes instead of slashes."""
        # Parse "8-5 6-5" - should work the same
        moves = gnubg.parsemove("8-5 6-5")
        self.assertIsInstance(moves, tuple)
        self.assertEqual(len(moves), 2)
        self.assertEqual(moves[0], (8, 5))
        self.assertEqual(moves[1], (6, 5))

    def test_parsemove_bar(self):
        """Test parsemove with bar move."""
        # Parse "bar/22" - entering from bar
        moves = gnubg.parsemove("bar/22")
        self.assertIsInstance(moves, tuple)
        self.assertGreater(len(moves), 0)

    def test_parsemove_invalid(self):
        """Test parsemove with invalid move string."""
        with self.assertRaises(ValueError):
            gnubg.parsemove("invalid move")

    def test_movetupletostring_exists(self):
        """Test that movetupletostring function exists."""
        self.assertTrue(hasattr(gnubg, 'movetupletostring'))
        self.assertTrue(callable(gnubg.movetupletostring))

    def test_movetupletostring_simple(self):
        """Test movetupletostring with simple move."""
        # Create a simple board
        board = (
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        )
        # Move from 8 to 5, 6 to 5
        move = ((8, 5), (6, 5))
        move_str = gnubg.movetupletostring(move, board)
        self.assertIsInstance(move_str, str)
        self.assertGreater(len(move_str), 0)

    def test_movetupletostring_flat_format(self):
        """Test movetupletostring with flat tuple format."""
        board = (
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        )
        # Flat format: (8, 5, 6, 5)
        move = (8, 5, 6, 5)
        move_str = gnubg.movetupletostring(move, board)
        self.assertIsInstance(move_str, str)
        self.assertGreater(len(move_str), 0)

    def test_move_roundtrip(self):
        """Test that parsemove and movetupletostring are inverses."""
        board = (
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        )
        # Parse a move
        original_str = "8/5 6/5"
        moves = gnubg.parsemove(original_str)
        # Convert back to string
        move_str = gnubg.movetupletostring(moves, board)
        # Parse again
        moves2 = gnubg.parsemove(move_str)
        # Should get the same moves
        self.assertEqual(moves, moves2)


if __name__ == '__main__':
    unittest.main()
