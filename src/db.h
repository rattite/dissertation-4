#include "curve.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <spatialite/gaiaconfig.h>
#include <spatialite/gaiageo.h>
#include <spatialite/gaiaaux.h>
#include <spatialite.h>
#include <geos_c.h>
#include <sys/stat.h>
#include <stdio.h>

#include <string.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h> 
unsigned int get_tab_size(sqlite3 *db, char *tab);
point **sample_random_points_from_table(sqlite3 *db, char *table, char *col, int n);
void write_points_to_file(sqlite3 *db, char *tab, char *col);

int file_exists(char *filename);
void normalise(point *p, bbox *b);
void normalise_bbox(bbox *b, bbox *ref);
bbox *get_db_boundaries(sqlite3 *db, char *tab, char *col);
double make_range_with_index(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, int lim, rule *base, int verbose, int ind_depth);
unsigned int make_naive_range(sqlite3 *db, char *tab, char *col, double x, double y, double rad, int lim, int verbose);
void add_index(sqlite3 *db, char *tab, char *col, rule *base, char *name, int depth);
point **create_random(int n, bbox *b);
point **create_gaussian(int n, bbox *b);

sqlite3 *create_db(char *name);
void add_data_to_db(sqlite3 *db, point **p, char *name, int lim);
void remove_col(sqlite3 *db, char *tab, char *col);
rule *get_hilbert_curve();
rule *get_zorder_curve();
sqlite3 *setup_db(char *filename);




