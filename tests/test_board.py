import unittest
import sys

# We wrap the import in a try-block to handle cases where
# the package isn't built yet, but strict TDD implies the test exists first.
try:
    import gnubg
except ImportError:
    gnubg = None


class TestGnubgBoard(unittest.TestCase):

    def setUp(self):
        if gnubg is None:
            self.fail("Could not import the 'gnubg' module. Ensure the extension is built and installed.")

    def test_board_signature(self):
        """
        Verify that board() takes no arguments.
        """
        with self.assertRaises(TypeError):
            gnubg.board("unexpected argument")

    def test_board_return_structure(self):
        """
        Verify that board() returns either None (if no game is active)
        or a specific tuple structure representing the board.
        """
        board = gnubg.board()

        # If GAME_NONE (no game active), gnubg returns None.
        if board is None:
            print("\n[Info] No active game state detected. gnubg.board() returned None.")
            return

        # If a game is active, verify the data structure
        self.assertIsInstance(board, tuple, "Board must return a tuple")
        self.assertEqual(len(board), 2, "Board tuple must have exactly 2 elements (one per player)")

        player0, player1 = board

        # Check Player 0 (X)
        self.assertIsInstance(player0, tuple, "Player 0 board must be a tuple")
        self.assertEqual(len(player0), 25, "Player 0 board must have 25 points (including bar)")
        for point in player0:
            self.assertIsInstance(point, int, "Checkers on points must be integers")
            self.assertGreaterEqual(point, 0, "Checker count cannot be negative")

        # Check Player 1 (O)
        self.assertIsInstance(player1, tuple, "Player 1 board must be a tuple")
        self.assertEqual(len(player1), 25, "Player 1 board must have 25 points (including bar)")
        for point in player1:
            self.assertIsInstance(point, int, "Checkers on points must be integers")
            self.assertGreaterEqual(point, 0, "Checker count cannot be negative")

    def test_board_values_sanity(self):
        """
        If a board exists, ensure the total number of checkers is reasonable
        (e.g., standard backgammon has 15 checkers per side).
        """
        board = gnubg.board()
        if board is not None:
            p0_count = sum(board[0])
            p1_count = sum(board[1])

            # Note: This might vary if testing Nackgammon or Hypergammon,
            # but usually it's non-zero if a game is running.
            self.assertTrue(p0_count <= 15, "Standard board usually has max 15 checkers (unless custom setup)")
            self.assertTrue(p1_count <= 15, "Standard board usually has max 15 checkers (unless custom setup)")


if __name__ == '__main__':
    unittest.main()
