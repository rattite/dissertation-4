//adds an geometry column in epsg3857 for a dataset and gives it a nice name
//this is so we can try other datasets with our existing system!

#include <sqlite3.h>
#include <spatialite/gaiaconfig.h>
#include <spatialite/gaiageo.h>
#include <spatialite/gaiaaux.h>
#include <spatialite.h>
#include <geos_c.h>
#include <stdio.h>
#include "db.h"

void add_row_2(sqlite3 *db, char *tab, char *col, char *new);
void add_row_2(sqlite3 *db, char *tab, char *col, char *new){
		sqlite3_stmt *stmt;
		char sql[256];
		snprintf(sql,sizeof(sql),"SELECT AddGeometryColumn(\'%s\', \'%s\', 3857, 'POINT', 2)",tab,new);
		sqlite3_exec(db,sql,NULL,NULL,NULL);
		snprintf(sql,sizeof(sql),"UPDATE %s SET %s = ST_Transform(ST_Centroid(%s),3857) WHERE %s IS NOT NULL",tab,new,col,col);
		sqlite3_exec(db,sql,NULL,NULL,NULL);
		sqlite3_exec(db, "SELECT UpdateLayerStatistics()",NULL,NULL,NULL);

		printf("completed!\n");
}

int main(int argc, char *argv[]){
	if (argc < 5){
		printf("how to use: ./process <file> <table> <col> <new col>\n");
		return 1;
	}
	sqlite3 *db;
	sqlite3_open(argv[1], &db);
	void *cache = spatialite_alloc_connection ();
	spatialite_init_ex (db, cache, 0);
	spatialite_init_geos();
	add_row_2(db,argv[2],argv[3],argv[4]);
	printf("processing complete!");
	return 0;
}


