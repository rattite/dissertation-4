#ifndef NODE_H
#define NODE_H
#include "curve.h"

typedef struct Node Node;
typedef struct Node{
	Node **children;
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
}Node;

Node *make_node(bbox *bounds); 
Node *grid_division_x(point **p, int pnum, bbox *bounds, int min_count, int depth, int *count);

bbox *create_bbox(double min_x, double min_y, double max_x, double max_y);
int *pop_by_index_range(int *indices, int pcount, int max_index, int sample_freq);
void print_bbox(bbox *b);
rangelist *make_partitions_by_population(unsigned int *indices, int maxparts, int pcount, int depth, int grouping);
rangelist *make_good_partitioningquestionmark(int *indices, int maxparts, int pcount);
Node *grid_division(point **p, int pnum, bbox *bounds, int minp, int maxp, int depth, int *count);
void test_node(Node *n);
double dist(point *p, point *q);
unsigned short int *k_means(point **p, int k, int pnum);

void destroy_tree(Node *n);
int *get_cluster_sizes(unsigned short int *cluster, int k, int pnum);
double silhouette_score(point **p, unsigned short int *cluster, int pnum, int k);
double davies_bouldin(point **p, unsigned short int *cluster, int pnum, int k);
bbox **partitions_to_bboxes(point **p, unsigned short int *cluster, int k, int pnum);

int sort_points_x(const void *p, const void *q);
int sort_points_y(const void *p, const void *q);


#endif

