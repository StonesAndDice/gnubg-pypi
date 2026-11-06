#!/usr/bin/env python3
"""Check that an installed gnubg wheel works on a system without GLib.

CI runs this in clean ``python:3.x-slim`` / ``python:3.x-alpine`` containers
after installing the freshly built wheel (issue #19: the manylinux wheels
needed the system's GLib). It first makes sure GLib really is absent, so
that a pass means the wheel brought its own copy.
"""
import ctypes.util
import sys

for lib in ("glib-2.0", "gobject-2.0"):
    found = ctypes.util.find_library(lib)
    if found:
        sys.exit(f"{lib} is installed on this system ({found}): the check would prove nothing")

import gnubg  # noqa: E402  (after the check above, on purpose)

board = gnubg.positionfromid("4HPwATDgc/ABMA")
move = gnubg.findbestmove(board, gnubg.cubeinfo(), gnubg.evalcontext(0, 0, 1, 0, 0.0), (3, 1))
print(f"gnubg {gnubg.__version__} on Python {sys.version.split()[0]}: opening 3-1 -> {move}")
if move != (8, 5, 6, 5):
    sys.exit(f"unexpected best move {move}")
