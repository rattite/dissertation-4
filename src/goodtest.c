#include "curve.h"
#include "test_helper.h"
#include "db.h"
#include <time.h>
#include <stdio.h>

void really_remove_index(sqlite3 *db, char *tab, char *col){
	char buf[256];
	snprintf(buf,sizeof(buf),"SELECT DisableSpatialIndex('%s', '%s')", tab,col);
	if(sqlite3_exec(db,buf,NULL,NULL,NULL)!=SQLITE_OK){printf("err: %s\n", sqlite3_errmsg(db));}

	const char *s[] = {"", "_node", "_parent", "_rowid"};
	char sql[512];
	for (int i=0;i<4;i++){
		snprintf(sql,sizeof(sql),"SELECT DropTable(NULL,'idx_%s_%s%s')",tab,col,s[i]);
		if(sqlite3_exec(db,sql,NULL,NULL,NULL)!=SQLITE_OK){printf("err removing: %s\n", sqlite3_errmsg(db));}
	}
	printf("completed?\n");
}

void make_good_range(sqlite3 *db, char *tab, char *col, double x, double y, double rad, int lim, int verbose){
    sqlite3_stmt *stmt;
    char sql[512];
    int count = 0;
    snprintf(sql, sizeof(sql), "SELECT a.ogc_fid, a.%s FROM %s AS a WHERE ST_Distance(a.%s, MakePoint(?, ?, 3857)) <= ? AND a.ROWID IN (SELECT ROWID FROM SpatialIndex WHERE f_table_name = ? AND f_geometry_column = ? AND search_frame = BuildCircleMbr(?, ?, ?,3857)) LIMIT ?", col, tab, col);

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK){printf("err: %s\n", sqlite3_errmsg(db));}
    sqlite3_bind_double(stmt, 1, x);
    sqlite3_bind_double(stmt, 2, y);
    sqlite3_bind_double(stmt, 3, rad);
    sqlite3_bind_text(stmt, 4, tab, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt,5,col,-1,SQLITE_STATIC);
    sqlite3_bind_double(stmt, 6, x);
    sqlite3_bind_double(stmt, 7, y);
    sqlite3_bind_double(stmt, 8, rad);
    sqlite3_bind_int(stmt, 9, lim);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
        if (verbose) {
            const void *blob = sqlite3_column_blob(stmt, 1);
            int blob_size = sqlite3_column_bytes(stmt, 1);
            gaiaGeomCollPtr geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);
            
            if (geom && geom->FirstPoint) {
                gaiaPointPtr pt = geom->FirstPoint;
                printf("ID: %d | X: %f, Y: %f\n", sqlite3_column_int(stmt, 0), pt->X, pt->Y);
            }
            gaiaFreeGeomColl(geom);
        }
    }

    sqlite3_finalize(stmt);
    printf("The query was a success and found %d points\n", count);
}
int main(int argc, char *argv[]){
	//there are NO database files involved
	//each test includes a database file, table, column, and a list of queries
	//it would be best to get it with json or somethin
	struct timespec start, end;
	if (argc == 1){
		sqlite3 *db = setup_db("large.sqlite");
		really_remove_index(db,"large","cent");

		char buf[256];
		snprintf(buf,sizeof(buf),"SELECT CreateSpatialIndex('%s', '%s')", "large", "cent");
		if(sqlite3_exec(db,buf,NULL,NULL,NULL)!=SQLITE_OK){printf("err: %s\n", sqlite3_errmsg(db));}
		clock_gettime(CLOCK_MONOTONIC, &start);
		make_good_range(db,"large","cent",-460000,6625000,20000,100000,0);
    		clock_gettime(CLOCK_MONOTONIC, &end);
    		double elapsed = (end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec) / 1e9;
		printf("clock strikes 12 midnight arrives %.9f seconds\n", elapsed);
		really_remove_index(db,"large","cent");

		sqlite3_close(db);

	}
	else{
		FILE *extra_stream = fopen("lizard.results","w");

		sqlite3 *db = setup_db(argv[1]);
		char sql[256];
		really_remove_index(db,argv[2],argv[3]);
		snprintf(sql,sizeof(sql),"SELECT CreateSpatialIndex('%s', '%s')", argv[2], argv[3]);
		if(sqlite3_exec(db,sql,NULL,NULL,NULL)!=SQLITE_OK){printf("err adding: %s\n", sqlite3_errmsg(db));}

		//gets queries
		int qnum = 0;
		query **q = read_queries_from_file(argv[4],&qnum);
		for (int i=0;i<qnum;i++){
    			clock_gettime(CLOCK_MONOTONIC, &start);
			printf("query: %f %f %f", q[i]->x, q[i]->y, q[i]->rad);
			make_good_range(db,argv[2],argv[3],q[i]->x,q[i]->y,q[i]->rad,100000,0);
    			clock_gettime(CLOCK_MONOTONIC, &end);
    			double elapsed = (end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec) / 1e9;
			fprintf(extra_stream,"%.9f,-1\n", elapsed);
			fflush(extra_stream);

		}
		fclose(extra_stream);
		really_remove_index(db,argv[2],argv[3]);
		printf("removed index!\n");

		sqlite3_close(db);
	}
	return 0;
}

