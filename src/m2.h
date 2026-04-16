#include "db.h"
#include <limits.h>
#include "curve.h"
#include "grid.h"

void add_node_part(sqlite3 *db, char *tab, char *col, char *ind, Node *n, int *count); 
int point_in_bbox(point *p, bbox *b);
void partition_by_grid(sqlite3 *db, char *tab, char *col, char *ind, Node *begin, rule *r);
void denormalise(point *p, bbox *b);
int get_index_2(point *p, Node *n, rule *r, int prec, int plog);
void test_grid_2(Node *n, rule *base, int pcount);
void serialtest();
void serialise_node(Node *n, FILE *f);
Node *read_node(FILE *f);

typedef struct Node2 Node2;
typedef struct Node2{
	Node2 **children;
	//indexed in the N order for some reason; that's how we've been doing everything else
	unsigned int depth;
	unsigned int pop;
	bbox *boundaries;
	int pnum;
	int ind;
	int leaf;
	double x_med;
	double ly_med;
	double ry_med;
}Node2;


bbox **read_bboxes_from_file(char *filename, int *bnum);
bbox *get_intersect_help(bbox *b, bbox *c);

int point_in_bbox(point *p, bbox *b);
int bbox_in_bbox(bbox *s, bbox *l);
Node2 *make_tree_2(point **p, int pnum, bbox *bounds, int min_count, int depth, int *count);
int get_index_node(Node2 *start, point *p, int *partnum, rule *r, int ind_depth);
void add_node_part_help(sqlite3 *db, char *tab, char *col, char *ind, Node2 *n, char *partname);
void partition_help(sqlite3 *db, char *tab, char *col, char *ind, Node2 *start, rule *r, char *partname, int ind_depth); 
void test_node_help(sqlite3 *db,Node2 *n, int ind_depth);
void range_4_help(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, bbox *query, int *found, int *checked, Node2 *n, rule *base, char *partname, int ind_depth);
double range_wrapper_help(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, int limit, Node2 *n, rule *base, char *partname, int ind_depth);

