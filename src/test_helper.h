#include "db.h"

/*
 * Helper functions for automated testing*/
typedef struct{
	double x;
	double y;
	double rad;
}query;

void reset_db(sqlite3 *db, char *tab);
query **read_queries_from_file(char *filename, int *qnum);
sqlite3 *setup_db(char *filename);
void remove_index_col(sqlite3 *db, char *tab, char *col);



