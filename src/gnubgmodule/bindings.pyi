from typing import Tuple, Optional, Any

# Type alias for a board: Tuple of two tuples, each containing 25 integers
BoardType = Tuple[Tuple[int, ...], Tuple[int, ...]]


def board() -> Optional[BoardType]:
    """
    Get the current board.

    Returns:
        A tuple of two tuples (one for each player), where each inner tuple
        contains 25 integers representing the checkers on points 1..24 and the bar.
        Returns None if no game is active.
    """
    ...