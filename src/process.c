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


void clean(sqlite3 *db, char *tab, char *col);
void clean(sqlite3 *db, char *tab, char *col){
		if(sqlite3_exec(db,"BEGIN",NULL,NULL,NULL)!=SQLITE_OK){printf("err2: %s\n", sqlite3_errmsg(db));}
		char sql2[256];
		snprintf(sql2,sizeof(sql2), "DELETE FROM %s WHERE %s IS NULL OR ST_IsEmpty(%s)", tab, col,col);
		if(sqlite3_exec(db, sql2, NULL, NULL, NULL)!=SQLITE_OK){printf("err3: %s\n", sqlite3_errmsg(db));}
		if(sqlite3_exec(db, "COMMIT", NULL, NULL, NULL)!=SQLITE_OK){printf("err3: %s\n", sqlite3_errmsg(db));}
		printf("clean\n");
}

void add_row_2(sqlite3 *db, char *tab, char *col, char *new);
void add_row_2(sqlite3 *db, char *tab, char *col, char *new){
		sqlite3_stmt *stmt;
		char sql[256];
		snprintf(sql,sizeof(sql),"SELECT AddGeometryColumn(\'%s\', \'%s\', 3857, 'POINT', 2)",tab,new);
		sqlite3_exec(db,sql,NULL,NULL,NULL);
		if(sqlite3_exec(db,"BEGIN",NULL,NULL,NULL)!=SQLITE_OK){printf("err2: %s\n", sqlite3_errmsg(db));}
		snprintf(sql,sizeof(sql),"UPDATE %s SET %s = ST_Transform(ST_Centroid(%s),3857)",tab,new,col,col);
		if(sqlite3_exec(db,sql,NULL,NULL,NULL)!=SQLITE_OK){printf("err: %s\n", sqlite3_errmsg(db));}else{printf("ok!\n");}

		if(sqlite3_exec(db, "COMMIT", NULL, NULL, NULL)!=SQLITE_OK){printf("err3: %s\n", sqlite3_errmsg(db));}
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
	//clean(db,argv[2],argv[3]);
	add_row_2(db,argv[2],argv[3],argv[4]);

	printf("processing complete!");
	return 0;
}


