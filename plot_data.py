import numpy as np
import numpy.random
import matplotlib.pyplot as plt
import sys
import geopandas as gpd
from matplotlib.patches import PathPatch
from matplotlib.path import Path
def get_mpl_patch(shapely_geom):
    """Converts a Shapely geometry to a Matplotlib PathPatch."""
    from matplotlib.path import Path
    from matplotlib.patches import PathPatch
    
    if shapely_geom.geom_type == 'Polygon':
        paths = [Path(np.array(shapely_geom.exterior.coords))]
    elif shapely_geom.geom_type == 'MultiPolygon':
        # Combine all polygons into one path
        all_coords = []
        for poly in shapely_geom.geoms:
            all_coords.append(np.array(poly.exterior.coords))
        paths = [Path(coords) for coords in all_coords]
        # To clip with a MultiPolygon, we join the paths
        combined_path = Path.make_compound_path(*paths)
        return PathPatch(combined_path)
    
    return PathPatch(paths[0])
def read_data(filename):
    f = open(filename,"r")
    x = []
    y = []
    while True:
        k = f.readline()
        if not k:
            break
        a = k.split(",")
        x.append(float(a[0]))
        y.append(float(a[1]))
    f.close()
    return x,y

if len(sys.argv) not in (2,3):
    print("usage: plot_data.py <filename.dat>")
    exit()

def plot_hexbin(x,y, shapefile=None):

    fig, ax= plt.subplots()
    ax.axis('off')

    if shapefile:
        region = gpd.read_file(shapefile)
        region = region.to_crs(epsg=3857)
        ext = region.total_bounds
        #scol = (14/16, 255/256, 14/16)
        scol = "palegoldenrod"
        region.plot(ax=ax, color=scol, edgecolor="black")
    hb = ax.hexbin(x, y, gridsize=40, cmap='PRGn', mincnt=0, edgecolors='none',bins='log')
    poly_geom = region.geometry.unary_union
    #hb = ax.hexbin(x, y,gridsize=32, bins='log', cmap='PRGn',mincnt=1)
    clip_patch = get_mpl_patch(poly_geom)
    ax.add_patch(clip_patch)
    print(type(clip_patch))
    print(poly_geom.is_valid)
    clip_patch.set_visible(False)
    hb.set_clip_path(clip_patch.get_path(), transform=ax.transData)
    region.boundary.plot(ax=ax, color='black', linewidth=1)
    ax.autoscale_view()    
    cb = fig.colorbar(hb, ax=ax, label='no. of points',shrink=0.8)

    plt.savefig("img/"+sys.argv[1]+"_hex.png",bbox_inches="tight",dpi=256)
    plt.show()

def plot_hist(x,y):
    xlim = min(x), max(x)
    ylim = min(y), max(y)
    fig, ax= plt.subplots()
    ax.set_aspect('equal','box')
    ax.axis('off')

    heatmap, xedges, yedges, im= plt.hist2d(x, y, bins=64,norm="log",cmap="PRGn")
    extent = [xedges[0], xedges[-1], yedges[0], yedges[-1]]

    ax.imshow(heatmap.T, extent=extent, origin='lower')
    plt.savefig("img/"+sys.argv[1]+"_map.png",bbox_inches="tight")
    plt.show()

x,y = read_data("data/"+sys.argv[1]+".dat")
if len(sys.argv) == 3:
    plot_hexbin(x,y,sys.argv[2])
else:
    plot_hexbin(x,y)
#plot_hist(x,y)


