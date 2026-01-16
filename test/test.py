"""
Xcache test suite
"""

import errno
import math
import os
import re
import shlex
import shutil
import stat
import subprocess
import sys
from pathlib import Path

import pytest

PathLike = Path | str
"""a file system path"""


def run(
    args: list[PathLike],
    *,
    cwd: None | PathLike = None,
    env: None | dict[str, str] = None,
) -> tuple[int, str, str]:
    """
    run a command, echoing its output like Bash’s `set +x`

    Args:
        args: Command line arguments
        cwd: Current working directory
        env: Environment for new process

    Returns
        (Exit status, Stdout, Stderr)
    """

    sys.stdout.flush()
    sys.stderr.flush()

    prefix1 = "" if cwd is None else f"cd {shlex.quote(str(cwd))} && "
    prefix2 = "" if env is None else "env … "
    print(f"+ {prefix1}{prefix2}{shlex.join(str(a) for a in args)}", flush=True)
    p = subprocess.run(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=cwd,
        check=False,
        text=True,
        env=env,
    )

    sys.stdout.write(p.stdout)
    sys.stdout.flush()
    sys.stderr.write(p.stderr)
    sys.stderr.flush()

    return p.returncode, p.stdout, p.stderr


def sandbox(
    args: list[PathLike], *, box: PathLike, env: None | dict[str, str] = None
) -> tuple[int, str, str]:
    """
    run a command within a sandbox

    Strictly speaking, the current working directory and the sandbox boundary do not
    need to be tied together. However, I have found with `bwrap` that the current
    directory seems available read/write even if it is not within any read/write bind
    mount. So fewer surprises occur if we constrain these to be the same.

    Args:
        args: Command line arguments
        box: Sandbox boundary and current working directory
        env: Environment for the sandboxed process

    Returns
        (Exit status, Stdout, Stderr)
    """

    # find sandboxer
    wrapper = shutil.which("bwrap")
    assert wrapper is not None, "Bubblewrap not found"

    # construct a sandbox invocation, read-only mounting everything
    wrap = [wrapper, "--die-with-parent", "--ro-bind", "/", "/"]

    # include a usable stub /dev
    wrap += ["--dev", "/dev"]

    # make /proc usable
    wrap += ["--proc", "/proc"]

    # allow writing to the sandbox itself
    wrap += ["--bind", box, box, "--unshare-all", "--"]

    argv = wrap + args
    return run(argv, cwd=box, env=env)


@pytest.mark.parametrize("absolutify", (False, True))
@pytest.mark.parametrize("box", ("a", "b"))
@pytest.mark.parametrize("arg1", ("foo", "../a/foo", "../b/foo"))
def test_sandbox(absolutify: bool, box: str, arg1: str, tmp_path: Path):
    """
    does the sandbox functionality behave like we expect?

    Args:
        absolutify: Use an absolute version of the path in `arg1` when trying to modify
            it?
        box: Directory relative to `tmp_path` to sandbox ourselves in
        arg1: A path relative to `tmp_path` to try to `touch`
        tmp_path: Temporary directory supplied by Pytest
    """

    (tmp_path / "a").mkdir()
    (tmp_path / "b").mkdir()

    abs_target = (tmp_path / box / arg1).resolve()
    args = ["touch", abs_target if absolutify else arg1]
    ret, _, _ = sandbox(args, box=tmp_path / box)

    if not abs_target.is_relative_to(tmp_path / box):
        assert ret != 0, "can write outside sandbox"
        assert not abs_target.exists(), "write occurred outside sandbox"
        return

    assert ret == 0, "cannot write inside sandbox"
    assert abs_target.exists(), "write failed outside sandbox"


def strace(args: list[Path | str], cwd: Path):
    """
    `strace` a process, expecting it to succeed
    """

    # we need to disable LSan, which does not work under `strace`
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = "detect_leaks=0"

    ret, _, _ = sandbox(["strace", "-f", "--"] + args, box=cwd, env=env)
    assert ret == 0


@pytest.mark.parametrize(
    "debug", (pytest.param(False, id="nodebug"), pytest.param(True, id="debug"))
)
@pytest.mark.parametrize(
    "record",
    (
        pytest.param(False, id="norecord"),
        pytest.param(True, id="record"),
    ),
)
@pytest.mark.parametrize(
    "replay", (pytest.param(False, id="noreplay"), pytest.param(True, id="replay"))
)
@pytest.mark.parametrize("forker", ("forker", "forker-fork", "forker-vfork"))
def test_fork(debug: bool, record: bool, replay: bool, forker: str, tmp_path: Path):
    """
    can we handle something that forks?
    """

    # First, `strace` the process we are about to test. If the test fails, the
    # `strace` output will show what syscalls it made which may aid debugging.
    # This is useful when, e.g., running on a new kernel where the dynamic
    # loader or libc makes unanticipated syscalls.
    strace([f"xcache-test-{forker}"], tmp_path)
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
    args += ["--", f"xcache-test-{forker}"]

    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    if debug:
        if replay:
            assert "replay failed" in stderr, "replay succeeded with no trace"
        else:
            assert "replay failed" not in stderr, "replay incorrectly enabled"
            assert "replay succeeded" not in stderr, "replay incorrectly enabled"
        if record:
            assert "record succeeded" in stderr, "record of file write failed"
        else:
            assert "record failed" not in stderr, "record incorrectly enabled"
            assert "record succeeded" not in stderr, "record incorrectly enabled"

    assert (tmp_path / "foo").exists(), "file not written"
    assert (tmp_path / "foo").read_text() == "hello world", "file contents not written"

    # try it again to see if we can replay
    (tmp_path / "foo").unlink()
    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    if debug:
        if record and replay:
            assert "replay succeeded" in stderr, "replay of file write failed"
        elif replay:
            assert "replay failed" in stderr, "replay succeeded with no trace"
        else:
            assert "replay failed" not in stderr, "replay incorrectly enabled"
            assert "replay succeeded" not in stderr, "replay incorrectly enabled"
        if record and replay:
            assert "record failed" not in stderr, "record still attempted after replay"
            assert "record succeeded" not in stderr, "record after successful replay"
        elif record:
            assert "record succeeded" in stderr, "record of file write failed"
        else:
            assert "record failed" not in stderr, "record incorrectly enabled"
            assert "record succeeded" not in stderr, "record incorrectly enabled"

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
    ret, _, _ = sandbox(args + ["--", "xcache-test-echo", "foo", "bar"], box=tmp_path)
    assert ret != 0, "caching in an invalid directory did not fail"
    assert not nested.exists(), "nested cache directories created"


@pytest.mark.parametrize("debug", (False, True))
def test_nonexistent(debug: bool, tmp_path: Path):
    """
    running something that does not exist should fail
    """
    args = ["xcache"]
    if debug:
        args += ["--debug"]
    args += [f"--dir={tmp_path}/database", "--", tmp_path / "nonexistent"]
    ret, _, _ = sandbox(args, box=tmp_path)
    assert ret == 127, "unexpected return from non-existent exec"

    # even if we cached it, replay should return failure
    ret, _, _ = sandbox(args, box=tmp_path)
    assert ret == 127, "unexpected return from non-existent exec"


def test_nonexistent2(tmp_path: Path):
    """running something that non-existent should report a readable error"""
    args = ["xcache", f"--dir={tmp_path}/database", "--", tmp_path / "nonexistent"]
    _, _, stderr = sandbox(args, box=tmp_path)
    assert (
        os.strerror(errno.ENOENT).lower() in stderr.lower()
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
    strace(["xcache-test-nop"], tmp_path)

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
    args += ["--", "xcache-test-nop"]

    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    if debug:
        if replay:
            assert "replay failed" in stderr, "replay succeeded with no trace"
        else:
            assert "replay failed" not in stderr, "replay incorrectly enabled"
            assert "replay succeeded" not in stderr, "replay incorrectly enabled"
        if record:
            assert "record succeeded" in stderr, "record of no-op failed"
        else:
            assert "record failed" not in stderr, "record incorrectly enabled"
            assert "record succeeded" not in stderr, "record incorrectly enabled"

    # try it again to see if we can replay
    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    if debug:
        if record and replay:
            assert "replay succeeded" in stderr, "replay of no-op failed"
        elif replay:
            assert "replay failed" in stderr, "replay succeeded with no trace"
        else:
            assert "replay failed" not in stderr, "replay incorrectly enabled"
            assert "replay succeeded" not in stderr, "replay incorrectly enabled"
        if record and replay:
            assert "record failed" not in stderr, "record still attempted after replay"
            assert "record succeeded" not in stderr, "record after successful replay"
        elif record:
            assert "record succeeded" in stderr, "record of no-op failed"
        else:
            assert "record failed" not in stderr, "record incorrectly enabled"
            assert "record succeeded" not in stderr, "record incorrectly enabled"


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
    strace([f"xcache-test-print-{stream}"], tmp_path)

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
    args += ["--", f"xcache-test-print-{stream}"]

    ret, stdout, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    if debug:
        if replay:
            assert "replay failed" in stderr, "replay succeeded with no trace"
        else:
            assert "replay failed" not in stderr, "replay incorrectly enabled"
            assert "replay succeeded" not in stderr, "replay incorrectly enabled"
        if record:
            assert "record succeeded" in stderr, f"record of {stream} user failed"
        else:
            assert "record failed" not in stderr, "record incorrectly enabled"
            assert "record succeeded" not in stderr, "record incorrectly enabled"

    if stream == "stdout":
        assert re.search("hello\nworld", stdout), f"missing {stream}"
    else:
        assert re.search("hello\nworld", stderr), f"missing {stream}"

    # try it again to see if we can replay
    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    if debug:
        if record and replay:
            assert "replay succeeded" in stderr, f"replay of {stream} user failed"
        elif replay:
            assert "replay failed" in stderr, "replay succeeded with no trace"
        else:
            assert "replay failed" not in stderr, "replay incorrectly enabled"
            assert "replay succeeded" not in stderr, "replay incorrectly enabled"
        if record and replay:
            assert "record failed" not in stderr, "record still attempted after replay"
            assert "record succeeded" not in stderr, "record after successful replay"
        elif record:
            assert "record succeeded" in stderr, f"record of {stream} user failed"
        else:
            assert "record failed" not in stderr, "record incorrectly enabled"
            assert "record succeeded" not in stderr, "record incorrectly enabled"

    if stream == "stdout":
        assert re.search("hello\nworld", stdout), f"missing {stream}"
    else:
        assert re.search("hello\nworld", stderr), f"missing {stream}"


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
    strace(["xcache-test-write-file"], tmp_path)
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
    args += ["--", "xcache-test-write-file"]

    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    if debug:
        if replay:
            assert "replay failed" in stderr, "replay succeeded with no trace"
        else:
            assert "replay failed" not in stderr, "replay incorrectly enabled"
            assert "replay succeeded" not in stderr, "replay incorrectly enabled"
        if record:
            assert "record succeeded" in stderr, "record of file write failed"
        else:
            assert "record failed" not in stderr, "record incorrectly enabled"
            assert "record succeeded" not in stderr, "record incorrectly enabled"

    assert (tmp_path / "foo").exists(), "file not written"
    assert (tmp_path / "foo").read_text() == "hello world", "file contents not written"

    # try it again to see if we can replay
    (tmp_path / "foo").unlink()
    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    if debug:
        if record and replay:
            assert "replay succeeded" in stderr, "replay of file write failed"
        elif replay:
            assert "replay failed" in stderr, "replay succeeded with no trace"
        else:
            assert "replay failed" not in stderr, "replay incorrectly enabled"
            assert "replay succeeded" not in stderr, "replay incorrectly enabled"
        if record and replay:
            assert "record failed" not in stderr, "record still attempted after replay"
            assert "record succeeded" not in stderr, "record after successful replay"
        elif record:
            assert "record succeeded" in stderr, "record of file write failed"
        else:
            assert "record failed" not in stderr, "record incorrectly enabled"
            assert "record succeeded" not in stderr, "record incorrectly enabled"

    assert (tmp_path / "foo").exists(), "file not written"
    assert (tmp_path / "foo").read_text() == "hello world", "file contents not written"


@pytest.mark.parametrize("debug", (False, True))
def test_version(debug: bool, tmp_path: Path):
    """
    check --version does something reasonable
    """
    args = ["xcache", "--version"]
    if debug:
        args += ["--debug"]
    ret, stdout, _ = sandbox(args, box=tmp_path)
    assert ret == 0
    assert stdout.strip() != "", "--version output nothing"


@pytest.mark.parametrize("debug", (False, True))
@pytest.mark.parametrize("record", (False, True))
@pytest.mark.parametrize("replay", (False, True))
def test_uncacheable(debug: bool, record: bool, replay: bool, tmp_path: Path):
    """tracing of something we know we cannot cache"""

    # First, `strace` the process we are about to test. If the test fails, the
    # `strace` output will show what syscalls it made which may aid debugging.
    # This is useful when, e.g., running on a new kernel where the dynamic
    # loader or libc makes unanticipated syscalls.
    strace(["xcache-test-uncacheable"], tmp_path)

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
    args += ["--", "xcache-test-uncacheable"]

    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    if debug:
        if replay:
            assert "replay failed" in stderr, "replay succeeded with no trace"
        else:
            assert "replay failed" not in stderr, "replay incorrectly enabled"
            assert "replay succeeded" not in stderr, "replay incorrectly enabled"
        if record:
            assert "record failed" in stderr, "record of uncacheable succeeded"
        else:
            assert "record failed" not in stderr, "record incorrectly enabled"
            assert "record succeeded" not in stderr, "record incorrectly enabled"

    # try it again to see if we can replay
    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    if debug:
        if record and replay:
            assert "replay failed" in stderr, "replay of uncacheable succeeded"
        elif replay:
            assert "replay failed" in stderr, "replay succeeded with no trace"
        else:
            assert "replay failed" not in stderr, "replay incorrectly enabled"
            assert "replay succeeded" not in stderr, "replay incorrectly enabled"
        if record and replay:
            assert "record failed" in stderr, "record of uncacheable succeeded"
        elif record:
            assert "record failed" in stderr, "record of uncacheable succeeded"
        else:
            assert "record failed" not in stderr, "record incorrectly enabled"
            assert "record succeeded" not in stderr, "record incorrectly enabled"


@pytest.mark.parametrize(
    "debug", (pytest.param(False, id="nodebug"), pytest.param(True, id="debug"))
)
@pytest.mark.parametrize(
    "record",
    (
        pytest.param(False, id="norecord"),
        pytest.param(True, id="record"),
    ),
)
@pytest.mark.parametrize(
    "replay", (pytest.param(False, id="noreplay"), pytest.param(True, id="replay"))
)
def test_exec_dups_fds(debug: bool, record: bool, replay: bool, tmp_path: Path):
    """does xcache recognise that `execve` unshares the file descriptor table?"""

    # First, `strace` the process we are about to test. If the test fails, the
    # `strace` output will show what syscalls it made which may aid debugging.
    # This is useful when, e.g., running on a new kernel where the dynamic
    # loader or libc makes unanticipated syscalls.
    strace(["xcache-test-clone-exec-with-fd"], tmp_path)

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
    args += ["--", "xcache-test-clone-exec-with-fd"]

    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    if debug:
        if replay:
            assert "replay failed" in stderr, "replay succeeded with no trace"
        else:
            assert "replay failed" not in stderr, "replay incorrectly enabled"
            assert "replay succeeded" not in stderr, "replay incorrectly enabled"
        if record:
            assert "record succeeded" in stderr, "record failed"
        else:
            assert "record failed" not in stderr, "record incorrectly enabled"
            assert "record succeeded" not in stderr, "record incorrectly enabled"

    # try it again to see if we can replay
    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    if debug:
        if record and replay:
            assert "replay succeeded" in stderr, "replay failed"
        elif replay:
            assert "replay failed" in stderr, "replay succeeded with no trace"
        else:
            assert "replay failed" not in stderr, "replay incorrectly enabled"
            assert "replay succeeded" not in stderr, "replay incorrectly enabled"
        if record and replay:
            assert "record failed" not in stderr, "record still attempted after replay"
            assert "record succeeded" not in stderr, "record after successful replay"
        elif record:
            assert "record succeeded" in stderr, "record failed"
        else:
            assert "record failed" not in stderr, "record incorrectly enabled"
            assert "record succeeded" not in stderr, "record incorrectly enabled"


@pytest.mark.parametrize(
    "debug", (pytest.param(False, id="nodebug"), pytest.param(True, id="debug"))
)
@pytest.mark.parametrize(
    "record",
    (
        pytest.param(False, id="norecord"),
        pytest.param(True, id="record"),
    ),
)
@pytest.mark.parametrize(
    "replay", (pytest.param(False, id="noreplay"), pytest.param(True, id="replay"))
)
def test_close_on_exec(debug: bool, record: bool, replay: bool, tmp_path: Path):
    """does xcache understand semantics of the close-on-exec flag?"""

    # First, `strace` the process we are about to test. If the test fails, the
    # `strace` output will show what syscalls it made which may aid debugging.
    # This is useful when, e.g., running on a new kernel where the dynamic
    # loader or libc makes unanticipated syscalls.
    strace(["xcache-test-close-on-exec"], tmp_path)

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
    args += ["--", "xcache-test-close-on-exec"]

    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    if debug:
        if replay:
            assert "replay failed" in stderr, "replay succeeded with no trace"
        else:
            assert "replay failed" not in stderr, "replay incorrectly enabled"
            assert "replay succeeded" not in stderr, "replay incorrectly enabled"
        if record:
            assert "record succeeded" in stderr, "record failed"
        else:
            assert "record failed" not in stderr, "record incorrectly enabled"
            assert "record succeeded" not in stderr, "record incorrectly enabled"

    # try it again to see if we can replay
    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    if debug:
        if record and replay:
            assert "replay succeeded" in stderr, "replay failed"
        elif replay:
            assert "replay failed" in stderr, "replay succeeded with no trace"
        else:
            assert "replay failed" not in stderr, "replay incorrectly enabled"
            assert "replay succeeded" not in stderr, "replay incorrectly enabled"
        if record and replay:
            assert "record failed" not in stderr, "record still attempted after replay"
            assert "record succeeded" not in stderr, "record after successful replay"
        elif record:
            assert "record succeeded" in stderr, "record failed"
        else:
            assert "record failed" not in stderr, "record incorrectly enabled"
            assert "record succeeded" not in stderr, "record incorrectly enabled"


@pytest.mark.parametrize(
    "debug", (pytest.param(False, id="nodebug"), pytest.param(True, id="debug"))
)
@pytest.mark.parametrize(
    "record",
    (
        pytest.param(False, id="norecord"),
        pytest.param(True, id="record"),
    ),
)
@pytest.mark.parametrize(
    "replay", (pytest.param(False, id="noreplay"), pytest.param(True, id="replay"))
)
def test_umask(debug: bool, record: bool, replay: bool, tmp_path: Path):
    """does xcache understand umask semantics?"""

    # First, `strace` the process we are about to test. If the test fails, the
    # `strace` output will show what syscalls it made which may aid debugging.
    # This is useful when, e.g., running on a new kernel where the dynamic
    # loader or libc makes unanticipated syscalls.
    strace(["xcache-test-umask-open"], tmp_path)
    foo = tmp_path / "foo"
    foo.unlink()

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
    args += ["--", "xcache-test-umask-open"]

    ret, _, _ = sandbox(args, box=tmp_path)
    assert ret == 0

    assert foo.exists(), "expected output was not created"
    assert (
        stat.S_IMODE(foo.stat().st_mode) == 0o777
    ), "expected file mode was not applied"

    # try it again to see if we can replay
    foo.unlink()
    ret, _, _ = sandbox(args, box=tmp_path)
    assert ret == 0

    assert foo.exists(), "expected output was not created"
    assert (
        stat.S_IMODE(foo.stat().st_mode) == 0o777
    ), "expected file mode was not applied"


@pytest.mark.parametrize(
    "debug", (pytest.param(False, id="nodebug"), pytest.param(True, id="debug"))
)
@pytest.mark.parametrize(
    "record",
    (
        pytest.param(False, id="norecord"),
        pytest.param(True, id="record"),
    ),
)
@pytest.mark.parametrize(
    "replay", (pytest.param(False, id="noreplay"), pytest.param(True, id="replay"))
)
def test_umask2(debug: bool, record: bool, replay: bool, tmp_path: Path):
    """a variant of `test_umask` that _does_ expect masking"""

    # First, `strace` the process we are about to test. If the test fails, the
    # `strace` output will show what syscalls it made which may aid debugging.
    # This is useful when, e.g., running on a new kernel where the dynamic
    # loader or libc makes unanticipated syscalls.
    strace(["xcache-test-umask-open2"], tmp_path)
    foo = tmp_path / "foo"
    foo.unlink()

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
    args += ["--", "xcache-test-umask-open2"]

    ret, _, _ = sandbox(args, box=tmp_path)
    assert ret == 0

    assert foo.exists(), "expected output was not created"
    assert (
        stat.S_IMODE(foo.stat().st_mode) == 0o666
    ), "expected file mode was not applied"

    # try it again to see if we can replay
    foo.unlink()
    ret, _, _ = sandbox(args, box=tmp_path)
    assert ret == 0

    assert foo.exists(), "expected output was not created"
    assert (
        stat.S_IMODE(foo.stat().st_mode) == 0o666
    ), "expected file mode was not applied"


@pytest.mark.parametrize(
    "CLONE_FILES",
    (pytest.param(False, id="~CLONE_FILES"), pytest.param(True, id="CLONE_FILES")),
)
@pytest.mark.parametrize(
    "CLONE_FS", (pytest.param(False, id="~CLONE_FS"), pytest.param(True, id="CLONE_FS"))
)
@pytest.mark.parametrize(
    "CLONE_VM", (pytest.param(False, id="~CLONE_VM"), pytest.param(True, id="CLONE_VM"))
)
def test_ld_preload_in_child(
    CLONE_FILES: bool, CLONE_FS: bool, CLONE_VM: bool, tmp_path: Path
):
    """
    does libxcache-spy correctly propagate to cloned children?

    When cloning (or forking, vforking, etc), the child needs to receive a copy of
    libxcache-spy or reuse the parent’s libxcache-spy. Without this, relevant userspace
    actions performed in the child will go unseen. This test checks that various clone
    operations do propagate this correctly.
    """

    tracee = ["xcache-test-ld-preload-in-child"]
    if CLONE_FILES:
        tracee += ["CLONE_FILES"]
    if CLONE_FS:
        tracee += ["CLONE_FS"]
    if CLONE_VM:
        tracee += ["CLONE_VM"]

    # First, `strace` the process we are about to test. If the test fails, the
    # `strace` output will show what syscalls it made which may aid debugging.
    # This is useful when, e.g., running on a new kernel where the dynamic
    # loader or libc makes unanticipated syscalls.
    strace(tracee, tmp_path)

    args = [
        "xcache",
        "--debug",
        f"--dir={tmp_path}/database",
        "--read-write",
        "--",
    ] + tracee

    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    assert "record succeeded" in stderr, "record failed"

    # if tracing successfully propagated libxcache-spy to the child, we should
    # perceive the child’s `sysconf`
    assert "called sysconf(30 /* _SC_PAGESIZE */)" in stderr, "sysconf in child unseen"


def test_exec_sysconf(tmp_path: Path):
    """
    do exec-ed children correctly pick up libxcache-spy?

    When exec-ing, ones address space is replaced. This means getting a new copy of
    libxcache-spy. This test checks whether this new spy correctly starts up and
    observes the new process’ actions.
    """

    tracee = ["xcache-test-execvp", "xcache-test-sysconf", "_SC_PAGESIZE"]

    # First, `strace` the process we are about to test. If the test fails, the
    # `strace` output will show what syscalls it made which may aid debugging.
    # This is useful when, e.g., running on a new kernel where the dynamic
    # loader or libc makes unanticipated syscalls.
    strace(tracee, tmp_path)

    args = [
        "xcache",
        "--debug",
        f"--dir={tmp_path}/database",
        "--read-write",
        "--",
    ] + tracee

    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    assert "record succeeded" in stderr, "record failed"

    # if tracing successfully propagated libxcache-spy to the child, we should
    # perceive the child’s `sysconf`
    assert "called sysconf(30 /* _SC_PAGESIZE */)" in stderr, "sysconf in child unseen"


def test_bug_set_size(tmp_path: Path):
    """
    can we insert multiple entries into the `getenv`-looked-up set?

    There was previously a bug wherein the size of the set of looked up environment
    variables was not incremented. This does not quite test that case but probes
    something along the same lines.

    Args:
        tmp_path: Temporary directory supplied by Pytest
    """

    # run the command under xcache
    args = [
        "xcache",
        "--debug",
        f"--dir={tmp_path}/database",
        "--read-write",
        "--",
        "xcache-test-bug-set-size",
    ]
    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    assert "record succeeded" in stderr, "record failed"


@pytest.mark.parametrize("export1", (None, "foo", "bar"))
@pytest.mark.parametrize("export2", (None, "foo", "bar"))
@pytest.mark.parametrize("export3", (None, "foo", "bar"))
def test_clearenv(
    export1: None | str, export2: None | str, export3: None | str, tmp_path: Path
):
    """
    does xcache understand `clearenv`?

    Some programs call `clearenv` to wipe their environment. In this situation, xcache
    should recognise that no variable reads after this represent data from an external
    source.

    Args:
        export1: What to export as `$FOO` during the first run
        export2: What to export as `$FOO` during the second run
        export3: What to export as `$FOO` during the third run
        tmp_path: Temporary directory supplied by Pytest
    """

    # create an environment for running our process
    env = os.environ.copy()
    if export1 is None:
        if "FOO" in env:
            del env["FOO"]
    else:
        env["FOO"] = export1

    # run the command under xcache
    args = [
        "xcache",
        "--debug",
        f"--dir={tmp_path}/database",
        "--read-write",
        "--",
        "xcache-test-clearenv",
    ]
    ret, _, stderr = sandbox(args, box=tmp_path, env=env)
    assert ret == 0

    assert "record succeeded" in stderr, "record failed"

    # set the environment variable for a second run
    env = os.environ.copy()
    if export2 is None:
        if "FOO" in env:
            del env["FOO"]
    else:
        env["FOO"] = export2

    # run the command a second time
    ret, _, stderr = sandbox(args, box=tmp_path, env=env)
    assert ret == 0

    # replay should be independent of the environment variable
    assert "replay succeeded" in stderr, "replay failed"

    # set the environment variable for a third run
    env = os.environ.copy()
    if export3 is None:
        if "FOO" in env:
            del env["FOO"]
    else:
        env["FOO"] = export3

    # run the command a second time
    ret, _, stderr = sandbox(args, box=tmp_path, env=env)
    assert ret == 0

    # replay should be independent of the environment variable
    assert "replay succeeded" in stderr, "replay failed"


@pytest.mark.parametrize("direct", (False, True))
def test_creat(direct: bool, tmp_path: Path):
    """
    can we handle the `creat` syscall?

    Args:
        direct: invoke `creat` directly instead of via its libc wrapper?
        tmp_path: Temporary directory supplied by Pytest
    """

    exe = f"xcache-test-creat{'2' if direct else ''}"

    # First, `strace` the process we are about to test. If the test fails, the
    # `strace` output will show what syscalls it made which may aid debugging.
    # This is useful when, e.g., running on a new kernel where the dynamic
    # loader or libc makes unanticipated syscalls.
    strace([exe], tmp_path)
    foo = tmp_path / "foo"
    foo.unlink()

    # now try tracing it
    args = [
        "xcache",
        "--debug",
        f"--dir={tmp_path}/database",
        "--read-write",
        "--",
        exe,
    ]
    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0
    assert "record succeeded" in stderr, "record failed"
    assert foo.read_text(encoding="utf-8") == "bar", "incorrect content written"
    foo.unlink()

    # now try replaying it
    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0
    assert "replay succeeded" in stderr, "replay failed"
    assert foo.read_text(encoding="utf-8") == "bar", "incorrect content written"


def test_fd_without_path(tmp_path: Path):
    """can we handle a tracee that creates FDs that do not map to disk paths?"""

    # we do not guarantee we can trace such a thing, but it should at least run
    # successfully
    ret, _, _ = sandbox(
        [
            "xcache",
            "--debug",
            f"--dir={tmp_path}/database",
            "--read-write",
            "--",
            "xcache-test-fd-without-path",
        ],
        box=tmp_path,
    )
    assert ret == 0


@pytest.mark.parametrize("export1", (False, True))
@pytest.mark.parametrize("export2", (False, True))
def test_getenv(export1: bool, export2: bool, tmp_path: Path):
    """
    does xcache understand `getenv` dependencies?

    Some programs call `getenv` to look at environment variables and then modify their
    behaviour based on the value of those environment variables. Xcache needs to see and
    account for such calls in order to correctly trace and replay these programs.

    Args:
        export1: Should `$FOO` be exported when running the command for the first time?
        export2: Should `$FOO` be exported when running the command for the second time?
        tmp_path: Temporary directory supplied by Pytest
    """

    # create an environment for running our process
    env = os.environ.copy()
    if export1:
        env["FOO"] = "bar"
    else:
        if "FOO" in env:
            del env["FOO"]

    # run the command under xcache
    args = [
        "xcache",
        "--debug",
        f"--dir={tmp_path}/database",
        "--read-write",
        "--",
        "xcache-test-getenv",
    ]
    ret, _, stderr = sandbox(args, box=tmp_path, env=env)
    assert ret == 0

    assert "record succeeded" in stderr, "record failed"

    # if `FOO` was set, we should have written the output file
    foo = tmp_path / "foo"
    if export1:
        assert foo.exists(), "output file not written"
        assert (
            foo.read_text(encoding="utf-8") == "hello world"
        ), "incorrect content written"
        foo.unlink()
    else:
        assert not foo.exists(), "output file written"

    # create an environment for the second run
    env = os.environ.copy()
    if export2:
        env["FOO"] = "bar"
    else:
        if "FOO" in env:
            del env["FOO"]

    # run the command a second time
    ret, _, stderr = sandbox(args, box=tmp_path, env=env)
    assert ret == 0

    # replay should be dependent on the environment variable matching
    if export1 == export2:
        assert "replay succeeded" in stderr, "replay failed"
    else:
        assert "replay succeeded" not in stderr, "replay incorrectly succeeded"

    # if `FOO` was set, we should have written the output file
    if export2:
        assert foo.exists(), "output file not written"
        assert (
            foo.read_text(encoding="utf-8") == "hello world"
        ), "incorrect content written"
    else:
        assert not foo.exists(), "output file written"


def test_non_path_fds(tmp_path: Path):
    """can we trace processes that create in-memory file descriptors?"""

    # First, `strace` the process we are about to test. If the test fails, the
    # `strace` output will show what syscalls it made which may aid debugging.
    # This is useful when, e.g., running on a new kernel where the dynamic
    # loader or libc makes unanticipated syscalls.
    strace(["xcache-test-non-path-fds"], tmp_path)

    # run it under xcache
    args = [
        "xcache",
        "--debug",
        f"--dir={tmp_path}/database",
        "--read-write",
        "--",
        "xcache-test-non-path-fds",
    ]
    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0

    # we should have been able to cache it
    assert "record succeeded" in stderr, "record failed"


@pytest.mark.parametrize("mode", ("O_RDONLY", "O_WRONLY", "O_RDWR"))
@pytest.mark.parametrize(
    "creat", (pytest.param(False, id=""), pytest.param(True, id="O_CREAT"))
)
@pytest.mark.parametrize(
    "excl", (pytest.param(False, id=""), pytest.param(True, id="O_EXCL"))
)
@pytest.mark.parametrize(
    "trunc", (pytest.param(False, id=""), pytest.param(True, id="O_TRUNC"))
)
@pytest.mark.parametrize(
    "exist",
    (
        pytest.param(False, id="no existing file"),
        pytest.param(True, id="existing file"),
    ),
)
def test_open(
    mode: str, creat: bool, excl: bool, trunc: bool, exist: bool, tmp_path: Path
):
    """
    can we successfully trace variations of `open`?

    Args:
        mode: Read/write mode in which to open a file
        creat: Use `O_CREAT`?
        excl: Use `O_EXCL`?
        trunc: Use `O_TRUNC`?
        exist: Create a pre-existing file of a colliding name?
        tmp_path: Temporary directory supplied by Pytest
    """

    # determine command line options
    args = ["xcache-test-open", mode]
    if creat:
        args += ["O_CREAT"]
    if excl:
        args += ["O_EXCL"]
    if trunc:
        args += ["O_TRUNC"]

    # setup initial state
    target = tmp_path / "foo"
    if exist:
        target.write_bytes(b"hello world")

    # run the open without xcache
    ret, _, _ = sandbox(args, box=tmp_path)
    assert ret == 0

    # what outcome did this produce?
    after_exists = target.exists()
    after_content = target.read_bytes() if after_exists else None
    after_mode = target.stat().st_mode if after_exists else None

    # re-setup initial conditions
    target.unlink(missing_ok=True)
    if exist:
        target.write_bytes(b"hello world")

    # run under xcache
    xcache = [
        "xcache",
        "--debug",
        f"--dir={tmp_path}/database",
        "--read-write",
        "--",
    ] + args
    ret, _, stderr = sandbox(xcache, box=tmp_path)
    assert ret == 0

    # some combinations of flags are invalid
    is_invalid = False
    is_invalid |= excl and not creat
    is_invalid |= mode == "O_RDONLY" and trunc

    # whether we succeeded to record or not depends on the operation validity
    if is_invalid:
        assert "record succeeded" not in stderr, "record incorrectly succeeded"
    else:
        assert "record succeeded" in stderr, "record failed"

    # check outcomes were the same
    after_exists1 = target.exists()
    after_content1 = target.read_bytes() if after_exists1 else None
    after_mode1 = target.stat().st_mode if after_exists1 else None
    assert after_exists == after_exists1
    assert after_content == after_content1
    assert after_mode == after_mode1

    # re-setup initial conditions
    target.unlink(missing_ok=True)
    if exist:
        target.write_bytes(b"hello world")

    if is_invalid:
        return

    # try to replay
    ret, _, stderr = sandbox(xcache, box=tmp_path)
    assert ret == 0
    assert "replay succeeded" in stderr, "replay failed"

    # check outcomes were the same
    after_exists1 = target.exists()
    after_content1 = target.read_bytes() if after_exists1 else None
    after_mode1 = target.stat().st_mode if after_exists1 else None
    assert after_exists == after_exists1
    assert after_content == after_content1
    assert after_mode == after_mode1


def test_open2(tmp_path: Path):
    """can we handle the `open` syscall?"""

    # First, `strace` the process we are about to test. If the test fails, the
    # `strace` output will show what syscalls it made which may aid debugging.
    # This is useful when, e.g., running on a new kernel where the dynamic
    # loader or libc makes unanticipated syscalls.
    strace(["xcache-test-open2"], tmp_path)
    foo = tmp_path / "foo"
    foo.unlink()

    # now try tracing it
    args = [
        "xcache",
        "--debug",
        f"--dir={tmp_path}/database",
        "--read-write",
        "--",
        "xcache-test-open2",
    ]
    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0
    assert "record succeeded" in stderr, "record failed"
    assert foo.read_text(encoding="utf-8") == "bar", "incorrect content written"
    foo.unlink()

    # now try replaying it
    ret, _, stderr = sandbox(args, box=tmp_path)
    assert ret == 0
    assert "replay succeeded" in stderr, "replay failed"
    assert foo.read_text(encoding="utf-8") == "bar", "incorrect content written"


def test_open_rdonly_creat_exists(tmp_path: Path):
    """
    can we handle `open(…, O_RDONLY|O_CREAT)` when the file already exists?

    Somewhat surprisingly, the `open` flags combination of `O_RDONLY|O_CREAT` is legal.
    If the file already exists, it is opened read-only. If the file does not already
    exist, an empty file is created and then opened read-only. This leads to an odd
    situation wherein this variant of an `O_RDONLY` `open` call is conditionally an
    “output” to xcache.

    Handling this form of `open` correctly is a little tricky and one mental plan I had
    for xcache implementation would have run into a subtle bug with this case. Namely,
    if the file already exists but is also zero-sized and the `open` call is recorded as
    creation, replay can incorrectly fail when trying to recreate the file (`EEXIST`).
    This test confirms we do not have that bug.
    """

    # confirm `open(…, O_RDONLY|O_CREAT)` really does have the behaviour we claim above
    wd = tmp_path / "baseline1"
    wd.mkdir()
    foo = wd / "foo"
    assert not foo.exists(), "logic error in test setup"
    args = ["xcache-test-open", "O_RDONLY", "O_CREAT"]
    ret, _, _ = sandbox(args, box=wd)
    assert ret == 0
    assert foo.exists(), "`open(…, O_RDONLY|O_CREAT)` does not create files"

    # confirm `open(…, O_RDONLY|O_CREAT)` works when the file exists, even if it is
    # read-only
    wd = tmp_path / "baseline2"
    wd.mkdir()
    foo = wd / "foo"
    assert not foo.exists(), "logic error in test setup"
    foo.touch(0o400, exist_ok=False)
    ret, _, stderr = sandbox(args, box=wd)
    assert ret == 0
    assert (
        "open failed" not in stderr
    ), "`open(…, O_RDONLY|O_CREAT)` cannot open read-only files"

    # confirm we can record baseline2’s situation
    wd = tmp_path / "testcase"
    wd.mkdir()
    foo = wd / "foo"
    assert not foo.exists(), "logic error in test setup"
    foo.touch(0o400, exist_ok=False)
    xcache = ["xcache", "--debug", f"--dir={wd}/database", "--read-write", "--"]
    ret, _, stderr = sandbox(xcache + args, box=wd)
    assert ret == 0
    assert "record succeeded" in stderr, "record failed"

    assert foo.exists(), "`xcache … xcache-test-open O_RDONLY O_CREATE` deleted a file"
    assert (
        stat.S_IMODE(foo.stat().st_mode) == 0o400
    ), "`xcache … xcache-test-open O_RDONLY O_CREATE` changed the mode of target file"

    # confirm we can replay this
    ret, _, stderr = sandbox(xcache + args, box=wd)
    assert ret == 0
    assert "replay succeeded" in stderr, "replay failed"


@pytest.mark.parametrize("preload", ("append", "either", "prepend"))
@pytest.mark.skipif(shutil.which("ldd") is None, reason="ldd not available")
def test_previous_ld_preload(preload: str, tmp_path: Path):
    """
    are `$LD_PRELOAD`s set by the user preserved under tracing?

    Args:
        preload: `--preload-*` option to pass to xcache
        tmp_path: Temporary directory supplied by Pytest
    """

    exe = Path(shutil.which("xcache-test-ld-preload"))
    so = exe.parent / "libxcache-test-ld-preload-cos.so"

    # was xcache compiled with `-fsanitize=address`?
    xcache = shutil.which("xcache")
    links = subprocess.check_output(["ldd", "--", xcache], text=True)
    libasan: None | Path = None
    for line in links.splitlines():
        m = re.match(r"\s*libasan.*\s+=>\s+(?P<path>/[^\s]+)", line)
        if m is None:
            continue
        libasan = Path(m.group("path"))
        break

    # xcache should be able to record this binary
    env = os.environ.copy()
    env["LD_PRELOAD"] = str(so) if libasan is None else f"{libasan}:{so}"
    args = [
        "xcache",
        "--debug",
        f"--dir={tmp_path}/database",
        "--read-write",
        f"--preload-{preload}",
        "--",
        "xcache-test-ld-preload",
    ]
    ret, _, stderr = sandbox(args, box=tmp_path, env=env)

    # if libasan.so was loaded some unreplayable things like `clock_gettime()` will have
    # happened, but otherwise we expect a successful record
    if libasan is None:
        assert "record succeeded" in stderr, "record failed"

    # the preload should have worked as long as either (1) libasan.so was not required
    # or (2) the spy was added to `$LD_PRELOAD` after libasan.so
    if libasan is None or preload == "append":
        assert ret == 42, "$LD_PRELOAD was not preserved under tracing"


def test_previous_ld_preload_smoke(tmp_path: Path):
    """
    test that the `xcache-test-ld-preload` binary behaves as expected

    If this test fails, the results of `test_previous_ld_preload` can be considered
    invalid.
    """

    exe = Path(shutil.which("xcache-test-ld-preload"))
    so = exe.parent / "libxcache-test-ld-preload-cos.so"

    assert exe.exists(), "missing binary"
    assert so.exists(), "missing library to preload"

    # without the preload active, the binary should return `floor(cos(1) * 10)`
    ret, _, _ = sandbox([exe], box=tmp_path)
    assert ret == math.floor(math.cos(1) * 10), "misbehaviour without preload"

    # with the preload active, the binary should return 42
    env = os.environ.copy()
    env["LD_PRELOAD"] = str(so)
    ret, _, _ = sandbox([exe], box=tmp_path, env=env)
    assert ret == 42, "misbehaviour with preload"


def test_putenv(tmp_path: Path):
    """
    does xcache understand `putenv`?

    Some programs call `putenv` to modify their environment. In this situation, xcache
    should recognise that the set variable no longer represents data from an external
    source. Reading this variable should not incur a dependency.

    Args:
        tmp_path: Temporary directory supplied by Pytest
    """

    # create an environment for running our process
    env = os.environ.copy()
    env["FOO"] = "baz"

    # run the command under xcache
    args = [
        "xcache",
        "--debug",
        f"--dir={tmp_path}/database",
        "--read-write",
        "--",
        "xcache-test-putenv",
    ]
    ret, _, stderr = sandbox(args, box=tmp_path, env=env)
    assert ret == 0

    assert "record succeeded" in stderr, "record failed"

    # set the environment variable differently for a second run
    env = os.environ.copy()
    env["FOO"] = "qux"

    # run the command a second time
    ret, _, stderr = sandbox(args, box=tmp_path, env=env)
    assert ret == 0

    # replay should be independent of the environment variable
    assert "replay succeeded" in stderr, "replay failed"


def test_setenv(tmp_path: Path):
    """
    does xcache understand `setenv`?

    Some programs call `setenv` to modify their environment. In this situation, xcache
    should recognise that the set variable no longer represents data from an external
    source. Reading this variable should not incur a dependency.

    Args:
        tmp_path: Temporary directory supplied by Pytest
    """

    # create an environment for running our process
    env = os.environ.copy()
    env["FOO"] = "baz"

    # run the command under xcache
    args = [
        "xcache",
        "--debug",
        f"--dir={tmp_path}/database",
        "--read-write",
        "--",
        "xcache-test-setenv",
    ]
    ret, _, stderr = sandbox(args, box=tmp_path, env=env)
    assert ret == 0

    assert "record succeeded" in stderr, "record failed"

    # set the environment variable differently for a second run
    env = os.environ.copy()
    env["FOO"] = "qux"

    # run the command a second time
    ret, _, stderr = sandbox(args, box=tmp_path, env=env)
    assert ret == 0

    # replay should be independent of the environment variable
    assert "replay succeeded" in stderr, "replay failed"


def test_temp_usage(tmp_path: Path):
    """can we trace something compiler-like?"""

    # constrain temporary files to a test-local path
    env = os.environ.copy()
    env["TMPDIR"] = str(tmp_path)

    # confirm we can record the tracee
    args = [
        "xcache",
        "--debug",
        f"--dir={tmp_path}/database",
        "--read-write",
        "--",
        "xcache-test-temp-usage",
    ]
    ret, _, stderr = sandbox(args, box=tmp_path, env=env)
    assert ret == 0
    assert "record succeeded" in stderr, "record failed"

    # confirm we can replay it
    ret, _, stderr = sandbox(args, box=tmp_path, env=env)
    assert ret == 0
    assert "replay succeeded" in stderr, "replay failed"


@pytest.mark.parametrize(
    "dir_read", (pytest.param(False, id="-"), pytest.param(True, id="r"))
)
@pytest.mark.parametrize(
    "dir_write", (pytest.param(False, id="-"), pytest.param(True, id="w"))
)
@pytest.mark.parametrize(
    "dir_execute", (pytest.param(False, id="-"), pytest.param(True, id="x"))
)
@pytest.mark.parametrize(
    "file_read", (pytest.param(False, id="-"), pytest.param(True, id="r"))
)
@pytest.mark.parametrize(
    "file_write", (pytest.param(False, id="-"), pytest.param(True, id="w"))
)
@pytest.mark.parametrize(
    "file_execute", (pytest.param(False, id="-"), pytest.param(True, id="x"))
)
def test_unlink(
    dir_read: bool,
    dir_write: bool,
    dir_execute: bool,
    file_read: bool,
    file_write: bool,
    file_execute: bool,
    tmp_path: Path,
):
    """various variations of `unlink` scenarios"""

    # create a directory
    bar = tmp_path / "bar"
    bar.mkdir()

    # create a file with the given permission set
    mode = file_read * 0o400 + file_write * 0o200 + file_execute * 0o100
    foo = bar / "foo"
    foo.touch(mode)

    # make the directory the requested mode
    mode = dir_read * 0o400 + dir_write * 0o200 + dir_execute * 0o100
    bar.chmod(mode)

    # run the raw unlink
    baseline, _, _ = sandbox(["xcache-test-unlink"], box=tmp_path)
    bar.chmod(0o700)
    removed = foo.exists()

    # clean up and recreate
    shutil.rmtree(bar)
    bar.mkdir()
    mode = file_read * 0o400 + file_write * 0o200 + file_execute * 0o100
    foo = bar / "foo"
    foo.touch(mode)
    mode = dir_read * 0o400 + dir_write * 0o200 + dir_execute * 0o100
    bar.chmod(mode)

    # try caching it
    record, _, stderr = sandbox(
        [
            "xcache",
            "--debug",
            f"--dir={tmp_path}/database",
            "--read-write",
            "--",
            "xcache-test-unlink",
        ],
        box=tmp_path,
    )
    assert record == baseline, "differing result under xcache"
    bar.chmod(0o700)
    assert removed == foo.exists(), "differing outcome under xcache"

    if baseline != 0:
        return
    assert "record succeeded" in stderr, "recording of successful unlink failed"

    # clean up and recreate
    shutil.rmtree(bar)
    bar.mkdir()
    mode = file_read * 0o400 + file_write * 0o200 + file_execute * 0o100
    foo = bar / "foo"
    foo.touch(mode)
    mode = dir_read * 0o400 + dir_write * 0o200 + dir_execute * 0o100
    bar.chmod(mode)

    # try replaying it
    replay, _, stderr = sandbox(
        [
            "xcache",
            "--debug",
            f"--dir={tmp_path}/database",
            "--read-write",
            "--",
            "xcache-test-unlink",
        ],
        box=tmp_path,
    )
    assert replay == baseline, "differing result under xcache"
    bar.chmod(0o700)
    assert removed == foo.exists(), "differing outcome under xcache"

    assert "replay succeeded" in stderr, "replay of unlink failed"


def test_unreadable_output(tmp_path: Path):
    """
    can we record (or reasonably fail to record) a process that creates an unreadable
    file?
    """

    # run the creator uninstrumented to confirm its actions work
    ret, _, _ = sandbox(["xcache-test-unreadable-output"], box=tmp_path)
    assert ret == 0

    # it should have created an unreadable file
    foo = tmp_path / "foo"
    assert stat.S_IMODE(foo.stat().st_mode) == 0o000, "unexpected file mode"

    # clean up and reset
    foo.unlink()

    # now run the creator under xcache
    ret, _, _ = sandbox(
        [
            "xcache",
            "--debug",
            f"--dir={tmp_path}/database",
            "--read-write",
            "--",
            "xcache-test-unreadable-output",
        ],
        box=tmp_path,
    )
    assert ret == 0

    # it should have created the same thing
    assert stat.S_IMODE(foo.stat().st_mode) == 0o000, "unexpected file mode"


def test_unsetenv(tmp_path: Path):
    """
    does xcache understand `unsetenv`?

    Some programs call `unsetenv` to modify their environment. In this situation, xcache
    should recognise that the set variable no longer represents data from an external
    source. Reading this variable should not incur a dependency.

    Args:
        tmp_path: Temporary directory supplied by Pytest
    """

    # create an environment for running our process
    env = os.environ.copy()
    env["FOO"] = "baz"

    # run the command under xcache
    args = [
        "xcache",
        "--debug",
        f"--dir={tmp_path}/database",
        "--read-write",
        "--",
        "xcache-test-unsetenv",
    ]
    ret, _, stderr = sandbox(args, box=tmp_path, env=env)
    assert ret == 0

    assert "record succeeded" in stderr, "record failed"

    # set the environment variable differently for a second run
    env = os.environ.copy()
    env["FOO"] = "qux"

    # run the command a second time
    ret, _, stderr = sandbox(args, box=tmp_path, env=env)
    assert ret == 0

    # replay should be independent of the environment variable
    assert "replay succeeded" in stderr, "replay failed"
