#include "curve.h"
#include "test_helper.h"
#include "db.h"
#include <time.h>
#include <stdio.h>
#include "grid.h"
#include "m3.h"

void print_out_node(Node2 *n, FILE *f){
	if (n->leaf != 1){
		for (int i=0;i<4;i++){
			print_out_node(n->children[i],f);
		}
	} else{
		fprintf(f,"%.6f,%.6f,%.6f,%.6f\n", n->boundaries->min_x, n->boundaries->min_y, n->boundaries->max_x, n->boundaries->max_y);
		fflush(f);

	}

}	

int main(int argc, char *argv[]){
	//argv 1:filename 2:tab 3:col 4:sample 5:minleaf 6: minleaf_clus 7: clustering file 8:pipename
	printf("%s, %s, %s, %s, %s\n", argv[1], argv[2], argv[3], argv[4], argv[5]);
	fflush(stdout);
	sqlite3 *db = setup_db(argv[1]);
	FILE *extra_stream = fdopen(atoi(argv[8]), "w");
	point **p = sample_random_points_from_table(db,argv[2],argv[3],atoi(argv[4]));
	bbox *world = get_db_boundaries(db,argv[2],argv[3]);
	int count = 0;
	int bnum = 0;
	bbox **clusters = read_bboxes_from_file(argv[7],&bnum);
	Node2 **n = make_trees(p,atoi(argv[4]),world,clusters,bnum,atoi(argv[5]),atoi(argv[6]));
	for (int i=0;i<bnum+1;i++){
		print_out_node(n[i],extra_stream);
	}
	for (int i=0;i<atoi(argv[4]);i++){
		free(p[i]);
	}
	free(p);
	fclose(extra_stream);
	sqlite3_close(db);

	return 0;
}
		


