#if !defined(CURVE_H)
#define CURVE_H


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

typedef struct{
	double min_x;
	double min_y;
	double max_x;
	double max_y;
}bbox;

typedef struct{
	double x;
	double y;
}point;

typedef struct rule{
	int dim;
	struct rule **next;
	short int *trav;
	short int *pos;
}rule;

typedef struct rulelist{
	rule **rules;
	int len;
}rulelist;

typedef struct range{
	unsigned int start;
	unsigned int end;
}range;

typedef struct rangelist{
	range **ranges;
	int len;
}rangelist;

void free_rule(rule *r);
void free_rangelist(rangelist *r);
int get_index(point *p, rule *base, int prec);
bbox *get_bbox(point **p, int n);
point *bbox_to_unit(point *p, bbox *b);
point *unit_to_bbox(point *p, bbox *b);

void z_test();
int comp(const void *a, const void *b);
rangelist *collect (int *arr, int n, int tolerance);
void assign_trav(rule *r, short int *t, int dim);
rangelist *get_ranges(bbox *b, rule *base, int prec);
bbox *unindex(int ind, rule *base, int prec);
rule *rule_init(int dim);
int measure_large_discontinuities(rule *base, int prec);
int check_if_adjacent(bbox *b, bbox *c, int prec);
#endif

