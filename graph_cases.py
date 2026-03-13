import matplotlib.pyplot as plt
import matplotlib.patches as patches
from itertools import cycle, islice
from matplotlib.colors import ListedColormap
import sys
import sqlite3
import numpy as np



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

class bbox:
    def __init__(self,minx,miny,maxx,maxy):
        self.min_x = minx
        self.min_y = miny
        self.max_x = maxx
        self.max_y = maxy

def get_db_boundaries(filename:str) -> bbox:
    #Same as the c function. We need this for query generation!
    con = sqlite3.connect(filename)
    cur = con.cursor()
    tab = filename.split(".")[0]
    res = cur.execute("SELECT extent_min_x, extent_min_y, extent_max_x, extent_max_y FROM geometry_columns_statistics WHERE f_table_name = ? AND f_geometry_column = 'cent'",[tab])
    b = res.fetchone()
    print(b)
    out = bbox(b[0],b[1],b[2],b[3])
    con.close()
    print("COMPLETED!")
    return out

def get_points(filename,samp_size):
    f = open(filename,"r")
    nums = []
    while True:
        k = f.readline()
        if not k:
            break
        a = k.split(",")
        nums.append([float(a[0]), float(a[1])])
    n = np.array(nums)
    a = np.array(range(len(n)))
    b = np.random.choice(a,samp_size,replace=False)
    return n[b]

def normalise(p, b:bbox):
    q = [0,0]
    q[0] = (p[0]-b.min_x)/(b.max_x-b.min_x)
    q[1] = (p[1]-b.min_y)/(b.max_y-b.min_y)
    return q

def prep_points(db,datafile,samp):
    points = get_points(datafile,samp)
    bounds = get_db_boundaries(db)
    for i in range(len(points)):
        points[i] = normalise(points[i], bounds)
    return points



def draw_range_query(points):
    #basically just draws the points with a circle and a radius
    #draws them in red if they're in, black if not
    centre = np.array([0.5,0.5])
    if len(points) == 0:
        points = np.random.rand(100, 2)
    radius = 0.2
    fig,ax=plt.subplots()
    ax.set_title("Example of a range query on spatial data")
    ax.set_aspect('equal')
    ax.set_xlim(0,1)
    ax.set_ylim(0,1)
    colours = ['orchid','lightseagreen']
    is_inside = np.sum((points - centre)**2, axis=1) < radius**2
    colmap = ListedColormap(colours)
    ax.scatter(points[:, 0], points[:, 1], c=is_inside, cmap=colmap,edgecolor='none',linewidth=0.5,label='Spatial Points')  
    query_circle = plt.Circle(centre, radius, color='mediumseagreen', fill=False, linestyle='--', linewidth=2, label='Query Range')
    ax.add_patch(query_circle)
    ax.grid(True, alpha=0.3)
    plt.savefig("img/r.png",bbox_inches="tight")


    plt.show()


def rect_circle(min_x, max_x, min_y, max_y, c_x, c_y, rad):
    dx = max(min_x,min(c_x,max_x))
    dy = max(min_y,min(c_y,max_y))
    return ((c_x-dx)**2+(c_y-dy)**2<rad*rad)

def draw_hilbert_curve(order, ax):
    ax.set_aspect('equal','box')
    ax.set_title("n = "+str(order))
    ax.set_ylim(0,1)
    ax.set_xlim(0,1)
    ax.axis('off')
    rect = patches.Rectangle((0, 0), 1, 1, linewidth=2, edgecolor='black', facecolor='none', zorder=10)
    ax.add_patch(rect)
    curve = hilbert_curve(order, 'u')
    curve = np.array(curve)
    cc = np.array([1/(2**(order+1))+np.sum(curve[:i], 0)/(2**order) for i in range(len(curve)+1)])
    for i in range(len(cc)-1):
        ax.plot(cc[i:i+2][:,0],cc[i:i+2][:,1],color='mediumseagreen',linestyle="-")
    """
    if order < 4:
        square_size = 1/(2**order)
        for i in range(2**order):
                for j in range(2**order):
                    extent = [i*square_size, (i+1)*square_size, j*square_size, (j+1)*square_size]
                    y, x = np.ogrid[-1:1:10j, -1:1:10j]
                    mask = np.exp(-(x**2 + y**2) * 2) 
                    
                    ax.imshow(mask, extent=extent, cmap='Blues', alpha=0.3, origin='lower', interpolation='bilinear')   
    """
    plt.savefig("img/hil.png",bbox_inches="tight")

def draw_single_hilbert(order):
    fig, ax = plt.subplots()
    draw_hilbert_curve(order,ax)
    plt.show()

def in_range(x,y,px,py,rad):
    if ((x-px)**2+(y-py)**2 < rad**2):
        return 1
    return 0

def graph_hranges():
    fig, ax = plt.subplots()
    order = 4
    draw_hilbert_curve(order,ax)
    ax.set_title("Example of a Hilbert range query")
    centre = np.array([0.3,0.6])
    radius = 0.2

    curve = hilbert_curve(order, 'u')
    curve = np.array(curve)
    colours = ['orchid','lightseagreen',(1,0.9,1),(0.9,1,0.9)]
    col = []
    sl = 1/(2**order)
    cc = np.array([sl/2+np.sum(curve[:i], 0)*sl for i in range(len(curve)+1)])
    #gets colours
    for i in range(len(cc)):
        col.append(in_range(centre[0],centre[1],cc[i][0],cc[i][1],radius))
    for i in range(len(cc)-1):
        ax.plot(cc[i:i+2][:,0],cc[i:i+2][:,1],"-",c=colours[col[i]|col[i+1]],alpha=0.8)
        p = np.array([0,0])
        if col[i]^col[i+1] == 1:
            #draw an annotation
            plt.annotate(str(i),(cc[i][0]+sl/6,cc[i][1]+sl/6))
            pass

    for i in range(4**order):
        print(p)
        ret = rect_circle(p[0]*sl,p[0]*sl+sl,p[1]*sl,p[1]*sl+sl,centre[0],centre[1],radius)
        rect = patches.Rectangle((p[0]*sl,p[1]*sl), sl,sl, linewidth=0, facecolor=colours[2+ret], alpha=1)
        ax.add_patch(rect)
        if (i == len(curve)):
            break
        p += curve[i]






    """
    for i in range(2**order):
        for j in range(2**order):
            extent = [i*sl, (i+1)*sl, j*sl, (j+1)*sl]
            y, x = np.ogrid[-1:1:10j, -1:1:10j]
            mask = np.exp(-(x**2 + y**2) * 2)         
            ax.imshow(mask, extent=extent, cmap='Blues', alpha=0.3, origin='lower', interpolation='bilinear')   
    """

    query_circle = plt.Circle(centre, radius, color='mediumseagreen', fill=False, linestyle='--', linewidth=2, label='Query Range')
    ax.add_patch(query_circle)
    ax.grid(True, alpha=0.3)
    plt.savefig("img/range.png",bbox_inches="tight")

    plt.show()



def draw_multiple_hilbert(orders,dim):
    fig, axes = plt.subplots(dim[0], dim[1], figsize=(10, 10))
    axes = axes.flatten() # Makes it a 1D list
    for i, order in enumerate(orders):
        print(order)
        draw_hilbert_curve(order, axes[i])

    plt.tight_layout()
    plt.show()



def draw_range_partitions():
    pass


if __name__ == "__main__":
    """
    #draw_multiple_hilbert([1,2,4,6],(2,2))
    if len(sys.argv) < 4:
        print("how to use: (db_name) (data_name) (sample size)")
        quit()
    p = prep_points(sys.argv[1],sys.argv[2],int(sys.argv[3]))
    draw_range_query(p)
    """
    graph_hranges()
