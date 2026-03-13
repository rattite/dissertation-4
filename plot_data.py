import numpy as np
import numpy.random
import matplotlib.pyplot as plt
import sys

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

if len(sys.argv) != 2:
    print("usage: plot_data.py <filename.dat>")
    exit()

def plot_hexbin(x,y):

    xlim = min(x), max(x)
    ylim = min(y), max(y)
    fig, ax= plt.subplots()
    ax.set_aspect('equal','box')
    ax.axis('off')


    hb = ax.hexbin(x, y,gridsize=50, bins='log', cmap='PRGn')
    ax.set(xlim=xlim, ylim=ylim)
    ax.set_title("With a log color scale")
    cb = fig.colorbar(hb, ax=ax, label='counts')

    plt.savefig("img/"+sys.argv[1]+"_hex.png",bbox_inches="tight")
    plt.show()

def plot_hist(x,y):
    xlim = min(x), max(x)
    ylim = min(y), max(y)
    fig, ax= plt.subplots()
    ax.set_aspect('equal','box')
    ax.axis('off')

    heatmap, xedges, yedges, im= plt.hist2d(x, y, bins=50,norm="log",cmap="PRGn")
    extent = [xedges[0], xedges[-1], yedges[0], yedges[-1]]

    ax.imshow(heatmap.T, extent=extent, origin='lower')
    plt.savefig("img/"+sys.argv[1]+"_map.png",bbox_inches="tight")
    plt.show()

x,y = read_data(sys.argv[1]+".dat")
plot_hexbin(x,y)
#plot_hist(x,y)


