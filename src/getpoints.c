#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <spatialite/gaiaconfig.h>
#include <spatialite/gaiageo.h>
#include <spatialite/gaiaaux.h>
#include <spatialite.h>
#include <geos_c.h>
#include <sys/stat.h>

//USAGE: ./getpoints (db_name) (table_name) (col_name)


int main(int argc, char *argv[]){
	printf("ready to go!\n");
	sqlite3 *db;
	if (argc < 5){
		printf("how to use: (db) (table) (col) (outname)\n");
		exit(1);
	}
	sqlite3_open(argv[1], &db);
	void *cache = spatialite_alloc_connection ();
	spatialite_init_ex (db, cache, 0);
	sqlite3_stmt *stmt;
	char sql[256];
	snprintf(sql,sizeof(sql), "SELECT ST_Transform(ST_Centroid(%s),3857) FROM %s",argv[3],argv[2]);
	if (sqlite3_prepare_v2(db,sql,-1,&stmt,NULL) != SQLITE_OK){printf("err: %s\n", sqlite3_errmsg(db));}
	//opens file pointer
	FILE *f;
	printf("args are %s, %s, %s\n", argv[1], argv[2], argv[3]);
	f = fopen(argv[4], "w");
	while (sqlite3_step(stmt) == SQLITE_ROW){
	    const void *blob = sqlite3_column_blob(stmt, 0);
	    int blob_size = sqlite3_column_bytes(stmt, 0);
	    if (blob != NULL){
	    gaiaGeomCollPtr geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);
	    gaiaPointPtr pt = geom->FirstPoint;
		double x = pt->X;
		double y = pt->Y;
		fprintf(f, "%f,%f\n", x, y);
	    }
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	fclose(f);
	return 0;
}


