#!/usr/bin/env python3
"""``auditwheel repair`` that also bundles GLib into the wheel.

The manylinux policies list ``libglib-2.0.so.0``, ``libgobject-2.0.so.0``
and ``libgthread-2.0.so.0`` as libraries every system provides (their
``lib_whitelist``, a PEP 513 legacy), so ``auditwheel repair`` leaves them
out of the wheel. Minimal systems such as the ``python:3.x-slim`` images do
not have GLib, and ``import gnubg`` failed there (issue #19).

This runs auditwheel with those three sonames removed from every policy's
whitelist, so they and their own dependencies (PCRE, libffi, ...) are
grafted into ``gnubg.libs/`` like any other library. Used by cibuildwheel
as the Linux repair command::

    python tools/repair_wheel_linux.py -w {dest_dir} {wheel}

It relies on auditwheel's policy table (``auditwheel.policy._POLICY_JSON_MAP``),
so the auditwheel version is pinned where it is installed; if that table
moves, this fails loudly instead of producing a wheel without GLib.
"""
import json
import sys
import tempfile
from pathlib import Path

import auditwheel.policy as policy
from auditwheel.main import main

BUNDLE = {"libglib-2.0.so.0", "libgobject-2.0.so.0", "libgthread-2.0.so.0"}


def without_glib(policy_file: Path, out_dir: Path) -> Path:
    policies = json.loads(policy_file.read_text())
    for p in policies:
        p["lib_whitelist"] = [lib for lib in p["lib_whitelist"] if lib not in BUNDLE]
    patched = out_dir / policy_file.name
    patched.write_text(json.dumps(policies))
    return patched


def run(argv):
    out_dir = Path(tempfile.mkdtemp(prefix="gnubg-auditwheel-"))
    for libc, policy_file in list(policy._POLICY_JSON_MAP.items()):
        policy._POLICY_JSON_MAP[libc] = without_glib(Path(policy_file), out_dir)
    sys.argv = ["auditwheel", "repair", *argv]
    return main()


if __name__ == "__main__":
    sys.exit(run(sys.argv[1:]))
