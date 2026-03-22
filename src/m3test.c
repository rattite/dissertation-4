#include "curve.h"
#include "test_helper.h"
#include "db.h"
#include <time.h>
#include <stdio.h>
#include<unistd.h>
#include "grid.h"
#include "m3.h"

int main(int argc, char *argv[]){
	//there are NO database files involved
	//each test includes a database file, table, column, and a list of queries
	//it would be best to get it with json or somethin
	//
	//the command line arguments should go as follows:
	//1,2,3: filename, table, col
	//4: query file
	//5: cluster file
	//6,7: base min points, base index depth
	//8,9: cluster min points, cluster index depth
	struct timespec start, end;
	rule *r = get_hilbert_curve();

	//current investigation: it seems as if similar index values are being returned, but the right points aren't being put in those values!
	
	if (argc == 1){
		sqlite3 *db = setup_db("large.sqlite");

		point **p = sample_random_points_from_table(db,"large","cent",4096);
		bbox *world = get_db_boundaries(db,"large","cent");
		print_bbox(world);

		int count = 0;
		int bnum = 0;
		bbox **clusters = read_bboxes_from_file("test.lizard",&bnum);
		printf("bnum is %d\n", bnum);
		Node2 **n = make_trees(p,4096,world,clusters,bnum,128,64);
		partition_through_multiple_trees(db,"large","cent","q",n,"method3",bnum,r,4,3);
		sqlite3_exec(db, "SELECT UpdateLayerStatistics()",NULL,NULL,NULL);
    		clock_gettime(CLOCK_MONOTONIC, &start);

		method_3_wrapper(db,"large","cent","q",-460000,6625000,20000,100000,n,r,"method3",bnum,4,3);
    		clock_gettime(CLOCK_MONOTONIC, &end);
    		double elapsed = (end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec) / 1e9;
		printf("Elapsed time: %.9f seconds\n", elapsed);
		printf("completeda!\n");
		sqlite3_close(db);
	}
	else{
		FILE *test = fopen("help", "w");
			fprintf(test,"%d\n", argc);

		for (int i=1;i<=9;i++){
			fprintf(test,argv[i]);
			fprintf(test,"\n");
		}
		printf("going!\n");
		fclose(test);
		FILE *extra_stream;
		if (argc == 11){
		extra_stream = fdopen(atoi(argv[10]), "w");
		}

		sqlite3 *db = setup_db(argv[1]);
		sqlite3_exec(db, "SELECT UpdateLayerStatistics()",NULL,NULL,NULL);

		//gets queries
		int qnum = 0;
		query **q = read_queries_from_file(argv[4],&qnum);
		int base_depth = atoi(argv[7]);
		if (base_depth == 0){printf("wrong!\n");}
		int base_minp = atoi(argv[6]);
		if (base_minp == 0){printf("wrong!\n");}
		int c_depth = atoi(argv[9]);
		if (c_depth == 0){printf("wrong!\n");}
		int c_minp = atoi(argv[8]);
		if (c_minp == 0){printf("wrong!\n");}

		point **p = sample_random_points_from_table(db,argv[2],argv[3],4096);
		bbox *world = get_db_boundaries(db,argv[2],argv[3]);
		print_bbox(world);
		int count = 0;
		int bnum = 0;
		bbox **clusters = read_bboxes_from_file(argv[5],&bnum);
		for (int i=0;i<bnum;i++){
			print_bbox(clusters[i]);
		}

		printf("bnum is %d\n", bnum);
		Node2 **n = make_trees(p,4096,world,clusters,bnum,base_minp, c_minp);
		partition_through_multiple_trees(db,argv[2],argv[3],"q",n,"method3",bnum,r,base_depth,c_depth);
		sqlite3_exec(db, "SELECT UpdateLayerStatistics()",NULL,NULL,NULL);
    		clock_gettime(CLOCK_MONOTONIC, &start);
		for (int i=0;i<qnum;i++){
			method_3_wrapper(db,argv[2],argv[3],"q",q[i]->x,q[i]->y,q[i]->rad,100000,n,r,"method3",bnum,base_depth,c_depth);
		}
    		clock_gettime(CLOCK_MONOTONIC, &end);
    		double elapsed = (end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec) / 1e9;
		if (argc == 11){
		fprintf(extra_stream,"%.9f\n", elapsed);
		fflush(extra_stream);
		fclose(extra_stream);
		}
		sqlite3_close(db);
	}
	return 0;
}

