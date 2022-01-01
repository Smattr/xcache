#!/usr/bin/env python3

"""
Xcache test suite
"""

# pylint: disable=bad-indentation,invalid-name

from pathlib import Path
import subprocess
import tempfile

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
    output = subprocess.check_output(argv, universal_newlines=True)
    assert output == "hello world\n"

def test_uncached_echo():
  """
  running echo with no prior cache should result in the correct output
  """
  with tempfile.TemporaryDirectory() as tmp:
    argv = ["xcache", "--dir", tmp, "--", "my-echo", "hello", "world"]
    output = subprocess.check_output(argv, universal_newlines=True)
    assert output == "hello world\n"
