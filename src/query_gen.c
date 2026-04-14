#include "db.h"
#include "test_helper.h"
#include "curve.h"



int main(int arcg, char *argv[]){
	//1: db name
	//2: table
	//3: col
	//4: no of queries
	//5: grid length x
	//6: grid length y
	//7: sample size
	//8: out filename
	srand(time(NULL));
	printf("let's go ok!\n");
	sqlite3 *db = setup_db(argv[1]);
	char getsql[256];
	int no = atoi(argv[4]);
	int len_x = atoi(argv[5]);
	int len_y = atoi(argv[6]);
	int samp = atoi(argv[7]);
	printf("%s %s %s %s\n", argv[1],argv[2],argv[3],argv[4]);
	snprintf(getsql,sizeof(getsql),"SELECT %s FROM %s ORDER BY RANDOM() LIMIT %d",argv[3],argv[2],samp);
	printf("%s\n", getsql);
	sqlite3_stmt *stmt;
	if(sqlite3_prepare_v2(db,getsql,-1,&stmt,NULL)!=SQLITE_OK){printf("err: %s\n", sqlite3_errmsg(db));}
	unsigned int *grid = malloc(len_x*len_y*sizeof(unsigned int));
	for (int i=0;i<len_x*len_y;i++){
		grid[i]=0;
	}
	int x;
	int y;
	gaiaGeomCollPtr geom;
	gaiaPointPtr pt;
	bbox *world = get_db_boundaries(db,argv[2],argv[3]);
	printf("%f %f %f %f\n", world->min_x, world->min_y, world->max_x, world->max_y);
	point *p = malloc(sizeof(point));



	while (sqlite3_step(stmt)==SQLITE_ROW){
		//there are issues with data from CSV if it's of differing types somehow... not good
		const void *blob = sqlite3_column_blob(stmt, 0);
	    	int blob_size = sqlite3_column_bytes(stmt, 0);
	    	geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);
	    	pt = geom->FirstPoint; //this line is bad
		p->x = pt->X;
		p->y = pt->Y;
		normalise(p,world);
		x = floor(p->x * len_x);
		y = floor(p->y * len_y);
		grid[y * len_y + x]++;
		gaiaFreeGeomColl(geom);

	}
	sqlite3_finalize(stmt);
	free(p);
	printf("printing grid\n");

	for (int i=0;i<20;i++){printf("%d\n", grid[i]);}
	double world_x = world->max_x - world->min_x;
	double world_y = world->max_y - world->min_y;
	double grid_x = world_x / len_x;
	double grid_y = world_y / len_y;
	double diag = sqrt(world_x*world_y);
	FILE *fil = fopen(argv[8],"w");
	float f;
	int lim;
	int count;
	double q_x;
	double q_y;
	double rad;
	int cell_x;
	int cell_y;
	double min_rad = (0.0625 * pow(diag,0.625));
	double max_rad = 0.225 * pow(diag,0.875);	//generates queries
	for (int i=0;i<no;i++){
		count = 0;
		f = ((float)rand())/RAND_MAX;
		lim = floor(f*samp);
		int j = 0;
		while (1==1){
			count += grid[j];
			if (count > lim){
				break;
			}
			j++;
		}
		cell_x = j % (len_y);
		cell_y = j / len_y;
		f = ((float)rand())/RAND_MAX;
		q_x = (world->min_x + (grid_x*cell_x)+f*grid_x);
		f = ((float)rand())/RAND_MAX;
		q_y = (world->min_y + (grid_y*cell_y)+f*grid_y);
		f = ((float)rand())/RAND_MAX;
		rad = min_rad + ((max_rad-min_rad)*f);
		fprintf(fil,"%.0f\n%.0f\n%.0f\n", q_x,q_y,rad);
	}
	fclose(fil);
	free(world);
	printf("COMPLETED!\n");
	sqlite3_close(db);
	return 0;
}
		//writes out to file, or something

