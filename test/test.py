"""
Xcache test suite
"""

import subprocess
import tempfile
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
def test_nonexistent(debug: bool, tmp_path: Path):
    """
    running something that does not exist should fail
    """
    args = ["xcache"]
    if debug:
        args += ["--debug"]
    args += [f"--dir={tmp_path}/database", "--", tmp_path / "nonexistent"]
    with pytest.raises(subprocess.CalledProcessError):
        subprocess.check_call(args)

    # even if we cached it, replay should return failure
    with pytest.raises(subprocess.CalledProcessError):
        subprocess.check_call(args)


@pytest.mark.parametrize("debug", (False, True))
@pytest.mark.parametrize("record", (False, True))
@pytest.mark.parametrize("replay", (False, True))
def test_nop(debug: bool, record: bool, replay: bool, tmp_path: Path):
    """
    can we handle a no-op program?
    """
    args = ["xcache"]
    if debug:
        args += ["--debug"]
    args += [f"--dir={tmp_path}/database"]
    if record:
        if replay:
            args += ["--read-write"]
        else:
            args += ["--write-only"]
    else:
        if replay:
            args += ["--read-only"]
        else:
            args += ["--disable"]
    args += ["--", "nop"]

    subprocess.check_call(args)


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
