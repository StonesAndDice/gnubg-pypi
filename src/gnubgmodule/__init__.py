try:
    # Import from the uniquely named _version module
    from ._version import version, git_revision, short_version, full_version
    __version__ = version
except ImportError:
    version = "unknown"
    __version__ = "0.0.0"
    git_revision = "unknown"
    short_version = "0.0.0"

# Import all functions from the C++ extension module
# REMOVED try/except to reveal build/link errors
from ._gnubg import *