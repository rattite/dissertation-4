from run_tests import test_case
import os
import sys

#converts a case file to a LaTeX table
#this is an incredibly stupid idea, and i have no idea why i'm doing this
global suff
suff = [".allind", ".all1", ".all2", ".all3", ".best"]
#ok, think about it
#we'll need records for p1, p2, time, hit and stddev
if __name__ == "__main__":
    directory = os.fsencode(sys.argv[1]) #eg. data/large
    for file in os.listdir(directory):
        filename = os.fsdecode(file)
        for s in suff:
            if filename.endswith(s):
                cases = test_case.deserialise_list(sys.argv[1]+filename)
                test_case.serialise_list(sys.argv[1]+filename,cases)
                print("converted!")
                break

