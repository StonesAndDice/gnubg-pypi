#!/usr/bin/env python3
"""Quick smoke test: import gnubg and run evaluate (no Flask)."""
import sys
print("step 0: import", flush=True)
import gnubg
print("step 1: import ok", flush=True)
b = gnubg.positionfromid("4HPwATDgc/ABMA")
print("step 2: positionfromid ok", flush=True)
ci = gnubg.cubeinfo(1, -1, 0, 0, (0, 0), 0)
print("step 3: cubeinfo ok", flush=True)
ec = gnubg.evalcontext(1, 2, 1, 0, 0.0)
print("step 4: evalcontext ok", flush=True)
out = gnubg.evaluate(b, ci, ec)
print("step 5: evaluate ok", out[:2], flush=True)
