#all we need are the bboxes
import os
import subprocess
import sys
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from itertools import cycle, islice
import numpy as np
from shapely.geometry import box
import sqlite3
from matplotlib.colors import ListedColormap
import geopandas as gpd
from matplotlib.patches import PathPatch
from shapely.geometry import LineString
from matplotlib.path import Path
from math import sqrt



def draw_outline(geom, color, lw, ax,z):
    if geom.is_empty:
        return

    geoms = getattr(geom, "geoms", [geom])

    for g in geoms:
        x, y = g.xy
        ax.plot(x, y, color=color, linewidth=lw, zorder=z)
def poly_to_path(poly):
            # This is a simplified converter for a single polygon
    if poly.geom_type == 'Polygon':
        return Path(np.asarray(poly.exterior.coords))
    elif poly.geom_type == 'MultiPolygon':
                # Handle multipolygons by concatenating paths
        all_coords = [np.asarray(p.exterior.coords) for p in poly.geoms]
        return Path(np.concatenate(all_coords))
    return None
def draw_geom(geom, color, lw, z,ax):
    if geom.is_empty:
        return
    if geom.geom_type == "LineString":
        x, y = geom.xy
        ax.plot(x, y, color=color, linewidth=lw, zorder=z)
    elif geom.geom_type == "MultiLineString":
        for g in geom.geoms:
            x, y = g.xy
            ax.plot(x, y, color=color, linewidth=lw, zorder=z)

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
    def fix(self,world,offset):
        print(self.max_x)
        if abs((self.min_x-world.min_x)) < 3:
            self.min_x = self.min_x - offset
            print("ok1")
        print(world.max_x)
        if abs((self.max_x-world.max_x)) < 3:
            self.max_x = self.max_x + offset
            print("ok2")
        if abs((self.min_y-world.min_y)) < 3:
            self.min_y = self.min_y - offset
            print("ok3")
        if abs((self.max_y-world.max_y)) < 3:
            self.max_y = self.max_y + offset
            print("ok4")


def graph_m3_new(filename,tab,col,sam,min_leaf,min_leaf_base,clus,shapefile=None):

    #gets all bboxes
    if (1 == 2):
        pass
    else:
        strings = []
        r,w = os.pipe()
        try:
            subprocess.Popen(["bin/draw_3",filename,tab,col,str(sam),str(min_leaf),str(min_leaf_base),clus,str(w)],pass_fds=(w,))

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

    #gets world bbox
    con = sqlite3.connect(filename)
    cur = con.cursor()
    res = cur.execute("SELECT extent_min_x, extent_min_y, extent_max_x, extent_max_y FROM geometry_columns_statistics WHERE f_table_name = ? AND f_geometry_column = ? AND extent_max_y NOT NULL",(tab,col))
    worlds = res.fetchall()
    world = bbox(worlds[0][0], worlds[0][1], worlds[0][2], worlds[0][3])
    con.close()
    strings = strings[0].split("\n")


    #separates bboxes into strata
    point = 0
    boxes = {0:{}}
    for i in strings[:-1]:
        if len(i) == 1:
            point = int(i)
            boxes[point] = {}
        else:
            l = i.split(",")
            print(l)
            depth = int(l[0])
            b = bbox(float(l[1]),float(l[2]),float(l[3]),float(l[4]))
            if depth not in boxes[point]:
                boxes[point][depth] = [b]
            else:
                boxes[point][depth].append(b)
    print(strings)

    #matplotlib stuff
    fig,ax=plt.subplots(figsize=(8,8))
    ax.axis('off')
    ax.get_xaxis().set_visible(False)
    ax.get_yaxis().set_visible(False)
    centre = np.array([-420000,6540000])
    rad = 20000
    colours = [(1,0.9,1),(0.9,1,0.9)]
    clip_poly = None
    if shapefile:
        gdf = gpd.read_file(shapefile)
        gdf = gdf.to_crs(epsg=3857)

        merged_poly = gdf.unary_union
        poly_path = poly_to_path(merged_poly)


    #time to actually draw the stuff
    #this time we're doing the same thing with each cluster
    if shapefile:
        outline = merged_poly.boundary
        draw_outline(outline, 'black', 3, ax,9001)
        ax.set_aspect('equal', 'box')

        for c, clus in boxes.items():
            maxdep = max(clus.keys())
            baselw = 1
            if c == 0:
                baselw = 3
            print(clus)
            #we have a dict and such
            for dep in sorted(clus.keys()):
                for i in clus[dep]:
                    i.fix(world,9001)
                    col = colours[i.intersect_circle(rad,centre[0],centre[1])]
                    print(col)
                    rect_geom = box(i.min_x, i.min_y, i.max_x, i.max_y)
                    if shapefile:
                        clipped_geom = rect_geom.intersection(merged_poly)
                        if not clipped_geom.is_empty:
                            gpd.GeoSeries([clipped_geom]).plot(ax=ax, facecolor=col, edgecolor='none', linewidth=0,zorder=100*c+1)        
                    else:
                        rect = patches.Rectangle((i.min_x, i.min_y),i.max_x - i.min_x,i.max_y - i.min_y,linewidth=0,facecolor=col, color='none',zorder=100*c+1)
                        ax.add_patch(rect) 

            for dep in sorted(clus.keys()):  # 0 → 2
                for i in clus[dep]:
                    edges = [
                        LineString([(i.min_x, i.min_y), (i.max_x, i.min_y)]),
                        LineString([(i.max_x, i.min_y), (i.max_x, i.max_y)]),
                        LineString([(i.max_x, i.max_y), (i.min_x, i.max_y)]),
                        LineString([(i.min_x, i.max_y), (i.min_x, i.min_y)])
                    ]
                    for edge in edges:
                        inside = edge.intersection(merged_poly)
                        outside = edge.difference(merged_poly)
                        draw_geom(inside, 'dimgray', 2*(maxdep-dep)+baselw, 10+dep+100*c,ax)
                        draw_geom(outside, 'silver', 2*(maxdep-dep)+baselw, 10+dep+100*c,ax)
    query_circle = plt.Circle(centre, rad, color='black', fill=False, linestyle='--', linewidth=3, label='Query Range',zorder=6767)
    ax.scatter(centre[0],centre[1], color="black",edgecolor="black",s=100,marker="x",zorder=6767)

    ax.add_patch(query_circle)
    plt.savefig("img/m3.png",bbox_inches="tight",dpi=300)
    plt.show()



def graph_m3(filename, tab, col, sam, min_leaf,min_leaf_base,clus):
    if (1 == 2):
        pass
    else:
        strings = []
        r,w = os.pipe()
        try:
            subprocess.Popen(["bin/draw_3",filename,tab,col,str(sam),str(min_leaf),str(min_leaf_base),clus,str(w)],pass_fds=(w,))

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
    con.close()
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
    centre = np.array([0.5,0.5])
    rad = 0.2
    ax.set_xlim(0,1)
    ax.set_ylim(0,1)
    colours = [(1,0.9,1),(0.9,1,0.9)]
    for i in b2:
        col = colours[i.intersect_circle(rad,centre[0],centre[1])]
        rectangle = patches.Rectangle((i.min_x, i.min_y), i.max_x-i.min_x, i.max_y-i.min_y, linewidth=2, edgecolor='black', facecolor=col)
        ax.add_patch(rectangle)
    ax.set_title("Range query with the median grid clustering method")

    """
    points = np.random.rand(100, 2)
    colours = [(0.6,0.3,0.3),(0.3,0.6,0.3)]
    is_inside = np.sum((points - centre)**2, axis=1) < rad**2
    colmap = ListedColormap(colours)
    ax.scatter(points[:, 0], points[:, 1], c=is_inside, cmap=colmap,linewidth=0.5,label='Spatial Points')  
    """
    ax.set_aspect('equal', 'box')
    query_circle = plt.Circle(centre, rad, color='mediumseagreen', fill=False, linestyle='--', linewidth=2, label='Query Range')
    ax.add_patch(query_circle)
    plt.savefig("img/m3.png",bbox_inches="tight")
    plt.show()


    #well, we have another way of drawing the boxes now
    #we get some really weird boxes this way, so maybe change it for the graphical demo

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: draw3.py <filename>")
        exit()
    name = sys.argv[1]
    if len(sys.argv) == 2:
        graph_m3_new("data/"+name+".sqlite", name, "cent", 4096,256,256,"data/"+name+".lizard")
    else:
        graph_m3_new("data/"+name+".sqlite", name, "cent", 4096,256,256,"data/"+name+".lizard",sys.argv[2])



