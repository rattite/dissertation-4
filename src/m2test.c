#include "curve.h"
#include "test_helper.h"
#include "db.h"
#include <time.h>
#include <stdio.h>
#include "grid.h"
#include "m2.h"

int main(int argc, char *argv[]){
	//there are NO database files involved
	//each test includes a database file, table, column, and a list of queries
	//it would be best to get it with json or somethin
	//
	//the command line arguments should go as follows:
	//1,2,3: filename, table, col
	//4: query file
	//5: index depth
	//6: min points for a leaf
	struct timespec start, end;
	rule *r = get_hilbert_curve();

	//current investigation: it seems as if similar index values are being returned, but the right points aren't being put in those values!

	if (argc == 1){
		sqlite3 *db = setup_db("data/large.sqlite");

		point **p = sample_random_points_from_table(db,"large","cent",4096);
		bbox *world = get_db_boundaries(db,"large","cent");
		print_bbox(world);
		int count = 0;
		Node2 *n = make_tree_2(p,4096,world,255,0,&count);

		partition_help(db,"large","cent","q",n,r,"partname",4);
		sqlite3_exec(db, "SELECT UpdateLayerStatistics()",NULL,NULL,NULL);
    		clock_gettime(CLOCK_MONOTONIC, &start);
		range_wrapper_help(db,"large","cent","q",-460000,6625000,20000,100000,n,r,"partname",4);
    		clock_gettime(CLOCK_MONOTONIC, &end);
    		double elapsed = (end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec) / 1e9;
		printf("clock strikes 12 midnight arrives %.9f seconds\n", elapsed);
		sqlite3_close(db);

	}
	else{
		sqlite3 *db = setup_db(argv[1]);
		sqlite3_exec(db, "SELECT UpdateLayerStatistics()",NULL,NULL,NULL);
		FILE *extra_stream = fopen("data/large/lizard.results","w");

		//gets queries
		int qnum = 0;
		query **q = read_queries_from_file(argv[4],&qnum);
		int depth = atoi(argv[5]);
		if (depth == 0){printf("wrong!\n");}
		int minp = atoi(argv[6]);
		if (minp == 0){printf("wrong!\n");}

		point **p = sample_random_points_from_table(db,argv[2],argv[3],4096);
		bbox *world = get_db_boundaries(db,argv[2],argv[3]);
		print_bbox(world);
		int count = 0;
		Node2 *n = make_tree_2(p,4096,world,minp,0,&count);
		partition_help(db,argv[2],argv[3],"q",n,r,"partname",depth);
		sqlite3_exec(db, "SELECT UpdateLayerStatistics()",NULL,NULL,NULL);
		for (int i=0;i<qnum;i++){

    			clock_gettime(CLOCK_MONOTONIC, &start);
			double ret = range_wrapper_help(db,argv[2],argv[3],"q",q[i]->x,q[i]->y,q[i]->rad,100000,n,r,"partname",depth);
			clock_gettime(CLOCK_MONOTONIC, &end);
			double elapsed = (end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec) / 1e9;
			fprintf(extra_stream,"%.9f,%.6f\n", elapsed,ret);
			fflush(extra_stream);

		}
		fclose(extra_stream);
		sqlite3_close(db);
	}
	return 0;
}


		

