#all we need are the bboxes
import os
import subprocess
import sys
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from itertools import cycle, islice
import numpy as np
import geopandas as gpd
from shapely.geometry import box, MultiPolygon
import sqlite3
import math
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

def main(name,shapefile=None):

    con = sqlite3.connect("data/"+name+".sqlite")
    cur = con.cursor()
    res = cur.execute("SELECT start FROM large_parts_ind")
    startpoints = res.fetchall()[1:]
    starts = []
    for i in range(0,len(startpoints)):
        starts.append(startpoints[i][0])
    starts.append(9001)
    print(starts)

    res2 = cur.execute("SELECT extent_min_x, extent_min_y, extent_max_x, extent_max_y FROM geometry_columns_statistics WHERE f_table_name = ? and f_geometry_column = 'cent'",[name])
    b = res2.fetchone()
    min_x = b[0]
    min_y = b[1]
    x_len = b[2]-b[0]
    y_len = b[3]-b[1]
    print(min_x)
    print(min_y)
    print(x_len)
    print(y_len)
    cur.close()
    #fig,ax=plt.subplots(facecolor="cornflowerblue")
    fig,ax=plt.subplots(facecolor="lightgray",figsize=(10,10))

    ax.set_xlim(b[0]-2000, b[2]+2000)
    ax.set_ylim(b[1]-2000,b[3]+2000)

    mask_geom = None
    if shapefile:
        gdf = gpd.read_file(shapefile)
        gdf = gdf.to_crs(epsg=3857)
        scol = "palegoldenrod"
        #gdf.plot(ax=ax, color=scol, edgecolor="black")
        # Dissolve all shapes into a single geometry for easier checking
        mask_geom = gdf.unary_union

    #we have 4 components:
    #each partition in a different colour, with an exception if the partition intersects with the query area
    #query circle

    centre = np.array([0.3,0.6])
    radius = 0.2
    ax.set_aspect('equal','box')
    ax.axis('off')
    order = 4
    xstep = x_len / (2**order)
    ystep = y_len / (2**order)
    print(xstep,ystep)
    curve = hilbert_curve(order, 'u')
    curve = np.array(curve)
    start_pointer = 0
    cmap = plt.get_cmap("vanimo")
    print(len(starts))
    space = np.linspace(0,1,len(starts))
    colours = cmap(3*space**2-2*space**3)




    x = np.linspace(-1, 1, len(starts))
    warped_x = np.sign(x) * np.abs(x)**(1/1.5)
    sample_indices = (warped_x + 1) / 2
    cmap = plt.get_cmap("vanimo")
    colours = cmap(sample_indices)

    ccx = np.array([min_x+xstep/2+np.sum(curve[:i][:,0], 0)*xstep for i in range(len(curve)+1)])
    ccy = np.array([min_y+ystep/2+np.sum(curve[:i][:,1], 0)*ystep for i in range(len(curve)+1)])
    ccz = np.stack((ccx,ccy),axis=1)
    #print(ccz) this is probably fine

    #note: we have to scale the hilbert data to the world bbox, as that's where it's calculated from or something:
    #and the shapefile doesn't exactly match up
    part = []
    point = 0
    p = np.array([0,0])
    print(starts)

    black = [[0] * 16 for _ in range(16)]    #we index by rows, then columns
    #eg. black[0][1] = (1,0)
    x_pt = 0
    y_pt = 0
    for i in range(4**order):
        #print(x_pt,y_pt) these are correct
        if starts[point] == i:
            point += 1
        black[y_pt][x_pt] = point
        if i == len(curve):
            break
        x_pt += curve[i][0]
        y_pt += curve[i][1]
    point = 0
    for i in range(4**order):
        xlow = min_x+p[0]*xstep
        ylow = min_y+p[1]*ystep
        xup = xlow+xstep
        yup = ylow+ystep

        cell_polygon = box(xlow, ylow,xup,yup)
        should_draw = True
        if mask_geom:
            should_draw = cell_polygon.intersects(mask_geom)
        if starts[point] == i:
            point += 1
        part.append(point)
        if (1==1):
            try:
                if p[0] == 15 or black[p[1]][p[0]]!=black[p[1]][p[0]+1]:
                    ax.plot((xup,xup),(ylow,yup),c="black",linewidth=2)
            except IndexError:
                pass
            try:
                if p[0] == 0 or black[p[1]][p[0]]!=black[p[1]][p[0]-1]:
                    ax.plot((xlow,xlow),(ylow,yup),c="black",linewidth=2)
            except IndexError:
                pass
            try:
                if p[1] == 0 or black[p[1]][p[0]]!=black[p[1]-1][p[0]]:
                    ax.plot((xlow,xup),(ylow,ylow),c="black",linewidth=2)
            except IndexError:
                pass
            try:
                if p[1] == 15 or black[p[1]][p[0]]!=black[p[1]+1][p[0]]:
                    ax.plot((xlow,xup),(yup,yup),c="black",linewidth=2)
            except IndexError:
                pass
        if should_draw:

            rect = patches.Rectangle((xlow,ylow), xstep, ystep, linewidth=0, facecolor=colours[point%16], alpha=1)
            ax.add_patch(rect)
        #indent this if you're lam
        ax.plot(ccz[i:i+2][:,0],ccz[i:i+2][:,1],"w-",alpha=0.8)
        if (i == len(curve)):
            break
        p += curve[i]


    plt.savefig("img/m1_"+name+".png",bbox_inches="tight",dpi=300)
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: draw3.py <name>")
        exit()
    name = sys.argv[1]
    if len(sys.argv) == 2:
        main(name)
    else:
        main(name,sys.argv[2])

