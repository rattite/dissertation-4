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
        subprocess.run(["./bin/query_gen",db,tab,col,str(no),str(div_x),str(div_y),str(samp),out],capture_output=True)
        print("generated query set!")
    
    @classmethod
    def read_query_set(cls,file):
        queries = []
        f = open(file,"r")
        for line in f.readlines():
            a = line.split(",")
            queries.append(query(float(a[0]),float(a[1]),float(a[2])))
        return queries





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

class test_case:
    def __init__(self):
        self.times = []
        pass

    def subp(self,filename:str, tab:str, col:str, queries_file:str,w):
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

    def run(self,filename:str, tab:str, col:str, queries_file:str):
        p1 = subprocess.run(["bin/reset",filename,tab])


        r,w = os.pipe()
        try:
            process = self.subp(filename,tab,col,queries_file,w)
            print(process)
            os.close(w)
            process.wait()
            with os.fdopen(r, 'r') as secret_stream:
                data = secret_stream.read()
                print(f"Captured from FD 3: {data}")
                print(f"data is as follows: {data}")
                self.times.append(float(data))
                #if we redirect this to an output file, it gets printed at the end
                #do processing or something, save to local "time" variable
        finally:
            if 'w' in locals():
                try: os.close(w)
                except: pass

class good_case(test_case):
    def __init__(self):
        self.times = []
        self.label = "Good"
    def subp(self,filename:str, tab:str, col:str, queries_file:str,w):
        return subprocess.Popen(["bin/goodtest",filename,tab,col,queries_file,str(w)],pass_fds=(w,))

class naive_case(test_case):
    def __init__(self):
        self.times = []
        self.label = "Naive"
    def subp(self,filename:str, tab:str, col:str, queries_file:str,w):
        return subprocess.Popen(["bin/naivetest",filename,tab,col,queries_file,str(w)],pass_fds=(w,))

class index_case(test_case):
    def __init__(self, ind_depth):
        self.times = []
        self.ind_depth = str(ind_depth)
        self.label = "Hilbert"
    def subp(self,filename:str, tab:str, col:str, queries_file:str,w):
        return subprocess.Popen(["bin/indextest",filename,tab,col,queries_file,self.ind_depth,str(w)],pass_fds=(w,))

class m1_case(test_case):
    def __init__(self,ind_depth,pnum):
        self.times = []
        self.ind_depth = str(ind_depth)
        self.pnum = str(pnum)
        self.label = "M1"
    def subp(self,filename:str, tab:str, col:str, queries_file:str,w):
        print("ready to go")
        subprocess.run(["bin/m1test",filename,tab,col,queries_file,self.ind_depth,self.pnum,str(w),str(1)],pass_fds=(w,))
        print("ran first process")
        return subprocess.Popen(["bin/m1test",filename,tab,col,queries_file,self.ind_depth,self.pnum,str(w),str(0)],pass_fds=(w,))

class m2_case(test_case):
    def __init__(self,ind_depth,min_leaf):
        self.times = []
        self.ind_depth = str(ind_depth)
        self.min_leaf = str(min_leaf)
        self.label = "M2"
    def subp(self,filename:str, tab:str, col:str, queries_file:str,w):
        return subprocess.Popen(["bin/m2test",filename,tab,col,queries_file,self.ind_depth,self.min_leaf,str(w)],pass_fds=(w,))

class m3_case(test_case):
    def __init__(self,b_min,b_dep,c_min,c_dep,clus):
        self.times = []
        self.b_min = str(b_min)
        self.b_dep = str(b_dep)
        self.c_min = str(c_min)
        self.c_dep = str(c_dep)
        self.clus = clus
        self.label = "M3"

    def subp(self,filename:str, tab:str, col:str, queries_file:str,w):
        return subprocess.Popen(["bin/m3test",filename,tab,col,queries_file,self.clus,self.b_min,self.b_dep,self.c_min,self.c_dep,str(w)],pass_fds=(w,))


#
#
#
#
#Graphing functions (there will probably be many of these!)
#
#
#
#

def graph_final_results(cases:list[test_case]):
    xlabs = []
    y = []
    for case in cases:
        y.append(case.times)
        xlabs.append(case.label)
    bp = plt.boxplot(y,patch_artist=True,tick_labels=xlabs)
    for patch in bp['boxes']:
        patch.set_facecolor('seagreen')
        for median in bp['medians']:
            median.set_color('black')
            median.set_linewidth(2)

    #adds labels, title, etc to graph
    plt.yscale('log',base=10)
    plt.ylabel("log(t)")
    plt.title("Time taken for each test case")
    plt.grid(axis='y', linestyle='-', alpha=0.5)

    plt.savefig("img/res.png",bbox_inches="tight")
    plt.show()




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
                patch.set_facecolor('seagreen')
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



if __name__ == "__main__":
    #we iterate over files that we've supplied
    t1 = time.time()
    for i in range(1,len(sys.argv)):
        name = sys.argv[i]
        qname = "data/"+name
        query.generate_query_set(qname+".sqlite",name,"cent",20,32,32,4096,qname+".queries")
        cluster.clustering(qname+".dat")
        cases = []
        cases.append(naive_case())
        cases.append(good_case())
        #cases.append(index_case(6))
        #cases.append(index_case(8))
        #cases.append(m1_case(6,64))
        cases.append(m1_case(6,16))
        cases.append(m2_case(4,256))
        #cases.append(m2_case(4,128))
        cases.append(m3_case(4,256,4,256,qname+".lizard"))
        #cases.append(m3_case(4,64,4,64,qname+".lizard"))
        #test_case.serialise_list("test.json",cases)
        #cases2 = test_case.deserialise_list("test.json")

        subprocess.run(["./bin/reset",qname+".sqlite",name])
        print(len(cases))
        for case in cases:
            for i in range(3):
                #fix iteration so we don't have to make the partitioning each time
                print(case.label)
                case.run(qname+".sqlite",name,"cent",qname+".queries")
                sleep(0.142857)

        print("final times for each case:")
        for case in cases:
            print(sum(case.times)/len(case.times))
        graph_final_results(cases) 
        #graph_separate(cases2,(2,2))
        test_case.serialise_list("results/"+name+".dat",cases)
    t2 = time.time()
    print("real time taken: "+str(t2-t1))

    #cleanup
