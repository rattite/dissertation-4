import subprocess
import os
import sys


#wow what the helliante???????

for i in range(2):
    for j in range(4):
        p1 = subprocess.run(["bin/reset","data/"+sys.argv[1]+".sqlite",sys.argv[1]])
        print("reset!")
        subprocess.run(["bin/cursetest4", sys.argv[1],str(j),str(i)])
print("cool!\n")
 
