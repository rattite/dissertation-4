#include "curve.h"
#include "test_helper.h"
#include "db.h"
#include <time.h>
#include <stdio.h>

int main(int argc, char *argv[]){
	//there are NO database files involved
	//each test includes a database file, table, column, and a list of queries
	//it would be best to get it with json or somethin
	//argv 1,2,3: file,table,col
	////4: query file
	struct timespec start, end;
	if (argc == 1){
		sqlite3 *db = setup_db("data/large.sqlite");
    		clock_gettime(CLOCK_MONOTONIC, &start);
		make_naive_range(db,"large","cent",-460000,6625000,20000,100000,0);
    		clock_gettime(CLOCK_MONOTONIC, &end);
    		double elapsed = (end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec) / 1e9;
		printf("clock strikes 12 midnight arrives %.9f seconds\n", elapsed);
		sqlite3_close(db);

	}
	else{
		FILE *extra_stream = fopen("lizard.results","w");

		sqlite3 *db = setup_db(argv[1]);
		//gets queries
		int qnum = 0;
		query **q = read_queries_from_file(argv[4],&qnum);
		unsigned int tabsize = get_tab_size(db,argv[2]);
		for (int i=0;i<qnum;i++){
			printf("query: %f %f %f", q[i]->x, q[i]->y, q[i]->rad);
    			clock_gettime(CLOCK_MONOTONIC, &start);
			unsigned int found = make_naive_range(db,argv[2],argv[3],q[i]->x,q[i]->y,q[i]->rad,100000,0);
			clock_gettime(CLOCK_MONOTONIC, &end);
			double elapsed = (end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec) / 1e9;
			double rt = (double)found/(double)tabsize;
			fprintf(extra_stream,"%.9f,%.6f\n", elapsed,rt);
			fflush(extra_stream);

		}
		fclose(extra_stream);
		sqlite3_close(db);
	}
	spatialite_cleanup();
	return 0;
}


		
