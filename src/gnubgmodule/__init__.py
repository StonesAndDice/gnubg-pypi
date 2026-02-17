import os
from pathlib import Path
from ._gnubg import *

# Path to the weights file (gnubg.weights)
DATA_DIR = Path(__file__).parent / "data"

def initialize():
    """Initializes the engine and loads default weights."""
    weights_path = DATA_DIR / "gnubg.weights"
    if weights_path.exists():
        # Using the command interface we built in the C++ module
        command(f"set weights {weights_path}")
    else:
        print(f"Warning: Weights not found at {weights_path}")

# Auto-initialize on import
initialize()

__all__ = ["board", "positionid", "command"]