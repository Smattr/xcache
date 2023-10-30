"""
Xcache test suite
"""

import tempfile
import subprocess
from pathlib import Path
import pytest


@pytest.mark.parametrize("debug", (False, True))
def test_no_dir(debug: bool):
    """
    hosting the cache in a directory within a directory that does not exist
    should fail
    """
    with tempfile.TemporaryDirectory() as tmp:
        nested = Path(tmp) / "foo/bar"
        args = ["xcache", f"--dir={nested}"]
        if debug:
            args += ["--debug"]
        with pytest.raises(subprocess.CalledProcessError):
            subprocess.check_call(args + ["--", "my-echo", "foo", "bar"])


@pytest.mark.parametrize("debug", (False, True))
def test_version(debug: bool):
    """
    check --version does something reasonable
    """
    args = ["xcache", "--version"]
    if debug:
        args += ["--debug"]
    output = subprocess.check_output(args, stderr=subprocess.STDOUT)
    assert output.strip() != "", "--version output nothing"
