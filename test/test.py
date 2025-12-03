"""
Xcache test suite
"""

import errno
import os
import stat
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
    strace([forker], tmp_path)
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
    args += ["--", forker]

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


@pytest.mark.parametrize("debug", (False, True))
@pytest.mark.parametrize("record", (False, True))
@pytest.mark.parametrize("replay", (False, True))
def test_uncacheable(debug: bool, record: bool, replay: bool, tmp_path: Path):
    """tracing of something we know we cannot cache"""

    # First, `strace` the process we are about to test. If the test fails, the
    # `strace` output will show what syscalls it made which may aid debugging.
    # This is useful when, e.g., running on a new kernel where the dynamic
    # loader or libc makes unanticipated syscalls.
    strace(["uncacheable"])

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
    args += ["--", "uncacheable"]

    p = subprocess.run(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
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
            assert "record failed" in p.stdout, "record of uncacheable succeeded"
        else:
            assert "record failed" not in p.stdout, "record incorrectly enabled"
            assert "record succeeded" not in p.stdout, "record incorrectly enabled"

    # try it again to see if we can replay
    p = subprocess.run(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        timeout=120,
        check=False,
    )
    print(f"output:\n{p.stdout}\n")
    p.check_returncode()

    if debug:
        if record and replay:
            assert "replay failed" in p.stdout, "replay of uncacheable succeeded"
        elif replay:
            assert "replay failed" in p.stdout, "replay succeeded with no trace"
        else:
            assert "replay failed" not in p.stdout, "replay incorrectly enabled"
            assert "replay succeeded" not in p.stdout, "replay incorrectly enabled"
        if record and replay:
            assert "record failed" in p.stdout, "record of uncacheable succeeded"
        elif record:
            assert "record failed" in p.stdout, "record of uncacheable succeeded"
        else:
            assert "record failed" not in p.stdout, "record incorrectly enabled"
            assert "record succeeded" not in p.stdout, "record incorrectly enabled"


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
    strace(["clone-exec-with-fd"])

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
    args += ["--", "clone-exec-with-fd"]

    output = subprocess.check_output(
        args,
        stderr=subprocess.STDOUT,
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
            assert "record succeeded" in output, "record failed"
        else:
            assert "record failed" not in output, "record incorrectly enabled"
            assert "record succeeded" not in output, "record incorrectly enabled"

    # try it again to see if we can replay
    output = subprocess.check_output(
        args,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
        timeout=120,
    )

    if debug:
        if record and replay:
            assert "replay succeeded" in output, "replay failed"
        elif replay:
            assert "replay failed" in output, "replay succeeded with no trace"
        else:
            assert "replay failed" not in output, "replay incorrectly enabled"
            assert "replay succeeded" not in output, "replay incorrectly enabled"
        if record and replay:
            assert "record failed" not in output, "record still attempted after replay"
            assert "record succeeded" not in output, "record after successful replay"
        elif record:
            assert "record succeeded" in output, "record failed"
        else:
            assert "record failed" not in output, "record incorrectly enabled"
            assert "record succeeded" not in output, "record incorrectly enabled"


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
    strace(["close-on-exec"])

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
    args += ["--", "close-on-exec"]

    output = subprocess.check_output(
        args,
        stderr=subprocess.STDOUT,
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
            assert "record succeeded" in output, "record failed"
        else:
            assert "record failed" not in output, "record incorrectly enabled"
            assert "record succeeded" not in output, "record incorrectly enabled"

    # try it again to see if we can replay
    output = subprocess.check_output(
        args,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
        timeout=120,
    )

    if debug:
        if record and replay:
            assert "replay succeeded" in output, "replay failed"
        elif replay:
            assert "replay failed" in output, "replay succeeded with no trace"
        else:
            assert "replay failed" not in output, "replay incorrectly enabled"
            assert "replay succeeded" not in output, "replay incorrectly enabled"
        if record and replay:
            assert "record failed" not in output, "record still attempted after replay"
            assert "record succeeded" not in output, "record after successful replay"
        elif record:
            assert "record succeeded" in output, "record failed"
        else:
            assert "record failed" not in output, "record incorrectly enabled"
            assert "record succeeded" not in output, "record incorrectly enabled"


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
    strace(["umask-open"], tmp_path)
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
    args += ["--", "umask-open"]

    subprocess.check_call(
        args,
        cwd=tmp_path,
        timeout=120,
    )

    assert foo.exists(), "expected output was not created"
    assert (
        stat.S_IMODE(foo.stat().st_mode) == 0o777
    ), "expected file mode was not applied"

    # try it again to see if we can replay
    foo.unlink()
    subprocess.check_call(
        args,
        cwd=tmp_path,
        timeout=120,
    )

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
    strace(["umask-open2"], tmp_path)
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
    args += ["--", "umask-open2"]

    subprocess.check_call(
        args,
        cwd=tmp_path,
        timeout=120,
    )

    assert foo.exists(), "expected output was not created"
    assert (
        stat.S_IMODE(foo.stat().st_mode) == 0o666
    ), "expected file mode was not applied"

    # try it again to see if we can replay
    foo.unlink()
    subprocess.check_call(
        args,
        cwd=tmp_path,
        timeout=120,
    )

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

    tracee = ["ld-preload-in-child"]
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
    strace(tracee)

    args = [
        "xcache",
        "--debug",
        f"--dir={tmp_path}/database",
        "--read-write",
        "--",
    ] + tracee

    output = subprocess.check_output(
        args,
        stderr=subprocess.STDOUT,
        text=True,
        timeout=120,
    )

    assert "record succeeded" in output, "record failed"

    # if tracing successfully propagated libxcache-spy to the child, we should
    # perceive the child’s `sysconf`
    assert "called sysconf(30 /* _SC_PAGESIZE */)" in output, "sysconf in child unseen"


def test_exec_sysconf(tmp_path: Path):
    """
    do exec-ed children correctly pick up libxcache-spy?

    When exec-ing, ones address space is replaced. This means getting a new copy of
    libxcache-spy. This test checks whether this new spy correctly starts up and
    observes the new process’ actions.
    """

    tracee = ["my-execvp", "my-sysconf", "_SC_PAGESIZE"]

    # First, `strace` the process we are about to test. If the test fails, the
    # `strace` output will show what syscalls it made which may aid debugging.
    # This is useful when, e.g., running on a new kernel where the dynamic
    # loader or libc makes unanticipated syscalls.
    strace(tracee)

    args = [
        "xcache",
        "--debug",
        f"--dir={tmp_path}/database",
        "--read-write",
        "--",
    ] + tracee

    output = subprocess.check_output(
        args,
        stderr=subprocess.STDOUT,
        text=True,
        timeout=120,
    )

    assert "record succeeded" in output, "record failed"

    # if tracing successfully propagated libxcache-spy to the child, we should
    # perceive the child’s `sysconf`
    assert "called sysconf(30 /* _SC_PAGESIZE */)" in output, "sysconf in child unseen"


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
    p = subprocess.run(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=tmp_path,
        check=True,
        text=True,
        env=env,
    )

    assert "record succeeded" in p.stdout, "record failed"

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
    p = subprocess.run(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=tmp_path,
        check=True,
        text=True,
        env=env,
    )

    # replay should be dependent on the environment variable matching
    if export1 == export2:
        assert "replay succeeded" in p.stdout, "replay failed"
    else:
        assert "replay succeeded" not in p.stdout, "replay incorrectly succeeded"

    # if `FOO` was set, we should have written the output file
    if export2:
        assert foo.exists(), "output file not written"
        assert (
            foo.read_text(encoding="utf-8") == "hello world"
        ), "incorrect content written"
    else:
        assert not foo.exists(), "output file written"


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
    p = subprocess.run(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=tmp_path,
        check=True,
        text=True,
        env=env,
    )

    assert "record succeeded" in p.stdout, "record failed"

    # set the environment variable differently for a second run
    env = os.environ.copy()
    env["FOO"] = "qux"

    # run the command a second time
    p = subprocess.run(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=tmp_path,
        check=True,
        text=True,
        env=env,
    )

    # replay should be independent of the environment variable
    assert "replay succeeded" in p.stdout, "replay failed"


@pytest.mark.xfail(strict=True)
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
    p = subprocess.run(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=tmp_path,
        check=True,
        text=True,
        env=env,
    )

    assert "record succeeded" in p.stdout, "record failed"

    # set the environment variable differently for a second run
    env = os.environ.copy()
    env["FOO"] = "qux"

    # run the command a second time
    p = subprocess.run(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=tmp_path,
        check=True,
        text=True,
        env=env,
    )

    # replay should be independent of the environment variable
    assert "replay succeeded" in p.stdout, "replay failed"
