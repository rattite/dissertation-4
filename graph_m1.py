#all we need are the bboxes
import os
import subprocess
import sys
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from itertools import cycle, islice
import numpy as np
import sqlite3
#we just get it from the table and such
base_shape = {'u': [np.array([0, 1]), np.array([1, 0]), np.array([0, -1])],
              'd': [np.array([0, -1]), np.array([-1, 0]), np.array([0, 1])],
              'r': [np.array([1, 0]), np.array([0, 1]), np.array([-1, 0])],
              'l': [np.array([-1, 0]), np.array([0, -1]), np.array([1, 0])]}

def hilbert_curve(order, orientation):
    """
    Recursively creates the structure for a hilbert curve of given order
    """
    if order > 1:
        if orientation == 'u':
            return hilbert_curve(order - 1, 'r') + [np.array([0, 1])] + \
                   hilbert_curve(order - 1, 'u') + [np.array([1, 0])] + \
                   hilbert_curve(order - 1, 'u') + [np.array([0, -1])] + \
                   hilbert_curve(order - 1, 'l')
        elif orientation == 'd':
            return hilbert_curve(order - 1, 'l') + [np.array([0, -1])] + \
                   hilbert_curve(order - 1, 'd') + [np.array([-1, 0])] + \
                   hilbert_curve(order - 1, 'd') + [np.array([0, 1])] + \
                   hilbert_curve(order - 1, 'r')
        elif orientation == 'r':
            return hilbert_curve(order - 1, 'u') + [np.array([1, 0])] + \
                   hilbert_curve(order - 1, 'r') + [np.array([0, 1])] + \
                   hilbert_curve(order - 1, 'r') + [np.array([-1, 0])] + \
                   hilbert_curve(order - 1, 'd')
        else:
            return hilbert_curve(order - 1, 'd') + [np.array([-1, 0])] + \
                   hilbert_curve(order - 1, 'l') + [np.array([0, -1])] + \
                   hilbert_curve(order - 1, 'l') + [np.array([1, 0])] + \
                   hilbert_curve(order - 1, 'u')
    else:
        return base_shape[orientation]

def main(name):
    con = sqlite3.connect("data/"+name+".sqlite")
    cur = con.cursor()
    res = cur.execute("SELECT start FROM large_parts_ind")
    startpoints = res.fetchall()[1:]
    starts = []
    for i in range(0,len(startpoints)):
        starts.append(startpoints[i][0])
    starts.append(9001)
    cur.close()

    #we have 4 components:
    #each partition in a different colour, with an exception if the partition intersects with the query area
    #query circle

    centre = np.array([0.3,0.6])
    radius = 0.2
    fig,ax=plt.subplots()
    ax.set_xlim(0,1)
    ax.set_ylim(0,1)
    ax.set_aspect('equal','box')
    ax.axis('off')
    order = 4
    curve = hilbert_curve(order, 'u')
    curve = np.array(curve)
    print(curve)
    start_pointer = 0
    cmap = plt.get_cmap("PRGn")
    colours = cmap(np.linspace(0, 1, 16))
    cc = np.array([1/(2**(order+1))+np.sum(curve[:i], 0)/(2**order) for i in range(len(curve)+1)])
    for i in range(len(cc)-1):
        ax.plot(cc[i:i+2][:,0],cc[i:i+2][:,1],"w-")
    
    #so apparently this doesn't work...
    part = []
    point = 0
    for i in range(4**order):
        if starts[point] == i:
            point += 1
        part.append(point)
    if order < 9001:
        square_size = 1/(2**order)
        p = np.array([0,0])
        for i in range(4**order):
            rect = patches.Rectangle((p[0]*square_size,p[1]*square_size), square_size, square_size, linewidth=0, facecolor=colours[9*(part[i])%16], alpha=1)
            ax.add_patch(rect)
            if (i == len(curve)):
                break
            p += curve[i]
    plt.savefig("img/m1_"+name+".png",bbox_inches="tight")
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: draw3.py <name>")
        exit()
    name = sys.argv[1]
    main(name)

