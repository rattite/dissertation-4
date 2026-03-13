#include "db.h"
#include <limits.h>
#include "curve.h"
#include "grid.h"
#include "m2.h"
Node2 **make_trees(point **points, int pnum, bbox *bounds, bbox **clusters, int cluster_count, int b_minp, int c_minp);
void partition_through_multiple_trees(sqlite3 *db, char *tab, char *col, char *ind, Node2 **n, char *partname, int c_count, rule *r, int base_depth, int c_depth);
void range_4_help_3(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, bbox *query, int *found, Node2 *n, rule *base, char *partname, int c_count, bbox **clusters, int partno, int ind_depth);
void method_3_wrapper(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, int limit, Node2 **n, rule *base, char *partname, int c_count, int base_depth,int c_depth);

