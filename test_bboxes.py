#actually draws the bboxes stored in the database

import sqlite3
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from itertools import cycle, islice
import numpy as np
con = sqlite3.connect("large.sqlite")
cur = con.cursor()
res = cur.execute("SELECT extent_min_x, extent_min_y, extent_max_x, extent_max_y FROM geometry_columns_statistics WHERE f_geometry_column = 'geom' AND extent_max_y NOT NULL")
bboxes = res.fetchall()
print(bboxes)
colours = np.array(list(islice(cycle([
                            "#377eb8",
                            "#ff7f00",
                            "#4daf4a",
                            "#f781bf",
                            "#a65628",
                            "#984ea3",
                            "#999999",
                            "#e41a1c",
                            "#dede00",
                            "#bbbbbb",
                            "#ff0000",
                            "#00ff00",
                            "#0000ff",
                            "#ffff00",
                            "#00ffff",
                            "#ff00ff"
                            ]),len(bboxes),)))
"""
#draws all points on some sort of scatter graph
x = []
y = []
for i in bboxes:
    x.append(i[0])
    y.append(i[1])
    x.append(i[2])
    y.append(i[3])

twocols = [x for x in colours for _ in range(2)]
print(twocols)
#plt.scatter(x, y, s=30, c=twocols, alpha=0.5)
#plt.show()
"""



fig,ax=plt.subplots()
ax.set_xlim(min(t[0] for t in bboxes)-1, max(t[2] for t in bboxes)+1)
ax.set_ylim(min(t[1] for t in bboxes)-1, max(t[3] for t in bboxes)+1)
for i in bboxes:
    rectangle = patches.Rectangle((i[0], i[1]), i[2]-i[0], i[3]-i[1], linewidth=2, edgecolor='blue', facecolor='none')
    ax.add_patch(rectangle)
ax.set_aspect('equal', 'box')
plt.show()

"""
#min_x: [0][0]
#min_y: [0][1]
#max_x: [1][0]
#max_y: [1][1]
for i in range(len(bboxes)):
    plt.plot(bboxes[i][0], bboxes[i][1], bboxes[i][0], bboxes[i][3], marker = 'o')
    plt.plot(bboxes[i][2], bboxes[i][1], bboxes[i][2], bboxes[i][3], marker = 'o')
    plt.plot(bboxes[i][0], bboxes[i][1], bboxes[i][2], bboxes[i][1], marker = 'o')
    plt.plot(bboxes[i][0], bboxes[i][3], bboxes[i][2], bboxes[i][3], marker = 'o')
"""
plt.show()

