#all we need are the bboxes
import os
import subprocess
import sys
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from itertools import cycle, islice
import numpy as np
import sqlite3
from matplotlib.colors import ListedColormap


class bbox:
    def __init__(self,min_x,min_y,max_x,max_y):
        self.min_x = min_x
        self.max_x = max_x
        self.min_y = min_y
        self.max_y = max_y

    def intersect_circle(self,rad,x,y):
        dx = max(self.min_x,min(self.max_x,x))
        dy = max(self.min_y,min(self.max_y,y))
        return int(((x-dx)**2+(y-dy)**2<rad**2))

    def normalise(self, world:bbox):
        self.min_x = (self.min_x-world.min_x)/(world.max_x-world.min_x)
        self.min_y = (self.min_y-world.min_y)/(world.max_y-world.min_y)
        self.max_x = (self.max_x-world.min_x)/(world.max_x-world.min_x)
        self.max_y = (self.max_y-world.min_y)/(world.max_y-world.min_y)


def graph_m2(filename, tab, col, sam, min_leaf):
    if (1 == 2):
        pass
    else:
        strings = []
        r,w = os.pipe()
        try:
            subprocess.Popen(["bin/draw_bboxes",filename,tab,col,str(sam),str(min_leaf),str(w)],pass_fds=(w,))

            os.close(w)
            with os.fdopen(r, 'r') as secret_stream:
                data = secret_stream.read()
                strings.append(data)
                    #if we redirect this to an output file, it gets printed at the end
                    #do processing or something, save to local "time" variable
        finally:
            if 'w' in locals():
                try: os.close(w)
                except: pass

        #this'll be a single string!

        lines = strings[0].split("\n")
        bboxes = []
        for i in range(len(lines)-1):
            bboxes.append(lines[i].split(","))
    #now we get the bboxes from another file somehow
    con = sqlite3.connect(filename)
    cur = con.cursor()
    res = cur.execute("SELECT extent_min_x, extent_min_y, extent_max_x, extent_max_y FROM geometry_columns_statistics WHERE f_table_name = ? AND f_geometry_column = ? AND extent_max_y NOT NULL",(tab,col))
    worlds = res.fetchall()
    world = bbox(worlds[0][0], worlds[0][1], worlds[0][2], worlds[0][3])
    print(worlds)
    con.close()
    print(bboxes)
    b2 = []
    for i in bboxes:
        b3 = bbox(float(i[0]),float(i[1]),float(i[2]),float(i[3]))
        b3.normalise(world)
        b2.append(b3)
    #we need to convert into a nicer format

    
    #now we merge them into a new set of bboxes that have normal split edges

        #graphing time, i suppose!
    fig,ax=plt.subplots()
    """
    ax.set_xlim(min(float(t[0]) for t in bboxes)-1, max(float(t[2]) for t in bboxes)+1)
    ax.set_ylim(min(float(t[1]) for t in bboxes)-1, max(float(t[3]) for t in bboxes)+1)
    for i in bboxes:
        rectangle = patches.Rectangle((float(i[0]), float(i[1])), float(i[2])-float(i[0]), float(i[3])-float(i[1]), linewidth=2, edgecolor='blue', facecolor='none')
        ax.add_patch(rectangle)
    ax.set_aspect('equal', 'box')
    plt.show()
    """
    ax.set_title("Range query with the median grid method")
    centre = np.array([0.5,0.5])
    rad = 0.2
    ax.set_xlim(0,1)
    ax.set_ylim(0,1)
    colours = [(1,0.9,1),(0.9,1,0.9)]
    for i in b2:
        col = colours[i.intersect_circle(rad,centre[0],centre[1])]
        rectangle = patches.Rectangle((i.min_x, i.min_y), i.max_x-i.min_x, i.max_y-i.min_y, linewidth=2, edgecolor='black', facecolor=col)
        ax.add_patch(rectangle)
        """
    points = np.random.rand(100, 2)
    is_inside = np.sum((points - centre)**2, axis=1) < rad**2
    colours = [(0.6,0.3,0.3),(0.3,0.6,0.3)]

    colmap = ListedColormap(colours)
    ax.scatter(points[:, 0], points[:, 1], c=is_inside, cmap=colmap,linewidth=0.5,label='Spatial Points')  
"""
    ax.set_aspect('equal', 'box')
    query_circle = plt.Circle(centre, rad, color='mediumseagreen', fill=False, linestyle='--', linewidth=2, label='Query Range')
    ax.add_patch(query_circle)
    plt.savefig("img/m2.png",bbox_inches="tight")
    plt.show()


    #well, we have another way of drawing the boxes now
    #we get some really weird boxes this way, so maybe change it for the graphical demo

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: draw3.py <filename>")
        exit()
    name = sys.argv[1]

    graph_m2("data/"+name+".sqlite", name, "cent", 4096,256)

