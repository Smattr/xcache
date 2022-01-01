#!/usr/bin/env python3

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
    with open(p, "wt") as f:
      f.write("hello world\n")

    # cat this file
    output = subprocess.check_output(["xcache", "--", "cat", p],
                                     universal_newlines=True)
    assert output == "hello world\n"

def test_uncached_echo():
  """
  running echo with no prior cache should result in the correct output
  """
  output = subprocess.check_output(["xcache", "--", "my-echo", "hello",
                                    "world"], universal_newlines=True)
  assert output == "hello world\n"
