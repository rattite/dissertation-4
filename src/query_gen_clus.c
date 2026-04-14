#include "db.h"
#include "test_helper.h"
#include "curve.h"
#include "m2.h"
float euclidean(float x1,float y1,float x2,float y2){
	return sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
}

float max(float a,float b,float c,float d){
	float out = a;
	if (b > out){
		out = b;
	} if (c > out){
		out = c;
	} if (d > out){
		out = d;
	}
	return out;


}
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
		printf("ok!\n");
		printf("%f %f %f %f\n", clus[i]->min_x, clus[i]->min_y, clus[i]->max_x, clus[i]->max_y);
		bbox_get_lengths(clus[i],&xlen,&ylen);
		//printf("xlen is %f ylen is %f\n", xlen, ylen);
		float size = sqrt(xlen*ylen);
		printf("size is %f\n", size);
		double mid_x = (clus[i]->min_x+clus[i]->max_x)/2;
		double mid_y = (clus[i]->min_y+clus[i]->max_y)/2;
		double diag = sqrt(fabs(clus[i]->max_x-clus[i]->min_x)*fabs(clus[i]->max_y-clus[i]->min_y));
		for (int j=0;j<no/bnum;j++){
			f = ((float)rand())/RAND_MAX;
			float q_x = (clus[i]->min_x + (0.15*xlen)+(0.75*f*xlen));
			f = ((float)rand())/RAND_MAX;
			float q_y = (clus[i]->min_y+(0.15*ylen) + (0.75*f*ylen));
			float xl = fabs(q_x-clus[i]->min_x);
			float xu = fabs(q_x-clus[i]->max_x);
			float yl = fabs(q_y-clus[i]->min_y);
			float yu = fabs(q_y-clus[i]->max_y);
			float out = max(xl,xu,yl,yu);
			f = ((float)rand())/RAND_MAX;
			//float q;
			//if (q_x < q_y){q=q_y;}else{q=q_x;}
			//rad = (0.1+0.9*f)*out;
			double ed = pow(((q_x-mid_x)*(q_x-mid_x)+(q_y-mid_y)*(q_y-mid_y)),0.5);
			//printf("diag is %f\n", diag);
			//printf("ed is %f\n", ed);
			rad = 300 + diag*pow(ed/diag,2);
				printf("rad is %f\n", rad);	
			fprintf(fil,"%.0f\n%.0f\n%.0f\n", q_x,q_y,rad);
			}


		}
	fclose(fil);
	free(world);
	printf("COMPLETED!\n");
	sqlite3_close(db);
	return 0;

	}

