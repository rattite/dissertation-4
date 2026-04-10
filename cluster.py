import time
import warnings
import sys
from itertools import cycle, islice
import matplotlib.patches as patches

import matplotlib.pyplot as plt
import numpy as np
import cluster
from sklearn import cluster, datasets, mixture
from sklearn.neighbors import kneighbors_graph
from sklearn.preprocessing import StandardScaler
import geopandas as gpd

#runs some clustering tests. mostly here just for posterity

def test_clustering(sub):

    params= {
    "quantile": 0.3,
    "eps": 0.5,
    "damping": 0.9,
    "preference": -200,
    "n_neighbors": 3,
    "n_clusters": 6,
    "min_samples": 400,
    "xi": 0.05,
    "min_cluster_size": 0.1,
    "allow_single_cluster": True,
    "hdbscan_min_cluster_size": 8,
    "hdbscan_min_samples": 5,
    "random_state": 42,
    }


    dataset = (sub, None)
    X, Y = dataset

    # normalize dataset for easier parameter selection
    X = StandardScaler().fit_transform(X)

    # estimate bandwidth for mean shift
    bandwidth = cluster.estimate_bandwidth(X, quantile=params["quantile"])

    # connectivity matrix for structured Ward
    connectivity = kneighbors_graph(
        X, n_neighbors=params["n_neighbors"], include_self=False
    )
    # make connectivity symmetric
    connectivity = 0.5 * (connectivity + connectivity.T)

# ============
    dbscan = cluster.DBSCAN(eps=0.18,min_samples=60)
    #dbscan = cluster.HDBSCAN(min_cluster_size=67,min_samples=30,cluster_selection_epsilon=0.1,cluster_selection_method="eom",allow_single_cluster=True)
    clustering_algorithms = (
        ("HDBSCAN", dbscan),
    )

    for name, algorithm in clustering_algorithms:
        # catch warnings related to kneighbors_graph
        with warnings.catch_warnings():
            warnings.filterwarnings(
                "ignore",
                message="the number of connected components of the "
                "connectivity matrix is [0-9]{1,2}"
                " > 1. Completing it to avoid stopping the tree early.",
                category=UserWarning,
            )
            warnings.filterwarnings(
                "ignore",
                message="Graph is not fully connected, spectral embedding"
                " may not work as expected.",
                category=UserWarning,
            )
            algorithm.fit(X)

        t1 = time.time()
        if hasattr(algorithm, "labels_"):
            y_pred = algorithm.labels_.astype(int)
        else:
            y_pred = algorithm.predict(X)

    return y_pred



def get_data(filename):
    f = open(filename,"r")
    nums = []
    while True:
        k = f.readline()
        if not k:
            break
        a = k.split(",")
        nums.append([float(a[0]), float(a[1])])

    return nums

def get_cluster_sizes(points, labels):
    sizes = {}
    for i in range(len(points)):
        if (labels[i] != -1):
            if (labels[i] not in sizes):
                sizes[labels[i]] = 1
            else:
                sizes[labels[i]] += 1
    return sizes



def process_clusters(points, labels,sizes):
    #filters clusters by size
    #a good heuristic could be proportion of the data contained
    #so for example, if we take the data size to be n, we take everything with n/8 points, or something like that
    #it would depend on how our data is distributed
    #we could test how much of our data gets clustered across multiple datasets, or something like that

    size = len(points)
    indices = []
    for i in range(0,len(sizes)):
        if sizes[i] > size / 64:
            indices.append(i)
    print("indices are:",indices)
    return indices
       

def intersect(c1:list[float],c2:list[float]):
    x1 = max(c1[0], c2[0])
    y1 = max(c1[1], c2[1])
    x2 = min(c2[2], c1[2])
    y2 = min(c2[3], c1[3])
    if x1 <= x2 and y1 <= y2:
        return [x1, y1, x2, y2]
    
    return [0,0,0,0]
def merge_bboxes(b1, b2):
    x_min = min(b1[0], b2[0])
    y_min = min(b1[1], b2[1])
    x_max = max(b1[2], b2[2])
    y_max = max(b1[3], b2[3])
    return [x_min,y_min,x_max,y_max]

    

    return [x_min, y_min, x_max, y_max] 
def intersect(b1, b2):
    return not (b1[2] < b2[0] or b1[0] > b2[2] or b1[3] < b2[1] or b1[1] > b2[3])

def merge_clusters(clusters):
    if not clusters:
        return []
    changed = True
    while changed:
        changed = False
        results = []
        while clusters:
            current = clusters.pop(0)
            has_intersected = False
            
            for i in range(len(results)):
                if intersect(current, results[i]):
                    results[i] = merge_bboxes(current, results[i])
                    has_intersected = True
                    changed = True
                    break
            
            if not has_intersected:
                results.append(current)
        clusters = results
        
    return clusters

def compute_bboxes(points, labels, clusters_to_use):
    ##TODO: merge clusters that have overlapping bboxes
    ##points: the array of points that we have
    #labels: the label of each point in points
    #clusters_to_use: a list of indices that we check labels against
    bboxes = {}
    for i in clusters_to_use:
        print(i)
        #minx, miny, maxx, maxy
        bboxes[i] = [30000000,30000000,-30000000,-30000000]
    print(bboxes)
    print(clusters_to_use)
    for i in range(len(points)):
        q = labels[i]
        if q in clusters_to_use:
            #if the point is in some cluster we're interested in
            if points[i][0] < bboxes[q][0]:
                bboxes[q][0] = points[i][0]
            if points[i][1] < bboxes[q][1]:
                bboxes[q][1] = points[i][1]
            if points[i][0] > bboxes[q][2]:
                bboxes[q][2] = points[i][0]
            if points[i][1] > bboxes[q][3]:
                bboxes[q][3] = points[i][1]
    return list(bboxes.values())

def graph_clusters(points, labels,bboxes, shapefile=None):
    fig, ax = plt.subplots(figsize=(10, 10))
    ax.set_axis_off()

    colors = np.array(["lightcoral","mediumseagreen", "cornflowerblue", "orchid", "orange", "turquoise", "mediumslateblue", "pink", "#000000"])
    if shapefile:
        ax.set_xlim(-480000,-370000)
        ax.set_ylim(6480000,6590000)

        region = gpd.read_file(shapefile)
        region = region.to_crs(epsg=3857)
        #scol = (14/16, 255/256, 14/16)
        scol = "palegoldenrod"
        region.plot(ax=ax, color=scol, edgecolor="black")
    ax.scatter(points[:, 0], points[:, 1], s=12, color=colors[labels])

    for b in bboxes:
        print(b)
        rectangle = patches.Rectangle((b[0], b[1]), b[2]-b[0], b[3]-b[1], linewidth=6, edgecolor='sienna', fill=False)
        ax.add_patch(rectangle)
    fig.savefig("img/clus_"+str(int(time.time()))+".png",bbox_inches="tight",dpi=300)
    #plt.show()
    plt.close(fig)

def clustering(filename,shapefile=None):
    data = get_data(filename)
    aq = np.array(data)

    if len(aq) > 1024:
        idx = np.random.choice(aq.shape[0], size=1024, replace=False)
        sub = aq[idx]
    else:
        sub = aq

    labels = test_clustering(sub)
    print(labels[:20])
    sizes = get_cluster_sizes(sub,labels)
    clusters_to_use = process_clusters(sub, labels,sizes)
    bboxes = compute_bboxes(sub,labels,clusters_to_use)
    #if len(bboxes) > 1:
     #   bboxes = merge_clusters(bboxes)
    graph_clusters(sub,labels,bboxes,shapefile)
    print("there are: " + str(len(bboxes)))
    a = filename.split(".")[0]
    write_bboxes_to_file(a+".lizard",bboxes)
    return len(bboxes)
    print("done!\n")

def write_bboxes_to_file(filename,bboxes):
    print(bboxes)
    f = open(filename, "w")
    for i in bboxes:
        f.write(str(i[0])+"\n"+str(i[1])+"\n"+str(i[2])+"\n"+str(i[3])+"\n")

    f.close()
    print("COMPLETED")


def main():
    try:
        filename = sys.argv[1]
        if len(sys.argv) == 2:
            clustering(filename)
        if len(sys.argv) == 3:
            clustering(filename,sys.argv[2])

    except Exception as e:
        print(e)
if __name__ == "__main__":
    main()
