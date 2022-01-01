#!/usr/bin/env python3

"""
Xcache test suite
"""

# pylint: disable=bad-indentation,invalid-name

from pathlib import Path
import subprocess
import tempfile
import pytest

# debug text indicating various operations succeeded or failed
# (see ../xcache/src/main.c)
RECORD_GOOD = "[DEBUG] record succeeded\n"
RECORD_BAD = "[DEBUG] record failed\n"
REPLAY_GOOD = "[DEBUG] replay succeeded\n"
REPLAY_BAD = "[DEBUG] replay failed\n"

@pytest.mark.parametrize("record_enabled", (False, True))
@pytest.mark.parametrize("replay_enabled", (False, True))
def test_echo(replay_enabled: bool, record_enabled: bool):
  """
  echo simple text
  """
  with tempfile.TemporaryDirectory() as tmp:

    # echo some text
    argv = ["xcache", "--dir", tmp, "--debug"]
    if replay_enabled:
      argv += ["--enable-replay"]
    else:
      argv += ["--disable-replay"]
    if record_enabled:
      argv += ["--enable-record"]
    else:
      argv += ["--disable-record"]
    argv += ["--", "my-echo", "hello", "world"]
    output = subprocess.check_output(argv, stderr=subprocess.STDOUT,
                                     universal_newlines=True, timeout=120)
    assert "hello world\n" in output

    # the cache was empty when this ran, so it should have missed
    if replay_enabled:
      assert REPLAY_BAD in output, "incorrect cache hit"
    else:
      assert REPLAY_BAD not in output, "replay incorrectly enabled"
      assert REPLAY_GOOD not in output, "replay incorrectly enabled"

    # recording should have worked for this, if enabled
    if record_enabled:
      assert RECORD_GOOD in output, "failed to write cache"
    else:
      assert RECORD_BAD not in output, "record incorrectly enabled"
      assert RECORD_GOOD not in output, "record incorrectly enabled"

    # try this again
    output = subprocess.check_output(argv, stderr=subprocess.STDOUT,
                                     universal_newlines=True, timeout=120)
    assert "hello world\n" in output

    # if recording was enabled, we should have recorded this in the first pass,
    # (if replay is enabled) replaying should have succeeded
    if replay_enabled:
      if record_enabled:
        assert REPLAY_GOOD in output, "failed expected replay"
      else:
        assert REPLAY_BAD in output, "replayed non-existent recording?"
    else:
      assert REPLAY_BAD not in output, "replay incorrectly enabled"
      assert REPLAY_GOOD not in output, "replay incorrectly enabled"

    # we should not have attempted to re-record a trace for this, unless we were
    # operating write-only
    if not replay_enabled and record_enabled:
      assert RECORD_GOOD in output, "failed to re-record"
    else:
      assert RECORD_BAD not in output, "redundant record attempted"
      assert RECORD_GOOD not in output, "redundant recording made"

@pytest.mark.parametrize("record_enabled", (False, True))
@pytest.mark.parametrize("replay_enabled", (False, True))
def test_cat(replay_enabled: bool, record_enabled: bool):
  """
  cat a short file
  """
  with tempfile.TemporaryDirectory() as tmp:

    # write a short file
    p = Path(tmp) / "test.txt"
    with open(p, "wt", encoding="utf-8") as f:
      f.write("hello world\n")

    # cat this file
    argv = ["xcache", "--dir", tmp, "--debug"]
    if replay_enabled:
      argv += ["--enable-replay"]
    else:
      argv += ["--disable-replay"]
    if record_enabled:
      argv += ["--enable-record"]
    else:
      argv += ["--disable-record"]
    argv += ["--", "my-cat", p]
    output = subprocess.check_output(argv, stderr=subprocess.STDOUT,
                                     universal_newlines=True, timeout=120)
    assert "hello world\n" in output

    # the cache was empty when this ran, so it should have missed
    if replay_enabled:
      assert REPLAY_BAD in output, "incorrect cache hit"
    else:
      assert REPLAY_BAD not in output, "replay incorrectly enabled"
      assert REPLAY_GOOD not in output, "replay incorrectly enabled"

    # recording should have worked for this, if enabled
    if record_enabled:
      assert RECORD_GOOD in output, "failed to write cache"
    else:
      assert RECORD_BAD not in output, "record incorrectly enabled"
      assert RECORD_GOOD not in output, "record incorrectly enabled"

    # try this again
    output = subprocess.check_output(argv, stderr=subprocess.STDOUT,
                                     universal_newlines=True, timeout=120)
    assert "hello world\n" in output

    # if recording was enabled, we should have recorded this in the first pass,
    # (if replay is enabled) replaying should have succeeded
    if replay_enabled:
      if record_enabled:
        assert REPLAY_GOOD in output, "failed expected replay"
      else:
        assert REPLAY_BAD in output, "replayed non-existent recording?"
    else:
      assert REPLAY_BAD not in output, "replay incorrectly enabled"
      assert REPLAY_GOOD not in output, "replay incorrectly enabled"

    # we should not have attempted to re-record a trace for this, unless we were
    # operating write-only
    if not replay_enabled and record_enabled:
      assert RECORD_GOOD in output, "failed to re-record"
    else:
      assert RECORD_BAD not in output, "redundant record attempted"
      assert RECORD_GOOD not in output, "redundant recording made"

def test_uncached_cat():
  """
  running cat with no prior cache should result in the correct output
  """
  with tempfile.TemporaryDirectory() as tmp:

    # write some content to a test file
    p = Path(tmp) / "test.txt"
    with open(p, "wt", encoding="utf-8") as f:
      f.write("hello world\n")

    # cat this file
    argv = ["xcache", "--dir", tmp, "--", "my-cat", p]
    output = subprocess.check_output(argv, universal_newlines=True, timeout=120)
    assert output == "hello world\n"

def test_uncached_echo():
  """
  running echo with no prior cache should result in the correct output
  """
  with tempfile.TemporaryDirectory() as tmp:
    argv = ["xcache", "--dir", tmp, "--", "my-echo", "hello", "world"]
    output = subprocess.check_output(argv, universal_newlines=True, timeout=120)
    assert output == "hello world\n"
