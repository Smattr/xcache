"""
Xcache test suite
"""

import errno
import os
import re
import subprocess
from pathlib import Path

import pytest


def strace(args: list[Path | str], cwd: Path | None = None):
    """
    `strace` a process, expecting it to succeed
    """

    # we need to disable LSan, which does not work under `strace`
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = "detect_leaks=0"

    kwargs = {}
    if cwd is not None:
        kwargs["cwd"] = cwd

    subprocess.run(["strace", "-f", "--"] + args, env=env, check=True, **kwargs)


@pytest.mark.parametrize(
    "debug", (pytest.param(False, id="nodebug"), pytest.param(True, id="debug"))
)
@pytest.mark.parametrize(
    "record",
    (
        pytest.param(False, id="norecord"),
        pytest.param(True, id="record", marks=pytest.mark.xfail(strict=True)),
    ),
)
@pytest.mark.parametrize(
    "replay", (pytest.param(False, id="noreplay"), pytest.param(True, id="replay"))
)
def test_fork(debug: bool, record: bool, replay: bool, tmp_path: Path):
    """
    can we handle something that forks?
    """

    # First, `strace` the process we are about to test. If the test fails, the
    # `strace` output will show what syscalls it made which may aid debugging.
    # This is useful when, e.g., running on a new kernel where the dynamic
    # loader or libc makes unanticipated syscalls.
    strace(["forker"], tmp_path)
    (tmp_path / "foo").unlink()

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
    args += ["--", "forker"]

    p = subprocess.run(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=tmp_path,
        text=True,
        timeout=120,
        check=False,
    )
    print(f"output:\n{p.stdout}\n")
    p.check_returncode()

    if debug:
        if replay:
            assert "replay failed" in p.stdout, "replay succeeded with no trace"
        else:
            assert "replay failed" not in p.stdout, "replay incorrectly enabled"
            assert "replay succeeded" not in p.stdout, "replay incorrectly enabled"
        if record:
            assert "record succeeded" in p.stdout, "record of file write failed"
        else:
            assert "record failed" not in p.stdout, "record incorrectly enabled"
            assert "record succeeded" not in p.stdout, "record incorrectly enabled"

    assert (tmp_path / "foo").exists(), "file not written"
    assert (tmp_path / "foo").read_text() == "hello world", "file contents not written"

    # try it again to see if we can replay
    (tmp_path / "foo").unlink()
    p = subprocess.run(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=tmp_path,
        text=True,
        timeout=120,
        check=False,
    )
    print(f"output:\n{p.stdout}\n")
    p.check_returncode()

    if debug:
        if record and replay:
            assert "replay succeeded" in p.stdout, "replay of file write failed"
        elif replay:
            assert "replay failed" in p.stdout, "replay succeeded with no trace"
        else:
            assert "replay failed" not in p.stdout, "replay incorrectly enabled"
            assert "replay succeeded" not in p.stdout, "replay incorrectly enabled"
        if record and replay:
            assert (
                "record failed" not in p.stdout
            ), "record still attempted after replay"
            assert "record succeeded" not in p.stdout, "record after successful replay"
        elif record:
            assert "record succeeded" in p.stdout, "record of file write failed"
        else:
            assert "record failed" not in p.stdout, "record incorrectly enabled"
            assert "record succeeded" not in p.stdout, "record incorrectly enabled"

    assert (tmp_path / "foo").exists(), "file not written"
    assert (tmp_path / "foo").read_text() == "hello world", "file contents not written"


@pytest.mark.parametrize("debug", (False, True))
def test_no_dir(debug: bool, tmp_path: Path):
    """
    hosting the cache in a directory within a directory that does not exist
    should fail
    """
    nested = Path(tmp_path) / "foo/bar"
    args = ["xcache", f"--dir={nested}"]
    if debug:
        args += ["--debug"]
    with pytest.raises(subprocess.CalledProcessError):
        subprocess.run(args + ["--", "my-echo", "foo", "bar"], check=True)


@pytest.mark.parametrize("debug", (False, True))
def test_nonexistent(debug: bool, tmp_path: Path):
    """
    running something that does not exist should fail
    """
    args = ["xcache"]
    if debug:
        args += ["--debug"]
    args += [f"--dir={tmp_path}/database", "--", tmp_path / "nonexistent"]
    ret = subprocess.call(args)
    assert ret == 127, "unexpected return from non-existent exec"

    # even if we cached it, replay should return failure
    ret = subprocess.call(args)
    assert ret == 127, "unexpected return from non-existent exec"


def test_nonexistent2(tmp_path: Path):
    """running something that non-existent should report a readable error"""
    args = ["xcache", f"--dir={tmp_path}/database", "--", tmp_path / "nonexistent"]
    p = subprocess.run(
        args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False, text=True
    )
    assert (
        os.strerror(errno.ENOENT).lower() in p.stdout.lower()
    ), "incorrect/missing error message for missing program"


@pytest.mark.parametrize("debug", (False, True))
@pytest.mark.parametrize("record", (False, True))
@pytest.mark.parametrize("replay", (False, True))
def test_nop(debug: bool, record: bool, replay: bool, tmp_path: Path):
    """
    can we handle a no-op program?
    """
    # First, `strace` the process we are about to test. If the test fails, the
    # `strace` output will show what syscalls it made which may aid debugging.
    # This is useful when, e.g., running on a new kernel where the dynamic
    # loader or libc makes unanticipated syscalls.
    strace(["nop"])

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

    output = subprocess.check_output(
        args, stderr=subprocess.STDOUT, universal_newlines=True, timeout=120
    )

    if debug:
        if replay:
            assert "replay failed" in output, "replay succeeded with no trace"
        else:
            assert "replay failed" not in output, "replay incorrectly enabled"
            assert "replay succeeded" not in output, "replay incorrectly enabled"
        if record:
            assert "record succeeded" in output, "record of no-op failed"
        else:
            assert "record failed" not in output, "record incorrectly enabled"
            assert "record succeeded" not in output, "record incorrectly enabled"

    # try it again to see if we can replay
    output = subprocess.check_output(
        args, stderr=subprocess.STDOUT, universal_newlines=True, timeout=120
    )

    if debug:
        if record and replay:
            assert "replay succeeded" in output, "replay of no-op failed"
        elif replay:
            assert "replay failed" in output, "replay succeeded with no trace"
        else:
            assert "replay failed" not in output, "replay incorrectly enabled"
            assert "replay succeeded" not in output, "replay incorrectly enabled"
        if record and replay:
            assert "record failed" not in output, "record still attempted after replay"
            assert "record succeeded" not in output, "record after successful replay"
        elif record:
            assert "record succeeded" in output, "record of no-op failed"
        else:
            assert "record failed" not in output, "record incorrectly enabled"
            assert "record succeeded" not in output, "record incorrectly enabled"


@pytest.mark.parametrize("debug", (False, True))
@pytest.mark.parametrize("record", (False, True))
@pytest.mark.parametrize("replay", (False, True))
@pytest.mark.parametrize("stream", ("stdout", "stderr"))
def test_stdout(debug: bool, record: bool, replay: bool, stream: str, tmp_path: Path):
    """
    can we handle something that prints to a standard stream?
    """
    # First, `strace` the process we are about to test. If the test fails, the
    # `strace` output will show what syscalls it made which may aid debugging.
    # This is useful when, e.g., running on a new kernel where the dynamic
    # loader or libc makes unanticipated syscalls.
    strace([f"print-{stream}"])

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
    args += ["--", "print-stdout"]

    output = subprocess.check_output(
        args, stderr=subprocess.STDOUT, universal_newlines=True, timeout=120
    )

    print(f"output:\n{output}\n")

    if debug:
        if replay:
            assert "replay failed" in output, "replay succeeded with no trace"
        else:
            assert "replay failed" not in output, "replay incorrectly enabled"
            assert "replay succeeded" not in output, "replay incorrectly enabled"
        if record:
            assert "record succeeded" in output, f"record of {stream} user failed"
        else:
            assert "record failed" not in output, "record incorrectly enabled"
            assert "record succeeded" not in output, "record incorrectly enabled"

    assert re.search("hello\nworld", output), f"missing {stream}"

    # try it again to see if we can replay
    output = subprocess.check_output(
        args, stderr=subprocess.STDOUT, universal_newlines=True, timeout=120
    )

    print(f"output:\n{output}\n")

    if debug:
        if record and replay:
            assert "replay succeeded" in output, f"replay of {stream} user failed"
        elif replay:
            assert "replay failed" in output, "replay succeeded with no trace"
        else:
            assert "replay failed" not in output, "replay incorrectly enabled"
            assert "replay succeeded" not in output, "replay incorrectly enabled"
        if record and replay:
            assert "record failed" not in output, "record still attempted after replay"
            assert "record succeeded" not in output, "record after successful replay"
        elif record:
            assert "record succeeded" in output, f"record of {stream} user failed"
        else:
            assert "record failed" not in output, "record incorrectly enabled"
            assert "record succeeded" not in output, "record incorrectly enabled"

    assert re.search("hello\nworld", output), f"missing {stream}"


@pytest.mark.parametrize("debug", (False, True))
@pytest.mark.parametrize("record", (False, True))
@pytest.mark.parametrize("replay", (False, True))
def test_write_file(debug: bool, record: bool, replay: bool, tmp_path: Path):
    """
    can we handle something that writes a file?
    """
    # First, `strace` the process we are about to test. If the test fails, the
    # `strace` output will show what syscalls it made which may aid debugging.
    # This is useful when, e.g., running on a new kernel where the dynamic
    # loader or libc makes unanticipated syscalls.
    strace(["write-file"], tmp_path)
    (tmp_path / "foo").unlink()

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
    args += ["--", "write-file"]

    output = subprocess.check_output(
        args,
        stderr=subprocess.STDOUT,
        cwd=tmp_path,
        universal_newlines=True,
        timeout=120,
    )

    if debug:
        if replay:
            assert "replay failed" in output, "replay succeeded with no trace"
        else:
            assert "replay failed" not in output, "replay incorrectly enabled"
            assert "replay succeeded" not in output, "replay incorrectly enabled"
        if record:
            assert "record succeeded" in output, "record of file write failed"
        else:
            assert "record failed" not in output, "record incorrectly enabled"
            assert "record succeeded" not in output, "record incorrectly enabled"

    assert (tmp_path / "foo").exists(), "file not written"
    assert (tmp_path / "foo").read_text() == "hello world", "file contents not written"

    # try it again to see if we can replay
    (tmp_path / "foo").unlink()
    output = subprocess.check_output(
        args,
        stderr=subprocess.STDOUT,
        cwd=tmp_path,
        universal_newlines=True,
        timeout=120,
    )

    if debug:
        if record and replay:
            assert "replay succeeded" in output, "replay of file write failed"
        elif replay:
            assert "replay failed" in output, "replay succeeded with no trace"
        else:
            assert "replay failed" not in output, "replay incorrectly enabled"
            assert "replay succeeded" not in output, "replay incorrectly enabled"
        if record and replay:
            assert "record failed" not in output, "record still attempted after replay"
            assert "record succeeded" not in output, "record after successful replay"
        elif record:
            assert "record succeeded" in output, "record of file write failed"
        else:
            assert "record failed" not in output, "record incorrectly enabled"
            assert "record succeeded" not in output, "record incorrectly enabled"

    assert (tmp_path / "foo").exists(), "file not written"
    assert (tmp_path / "foo").read_text() == "hello world", "file contents not written"


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
