#generates random and gaussian datasets or something

import subprocess
import os
import sys
if __name__ == "__main__":
    os.remove("data/gau.sqlite")
    os.remove("data/ran.sqlite")
    subprocess.run(["bin/make_ug","100000"])
    subprocess.run(["bin/getpoints", "data/ran.sqlite", "ran", "cent", "data/ran.dat"])
    subprocess.run(["bin/getpoints", "data/gau.sqlite", "gau", "cent", "data/gau.dat"])
    print("done!")
