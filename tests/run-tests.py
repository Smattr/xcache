#!/usr/bin/env python3

import subprocess
import unittest

class Tests(unittest.TestCase):

  def test_uncached_echo(self):
    """
    running echo with no prior cache should result in the correct output
    """
    output = subprocess.check_output(["xcache", "--", "my-echo", "hello",
                                      "world"], universal_newlines=True)
    self.assertEqual(output, "hello world\n");

if __name__ == "__main__":
  unittest.main()
