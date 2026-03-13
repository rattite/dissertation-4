#include "db.h"
#include <limits.h>
#include "curve.h"
#include "grid.h"


typedef struct indlist indlist;
typedef struct indlist{
	unsigned int *inds;
	int pnum;
}indlist;

void destroy_part_structure(sqlite3 *db, char *tab, int part_no);
void partition_col_by_index_ranges(sqlite3 *db, char *tab, char *col, char *ind,rangelist *ranges, rule *base, int ind_depth);
unsigned int *get_all_indices(sqlite3 *db, char *tab, char *ind, int depth, int grouping, int sample, int *count);
void beter2(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, int lim, rule *base, int verbose, int ind_depth);
void beter(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, int lim, rule *base, int verbose, int ind_depth);

