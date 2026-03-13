#include "curve.h"
#include "test_helper.h"
#include "db.h"
#include <time.h>
#include <stdio.h>

int main(int argc, char *argv[]){
	//there are NO database files involved
	//each test includes a database file, table, column, and a list of queries
	//it would be best to get it with json or somethin
	//arguments:
	//1,2,3: filename, table, col
	//4:query file
	//5: depth
	struct timespec start, end;
	rule *r = get_hilbert_curve();

	if (argc == 1){
		sqlite3 *db = setup_db("large.sqlite");
		//sqlite3_exec(db, "PRAGMA temp_store = MEMORY;", NULL, NULL, NULL);
		//sqlite3_exec(db, "PRAGMA synchronous = OFF;", NULL, NULL, NULL);
		//sets up index
		add_index(db,"large","cent",r,"arg1",6);
    		clock_gettime(CLOCK_MONOTONIC, &start);
		make_range_with_index(db,"large","cent","arg1",-460000,6625000,20000,10000,r,0,6);
    		clock_gettime(CLOCK_MONOTONIC, &end);
    		double elapsed = (end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec) / 1e9;
		printf("clock strikes 12 midnight arrives %.9f seconds\n", elapsed);
		//shuts down index
		remove_index_col(db,"large","arg1");
		sqlite3_close(db);

	}
	else{
		FILE *extra_stream = fdopen(atoi(argv[6]), "w");
		sqlite3 *db = setup_db(argv[1]);
		//gets queries
		int qnum = 0;
		int depth = atoi(argv[5]);
		if (depth == 0){printf("wrong!\n");}
		query **q = read_queries_from_file(argv[4],&qnum);
		add_index(db,argv[2],argv[3],r,"test",depth);
    		clock_gettime(CLOCK_MONOTONIC, &start);

		for (int i=0;i<qnum;i++){
			make_range_with_index(db,argv[2],argv[3],"test",q[i]->x,q[i]->y,q[i]->rad,10000,r,0,depth);
		}
    		clock_gettime(CLOCK_MONOTONIC, &end);
    		double elapsed = (end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec) / 1e9;
		fprintf(extra_stream,"%.9f\n", elapsed);
		fflush(extra_stream);
		fclose(extra_stream);
		remove_index_col(db,argv[2],"test");
		sqlite3_close(db);
	}
	return 0;
}

