#!/usr/bin/env python
# This is topmose file of this excercise.
import sys
import os
os.environ['LD_LIBRARY_PATH'] = '.'
sys.path.append(".")

import cymoose

if __name__ == "__main__":
    c = cymoose.PyShell()
    c.create("Neutral", None, "comp", 1)
