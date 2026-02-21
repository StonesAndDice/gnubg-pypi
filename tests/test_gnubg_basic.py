"""
Basic tests for gnubg module functionality.
Tests core functions that should work without an active game state.
"""
import unittest
import gnubg


class TestGnubgBasicFunctions(unittest.TestCase):
    """Test basic gnubg functions that don't require game state."""

    def test_module_imports(self):
        """Verify the module imports successfully."""
        self.assertIsNotNone(gnubg)
        self.assertTrue(hasattr(gnubg, '__version__'))

    def test_board_function_exists(self):
        """Verify board() function exists."""
        self.assertTrue(hasattr(gnubg, 'board'))
        self.assertTrue(callable(gnubg.board))

    def test_board_no_game(self):
        """Test board() returns None when no game is active."""
        result = gnubg.board()
        # Should return None if no game is active
        self.assertIsNone(result)


if __name__ == '__main__':
    unittest.main()
