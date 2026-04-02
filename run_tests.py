import random
import sqlite3
import subprocess
import os
import time
import graph_cases
import json
import matplotlib.pyplot as plt
import sys
import matplotlib.gridspec as gridspec
##todo: fix m1 bug somehow
from time import sleep
import math
from statistics import geometric_mean
import cluster
import graph_m1
from sklearn.base import BaseEstimator
from sklearn.model_selection import ParameterGrid
#TODO:
#implement graphing
#implement hyperparameter optimisation (should be easy now that we have baseline with test_case objects)
#fix range queries and method 1
#our whole procedure for doing range queries is bad apparently (fails if index depth is too high or low)
#fix method 2 so the boxes aren't shaped so weirdly
#likewise for method 3, i suppose
#25 days until poster deadline!
class query:
    def __init__(self,x,y,rad):
        self.x = x
        self.y = y
        self.rad = rad

    @classmethod
    def generate_query_set(cls,db,tab,col,no,div_x,div_y,samp,out):
        subprocess.run(["./bin/query_gen",db,tab,col,str(no),str(div_x),str(div_y),str(samp),out],capture_output=False)
        print("generated query set!")
    
    @classmethod
    def read_query_set(cls,file):
        queries = []
        f = open(file,"r")
        for line in f.readlines():
            a = line.split(",")
            queries.append(query(float(a[0]),float(a[1]),float(a[2])))
        return queries

    @classmethod
    def generate_clus_query_set(cls,db,tab,col,no,filename,out):
        subprocess.run(["./bin/query_gen_clus",db,tab,col,str(no),filename,out],capture_output=False)
        print("generated query set!")





                         #
                         #
                         #
                         #
                         #
                         #
                         #
                         #
class bbox:
    def __init__(self,minx,miny,maxx,maxy):
        self.min_x = minx
        self.min_y = miny
        self.max_x = maxx
        self.max_y = maxy


class test_case(BaseEstimator):

    def __init__(self):
        self.times = []
        self.hit = []
        pass

    def mean_time(self):
        if len(self.times) == 0:
            return None
        else:
            return sum(self.times)/len(self.times)
    @classmethod
    def get_param_grid(cls):
        return {}
    def fit(self, X=None, y=None):
        runs = []
        for _ in range(2):
            self.run(**X)
            runs.append(self.times[-1])

        self.avg_time_ = sum(runs) / len(runs)
        return self

    def score(self, X=None, y=None):
        return -self.avg_time_

    @classmethod
    def optimise(cls, X):
        best_time = float('inf')
        best_case = None
        
        for params in ParameterGrid(cls.get_param_grid()):
            # create object with current parameters
            obj = cls(**params)
            
            obj.times = []
            obj.fit(X) 
            mean_time = obj.avg_time_
            
            if mean_time < best_time:
                best_time = mean_time
                best_case = obj
        
        return best_case
    def subp(self,filename:str, tab:str, col:str, queries_file:str):
        pass

    def to_dict(self):
        return {"case":self.__class__.__name__, "data":self.__dict__}

    @classmethod
    def from_dict(cls,d:dict):
        t_name = d["case"]
        data = d["data"]
        subclass = globals().get(t_name)
        obj = subclass.__new__(subclass)
        obj.__dict__.update(data)
        return obj

    @classmethod
    def serialise_list(cls,filename:str,cases:list[test_case]):
        f = open(filename, "w")
        json.dump([c.to_dict() for c in cases],f,indent=4)
        f.close()

    @classmethod
    def deserialise_list(cls,filename:str)->list[test_case]:
        f = open(filename,"r")
        data = json.load(f)
        cases = [test_case.from_dict(obj) for obj in data]
        f.close()
        return cases

    def set_results(self):
        f = open("lizard.results","r")
        tc = 0
        hits = []
        time = 0
        lines = f.readlines()
        for line in lines:
            tc += 1
            l = line.split(",")
            time += float(l[0])
            if float(l[1])>0:
                hits.append(float(l[1]))
            else:
                hits.append(10**(-3))
        self.times.append(time/tc)
        self.hit.append(geometric_mean(hits))



    def run(self,filename:str, tab:str, col:str, queries_file:str):
        p1 = subprocess.run(["bin/reset",filename,tab])
        process = self.subp(filename,tab,col,queries_file)
        process.wait()
        self.set_results()
        time.sleep(0.5)

class good_case(test_case):
    def __init__(self):
        super().__init__()
        self.label = "R* Tree"
    def subp(self,filename:str, tab:str, col:str, queries_file:str):
        return subprocess.Popen(["bin/goodtest",filename,tab,col,queries_file])

class naive_case(test_case):
    def __init__(self):
        super().__init__()
        self.label = "Naive"
    def subp(self,filename:str, tab:str, col:str, queries_file:str):
        return subprocess.Popen(["bin/naivetest",filename,tab,col,queries_file])

class index_case(test_case):
    def __init__(self, ind_depth=4):
        super().__init__()
        self.ind_depth = str(ind_depth)
        self.label = "Hilbert"
    @classmethod
    def get_param_grid(cls):
        return {"ind_depth": [4,6]}
    def subp(self,filename:str, tab:str, col:str, queries_file:str):
        return subprocess.Popen(["bin/indextest",filename,tab,col,queries_file,str(self.ind_depth)])

class m1_case(test_case):
    def __init__(self,ind_depth=6,pnum=16):
        super().__init__()
        self.ind_depth = ind_depth
        self.pnum = pnum
        self.label = "Index-Ranges"
    @classmethod
    def get_param_grid(cls):
        return {"ind_depth": [4,6], "pnum": [16,32,64]}

    def subp(self,filename:str, tab:str, col:str, queries_file:str):
        print("ready to go")
        subprocess.run(["bin/m1test",filename,tab,col,queries_file,str(self.ind_depth),str(self.pnum),str(1)])
        print("ran first process")
        return subprocess.Popen(["bin/m1test",filename,tab,col,queries_file,str(self.ind_depth),str(self.pnum),str(0)])

class m2_case(test_case):
    def __init__(self,ind_depth=6,min_leaf=256):
        super().__init__()
        self.ind_depth = ind_depth
        self.min_leaf = min_leaf
        self.label = "Grid"
    @classmethod
    def get_param_grid(cls):
        return {"ind_depth": [4,6], "min_leaf": [128,256]}
    def subp(self,filename:str, tab:str, col:str, queries_file:str):
        return subprocess.Popen(["bin/m2test",filename,tab,col,queries_file,str(self.ind_depth),str(self.min_leaf)])

class m3_case(test_case):
    def __init__(self,b_min=256,c_min=128,b_dep=4,c_dep=4):
        super().__init__()
        self.b_min = b_min
        self.b_dep = b_dep
        self.c_min = c_min
        self.c_dep = b_dep
        self.label = "Grid+Clustering"
    @classmethod
    def get_param_grid(cls):
        return {"b_min": [64,128,256], "c_min": [64,128,256],"b_dep":[4],"c_dep":[4]}
    def subp(self,filename:str, tab:str, col:str, queries_file:str,clus:str):
        return subprocess.Popen(["bin/m3test",filename,tab,col,queries_file,clus,str(self.b_min),str(self.b_dep),str(self.c_min),str(self.c_dep)])
    def run(self,filename:str, tab:str, col:str, queries_file:str,clus:str):
        p1 = subprocess.run(["bin/reset",filename,tab])
        process = self.subp(filename,tab,col,queries_file,clus)
        process.wait()
        self.set_results()
        time.sleep(0.5)


#
#
#
#
#Graphing functions (there will probably be many of these!)
#
#
#
#


def select_best_from_list(cases:list[test_case]):
    best = {}
    for i in cases:
        if i.label not in best:
            best[i.label] = i
        elif i.mean_times() > best[i.label].mean_times():
            best[i.label] = i
    return best

def graph_final_results(cases:list[test_case],flag):
    fig,ax = plt.subplots(figsize=(10,10))
    xlabs = []
    y = []
    best = select_best_from_list(cases)
    for case in best.values():
        y.append(case.times)
        xlabs.append(case.label)
    bp = ax.boxplot(y,patch_artist=True,tick_labels=xlabs,showfliers=False)
    for patch in bp['boxes']:
        patch.set_facecolor('mediumseagreen')
        for median in bp['medians']:
            median.set_color('black')
            median.set_linewidth(2)

    #adds labels, title, etc to graph
    ax.set_yscale('log',base=10)
    ax.set_ylabel("log(time)",fontsize=16)
    ax.tick_params(axis='both', labelsize=20)    
    if flag == 0:
        ax.set_title("Times taken to execute a set of queries on the whole dataset",fontsize=20)
    else:
        ax.set_title("Times taken to execute a set of queries on the clusters",fontsize=20)
    ax.grid(axis='y', linestyle='-', alpha=0.5)

    fig.savefig("img/res"+str(int(time.time()))+".png",bbox_inches="tight",dpi=300)
    plt.close(fig)







    fig,ax = plt.subplots(figsize=(10,10))
    xlabs = []
    y = []
    best = select_best_from_list(cases)
    for case in best.values():
        if case.label != "R* Tree":
            y.append(case.hit)
            xlabs.append(case.label)
    bp = ax.boxplot(y,patch_artist=True,tick_labels=xlabs,showfliers=False)
    for patch in bp['boxes']:
        patch.set_facecolor('mediumseagreen')
        for median in bp['medians']:
            median.set_color('black')
            median.set_linewidth(2)

    #adds labels, title, etc to graph
    ax.set_yscale('log',base=10)
    ax.set_ylabel("log(HIT!)",fontsize=16)
    ax.tick_params(axis='both', labelsize=20)    
    if flag == 0:
        ax.set_title("HIT! for a set of queries on the whole dataset",fontsize=20)
    else:
        ax.set_title("HIT! for set of queries on the clusters",fontsize=20)
    ax.grid(axis='y', linestyle='-', alpha=0.5)

    fig.savefig("img/HIT"+str(int(time.time()))+".png",bbox_inches="tight",dpi=300)
    plt.close(fig)



def graph_separate(cases:list[test_case],dim):

    if (dim[0]*dim[1] == 5):
        print("dim")
        fig = plt.figure(figsize=(12, 12))
        gs = fig.add_gridspec(2, 15)
        ax1 = fig.add_subplot(gs[0, 1:4])
        ax2 = fig.add_subplot(gs[0, 6:9])
        ax3 = fig.add_subplot(gs[0, 11:14])
        ax4 = fig.add_subplot(gs[1, 3:6])
        ax5 = fig.add_subplot(gs[1, 9:12])
        labels=['']
        for i, ax in enumerate([ax1, ax2, ax3, ax4, ax5]):
            ax.set_title(f'{cases[i].label}')
            bp = ax.boxplot(cases[i].times,patch_artist=True,tick_labels=labels)
            for patch in bp['boxes']:
                patch.set_facecolor((1,0.9,1))
            for median in bp['medians']:
                median.set_color('black')
                median.set_linewidth(4)
        plt.subplots_adjust(wspace=0.3)
        plt.savefig("img/res2.png",bbox_inches="tight")
        plt.show()    
    else:
        labels = ['']
        fig, axes = plt.subplots(dim[0],dim[1],figsize=(12,12))
        axes=axes.flatten()
        for i, case in enumerate(cases):
            bp = axes[i].boxplot(case.times,patch_artist=True,tick_labels=labels)
            axes[i].tick_params(axis='y', labelsize=10)  
            for patch in bp['boxes']:
                patch.set_facecolor('lightseagreen')
            for median in bp['medians']:
                median.set_color('black')
                median.set_linewidth(4)
        
            axes[i].set_title(case.label)
        plt.subplots_adjust(wspace=0.3)
        plt.savefig("img/res2.png",bbox_inches="tight")
        plt.show()
#
#
#
#




def get_db_boundaries(filename:str, tab:str, col:str) -> bbox:
    #Same as the c function. We need this for query generation!
    con = sqlite3.connect(filename)
    cur = con.cursor()
    pars = (tab,col)
    res = cur.execute("SELECT extent_min_x, extent_min_y, extent_max_x, extent_max_y FROM geometry_columns_statistics WHERE f_table_name = ? AND f_geometry_column = ?",[tab,col])
    b = res.fetchone()
    print(b)
    out = bbox(b[0],b[1],b[2],b[3])
    con.close()
    print("COMPLETED!")
    return out


def write_out_queries(filename:str, queries: list[query]):
    f = open(filename,"w")
    for query in queries:
        f.write(str(query.x)+"\n")
        f.write(str(query.y)+"\n")
        f.write(str(query.rad)+"\n")
    f.close()
    print("written queries to file!\n")


def rt(name,flag,reps):
    qname = "data/"+name
    cluster.clustering(qname+".dat")
    if flag == 0:
        query.generate_query_set(qname+".sqlite",name,"cent",10,32,32,4096,qname+".queries")
    else:
        query.generate_clus_query_set(qname+".sqlite",name,"cent",10,qname+".lizard",qname+".queries")

    
    X = {"filename": qname + ".sqlite","tab": name,"col": "cent","queries_file": qname + ".queries"}
    X1 = {"filename": qname + ".sqlite","tab": name,"col": "cent","queries_file": qname + ".queries", "clus": qname+".lizard"}

    cases = []
    #cases.append(naive_case.optimise(X))
    cases.append(good_case.optimise(X))
    cases.append(m1_case.optimise(X))
    cases.append(m2_case.optimise(X))
    #m3c = m3_case.optimise(X1)
    for case in cases:
        for i in range(reps):
            case.run(qname+".sqlite",name,"cent",qname+".queries")
    #for i in range(reps):
     #   m3c.run(qname+".sqlite",name,"cent",qname+".queries",qname+".lizard")
    #cases.append(m3c)
    test_case.serialise_list("data/"+name+".cases",cases)

    graph_final_results(cases,flag)

#rt(sys.argv[1],0,int(sys.argv[2]))
#time.sleep(5)
rt(sys.argv[1],0,int(sys.argv[2]))


