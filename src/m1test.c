#include "curve.h"
#include "test_helper.h"
#include "m1.h"
#include "db.h"
#include <time.h>
#include <stdio.h>
#include "grid.h"

void run_demo_query(sqlite3 *db){
	const char *sql = "SELECT f_table_name FROM geometry_columns_statistics WHERE f_geometry_column = 'geom'";
	char count_sql[256];
	sqlite3_stmt *stmt;
	sqlite3_stmt *count_stmt;
	sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
	while (sqlite3_step(stmt) == SQLITE_ROW){
		const char *name = sqlite3_column_text(stmt,0);
		snprintf(count_sql,sizeof(count_sql),"SELECT COUNT(*) FROM %s", name);
		sqlite3_prepare_v2(db,count_sql,-1,&count_stmt,NULL);
		sqlite3_step(count_stmt);
		sqlite3_finalize(count_stmt);
	}
	sqlite3_finalize(stmt);
	printf("done test query\n");
}
		


int main(int argc, char *argv[]){
	//parameters are as follows
	//1,2,3: filename, tab and col
	//4: query file
	//5: index depth
	//6: no of index ranges
	//7: stream
	//8: make or query
	struct timespec start, end;
	rule *base = get_hilbert_curve();

	if (argc == 1){
		sqlite3 *db = setup_db("data/large.sqlite");
		destroy_part_structure(db,"large",64);

		//sets up index
		add_index(db,"large","cent",base,"arg1",4);
		int count = 0;
		unsigned int *indx = get_all_indices(db,"large","arg1",4,2,0,&count);
		sqlite3_exec(db,"CREATE TEMP TABLE IF NOT EXISTS warmup(x INT);",NULL,NULL,NULL);
		rangelist *r = make_partitions_by_population(indx, 16, count,4,2); 
		run_demo_query(db);
		partition_col_by_index_ranges(db,"large","cent","arg1",r,base,4);
		//do dummy query to warm up the database
    		clock_gettime(CLOCK_MONOTONIC, &start);
		beter2(db, "large", "cent", "arg1", -460000, 6625000, 20000, 100000, base, 0,4);
    		clock_gettime(CLOCK_MONOTONIC, &end);
    		double elapsed = (end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec) / 1e9;
		printf("clock strikes 12 midnight arrives %.9f seconds\n", elapsed);
		//this seems like an issue with page cache?
		//ie. the first time the program is ran, it's slow, and the second time it's rather fast

		//shuts down index
		remove_index_col(db,"large","arg1");
		destroy_part_structure(db,"large",64);
		sqlite3_close(db);

	} else{
		int depth = atoi(argv[5]);
		if (depth == 0){printf("wrong!\n");}
		int rangeno = atoi(argv[6]);
		if (rangeno == 0){printf("wrong!\n");}
		sqlite3 *db = setup_db(argv[1]);
		FILE *extra_stream = fdopen(atoi(argv[7]), "w");

		//gets queries
		int qnum = 0;
		query **q = read_queries_from_file(argv[4],&qnum);

		if (atoi(argv[8])==1){
			add_index(db,argv[2],argv[3],base,"test",depth);
			int count = 0;
			unsigned int *indx = get_all_indices(db,argv[2],"test",depth,4,0,&count);
			rangelist *r = make_partitions_by_population(indx, rangeno, count,depth,4); 
			partition_col_by_index_ranges(db,argv[2],argv[3],"test",r,base,depth);
			
			beter2(db, argv[2], argv[3], "test", q[0]->x, q[0]->y, q[0]->rad, 100000, base, 0,depth);
			remove_index_col(db,"large","arg1");
			sqlite3_close(db);
		} else{
			clock_gettime(CLOCK_MONOTONIC, &start);

			for (int i=0;i<qnum;i++){
				beter2(db, argv[2], argv[3], "test", q[i]->x, q[i]->y, q[i]->rad, 100000, base, 0,depth);
			}
			clock_gettime(CLOCK_MONOTONIC, &end);
			double elapsed = (end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec) / 1e9;
			fprintf(extra_stream,"%.9f\n", elapsed);
			fflush(extra_stream);
			fclose(extra_stream);
			//remove_index_col(db,argv[2],argv[3]);
			sqlite3_close(db);
		}
	}
	return 0;
}


