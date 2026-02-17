try:
    # Import from the uniquely named _version module
    from ._version import version, git_revision, short_version, full_version
    __version__ = version  # Assign the string to the standard attribute
except ImportError:
    version = "unknown"
    __version__ = "0.0.0"
    git_revision = "unknown"
    short_version = "0.0.0"