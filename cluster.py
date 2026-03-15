import time
import warnings
import sys
from itertools import cycle, islice

import matplotlib.pyplot as plt
import numpy as np
import cluster
from sklearn import cluster, datasets, mixture
from sklearn.neighbors import kneighbors_graph
from sklearn.preprocessing import StandardScaler

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
    ms = cluster.MeanShift(bandwidth=bandwidth, bin_seeding=True)
    two_means = cluster.MiniBatchKMeans(
        n_clusters=params["n_clusters"],
        random_state=params["random_state"],
    )
    ward = cluster.AgglomerativeClustering(
        n_clusters=params["n_clusters"], linkage="ward", connectivity=connectivity
    )
    spectral = cluster.SpectralClustering(
        n_clusters=params["n_clusters"],
        eigen_solver="arpack",
        affinity="nearest_neighbors",
        random_state=params["random_state"],
    )
    dbscan = cluster.DBSCAN(eps=0.2,min_samples=80)
    hdbscan = cluster.HDBSCAN(
        min_samples=params["hdbscan_min_samples"],
        min_cluster_size=params["hdbscan_min_cluster_size"],
        allow_single_cluster=params["allow_single_cluster"],
        copy=True,
    )
    optics = cluster.OPTICS(
        min_samples=params["min_samples"],
        xi=params["xi"],
        min_cluster_size=params["min_cluster_size"],
    )
    affinity_propagation = cluster.AffinityPropagation(
        damping=params["damping"],
        preference=params["preference"],
        random_state=params["random_state"],
    )
    average_linkage = cluster.AgglomerativeClustering(
        linkage="average",
        metric="cityblock",
        n_clusters=params["n_clusters"],
        connectivity=connectivity,
    )
    birch = cluster.Birch(n_clusters=params["n_clusters"])
    gmm = mixture.GaussianMixture(
        n_components=params["n_clusters"],
        covariance_type="full",
        random_state=params["random_state"],
    )

    clustering_algorithms = (
        #("MiniBatch\nKMeans", two_means),
        #("Affinity\nPropagation", affinity_propagation),
        #("MeanShift", ms),
        #("Spectral\nClustering", spectral),
        #("Ward", ward),
        #("Agglomerative\nClustering", average_linkage),
        ("DBSCAN", dbscan),
        #("HDBSCAN", hdbscan),
        #("OPTICS", optics),
        #("BIRCH", birch),
        #("Gaussian\nMixture", gmm),
    )

    for name, algorithm in clustering_algorithms:
        t0 = time.time()
        plt.figure(figsize=(6, 6))
        

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


                
        ##these are the results

        #now we proceed recursively and see whether we can't divide it further

        if name == "MiniBatch\nKMeans":
            plt.title("Or could it be said that the end justifies the K-Means?", size=18)

        colors = np.array(
            list(
                islice(
                    cycle(
                        [
                            "#377eb8",
                            "#ff7f00",
                            "#4daf4a",
                            "#f781bf",
                            "#a65628",
                            "#984ea3",
                            "#999999",
                            "#e41a1c",
                            "#dede00",
                            ]
                    ),
                    int(max(y_pred) + 1),
                )
            )
        )
        # add black color for outliers (if any)
        colors = np.append(colors, ["#000000"])
        plt.scatter(X[:, 0], X[:, 1], s=10, color=colors[y_pred])

        #plt.xlim(-2.5, 2.5)
        #plt.ylim(-2.5, 2.5)
        plt.xticks(())
        plt.yticks(())
        plt.text(
            0.99,
            0.01,
            ("%.2fs" % (t1 - t0)).lstrip("0"),
            transform=plt.gca().transAxes,
            size=15,
            horizontalalignment="right",
        )
        plt.tight_layout()
        plt.savefig("img/clus_"+str(time.time())+".png")
        plt.show()
    return y_pred


def test(arr):
    params= {
    "quantile": 0.3,
    "eps": 0.3,
    "damping": 0.9,
    "preference": -200,
    "n_neighbors": 3,
    "n_clusters": 6,
    "min_samples": 7,
    "xi": 0.05,
    "min_cluster_size": 0.1,
    "allow_single_cluster": True,
    "hdbscan_min_cluster_size": 15,
    "hdbscan_min_samples": 3,
    "random_state": 42,
    }

    aq = np.array(arr)

    if len(aq) > 4096:
        idx = np.random.choice(aq.shape[0], size=4096, replace=False)
        sub = aq[idx]
    else:
        sub = aq

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

    """af = cluster.AffinityPropagation(
        damping=params["damping"],
        preference=params["preference"],
        random_state=params["random_state"],
    )"""

    af = cluster.MiniBatchKMeans(
        n_clusters=7,
        random_state=params["random_state"],
    )


    ms = cluster.MiniBatchKMeans(
        n_clusters=3,
        random_state=params["random_state"],
    )
    plt.figure(figsize=(6, 6))
    t0=time.time()


    af.fit(X)

    if hasattr(af, "labels_"):
        y_pred = af.labels_.astype(int)
    else:
        y_pred = af.predict(X)


    unique, counts = np.unique(y_pred, return_counts=True)
    q = dict(zip(unique, counts))
    for i,j in q.items():
        if j > 600:
            print("ok!")
            #splits up the cluster
            a = []
            for k in range(len(X)):
                 if y_pred[k] == i:
                    a.append(X[k])
            ms.fit(a)
            if hasattr(ms, "labels_"):
                y_pred2 = ms.labels_.astype(int)
            else:
                y_pred2 = ms.predict(a)
            count = 0
            myp = max(y_pred) #this seems ok
            for l in range(len(X)):
                if y_pred[l] == i:
                    y_pred[l] = myp+1+y_pred2[count]
                    count+= 1
    unique, counts = np.unique(y_pred, return_counts=True)
    q = dict(zip(unique, counts))
    print(q)

    t1 = time.time()



            
    ##these are the results

    #now we proceed recursively and see whether we can't divide it further

    plt.title("Or could it be said that the end justifies the K-Means?", size=18)

    colors = np.array(
        list(
            islice(
                cycle(
                    [
                        "#377eb8",
                        "#ff7f00",
                        "#4daf4a",
                        "#f781bf",
                        "#a65628",
                        "#984ea3",
                        "#999999",
                        "#e41a1c",
                        "#dede00",
                        ]
                ),
                int(max(y_pred) + 1),
            )
        )
    )
    # add black color for outliers (if any)
    colors = np.append(colors, ["#000000"])
    plt.scatter(X[:, 0], X[:, 1], s=10, color=colors[y_pred])

    plt.xlim(-2.5, 2.5)
    plt.ylim(-2.5, 2.5)
    plt.xticks(())
    plt.yticks(())
    plt.text(
        0.99,
        0.01,
        ("%.2fs" % (t1 - t0)).lstrip("0"),
        transform=plt.gca().transAxes,
        size=15,
        horizontalalignment="right",
    )
    plt.title("my epic clustering system", fontsize=14)
    plt.tight_layout()
    plt.show()
    return labels



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
    
    return [x_min, y_min, x_max, y_max]

def merge_clusters(clusters):
    #we have a list of bboxes, we need to merge them if they intersect to avoid the causing of errors
    flag = 0
    while (flag == 0):
        flag = 0
        c2 = []
        if len(clusters) == 1:
            break
        for i in clusters:
            for j in clusters:
                if intersect(i,j) == [0,0,0,0]:
                    c2.append(i)
                    c2.append(j)
                else:
                    c2.append(merge_bboxes(i,j))
                    flag = 1
                    break
            if flag == 1:
                break
        clusters = c2
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


def clustering(filename):
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
    if len(bboxes) > 1:
        merge_clusters(bboxes)
    print("there are: " + str(len(bboxes)))
    a = filename.split(".")[0]
    write_bboxes_to_file(a+".lizard",bboxes)
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
        clustering(filename)

    except Exception as e:
        print(e)
if __name__ == "__main__":
    main()
