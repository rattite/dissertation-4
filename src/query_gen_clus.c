#include "db.h"
#include "test_helper.h"
#include "curve.h"
#include "m2.h"
int main(int argc, char *argv[]){
	//1: db name
	//2: table
	//3: col
	//4: no of queries per cluster
	//5: query file
	//6: out filename
	srand(time(NULL));
	printf("let's go ok!\n");
	sqlite3 *db = setup_db(argv[1]);
	bbox *world = get_db_boundaries(db,argv[2],argv[3]);
	int no = atoi(argv[4]);
	FILE *fil = fopen(argv[6],"w");
	int bnum = 0;
	bbox **clus = read_bboxes_from_file(argv[5], &bnum);
	int count = 0;
	double xlen = 0;
	double ylen = 0;
	float f;
	float rad;
	for (int i=0;i<bnum;i++){
		bbox_get_lengths(clus[i],&xlen,&ylen);
		float size = sqrt(xlen*ylen/8);

		for (int j=0;j<no;j++){
			f = ((float)rand())/RAND_MAX;
			float q_x = (clus[i]->min_x + (0.1*xlen)+(0.8*f*xlen));
			f = ((float)rand())/RAND_MAX;
			float q_y = (clus[i]->min_y+(0.1*ylen) + (0.8*f*ylen));
			f = ((float)rand())/RAND_MAX;
			float q;
			if (q_x < q_y){q=q_y;}else{q=q_x;}
			rad = (0.1+f)*size;
			fprintf(fil,"%.0f\n%.0f\n%.0f\n", q_x,q_y,rad);
			}


		}
	fclose(fil);
	free(world);
	printf("COMPLETED!\n");
	sqlite3_close(db);
	return 0;

	}

