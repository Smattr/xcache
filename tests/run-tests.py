#!/usr/bin/env python3

from pathlib import Path
import subprocess
import tempfile
import unittest

class Tests(unittest.TestCase):

  def test_uncached_cat(self):
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
      self.assertEqual(output, "hello world\n");

  def test_uncached_echo(self):
    """
    running echo with no prior cache should result in the correct output
    """
    output = subprocess.check_output(["xcache", "--", "my-echo", "hello",
                                      "world"], universal_newlines=True)
    self.assertEqual(output, "hello world\n");

if __name__ == "__main__":
  unittest.main()
