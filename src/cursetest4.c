#include "curve.h"
#include <stdio.h>
#include "db.h"
#include "test_helper.h"
#include <time.h>
rangelist *getr2(bbox *b, rule *r, int prec, int tolerance);
rangelist *getr2(bbox *b, rule *r, int prec, int tolerance){
	rangelist *rl = malloc(sizeof(rangelist));
	rl->ranges = malloc(100000*sizeof(range *)); //we will realloc this later!!!!
	rl->len = 0;
	intbbox *q = unit_to_int(b,prec);
	//printf("%d %d %d %d\n", q->min_x, q->min_y, q->max_x, q->max_y);
	intbbox *world = create_large(prec);
	//printf("%d %d\n", world->min_x, world->max_x);
	//now we call the gr function
	gr(q,world,r,prec,0,rl,0,tolerance); //this is recursive, so we need only call it once
	//HOPEFULLY it should come out sorted
	//
	//for (int i=0;i<rl->len;i++){
	//	printf("range is %d %d\n", rl->ranges[i]->start, rl->ranges[i]->end);
	//}
	//range **temp = realloc(rl->ranges, rl->len * sizeof(range *));
	//rl->ranges = temp;
	//printf("len is %d\n", rl->len);
	free(q);
	free(world);
	return rl;

}
double make_range_with_index3(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, int lim, rule *base, int verbose, int ind_depth, int tolerance, int *rangenum);
double make_range_with_index3(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, int lim, rule *base, int verbose, int ind_depth, int tolerance, int *rangenum){
	//Makes a GOOD range query on a particular table. 
	point *p = malloc(sizeof(point));
	p->x = x;
	p->y = y;
	sqlite3_stmt *stmt;
	//clock_t end = clock();
	//printf("stage 1 of preprep: %f\n",(double)(end-start)/CLOCKS_PER_SEC);
	int total = 0;
	bbox *world = get_db_boundaries(db,tab,col);
	//start = clock();
	normalise(p,world);
	bbox *b = (bbox *)malloc(sizeof(bbox));
	//TODO: add processing for when we're at poles or dateline
	b->min_x = x-rad;
	b->max_x = x+rad;
	b->min_y = y-rad;
	b->max_y = y+rad;
	//printf("x: %f %f, y: %f %f\n", b->min_x, b->max_x, b->min_y, b->max_y);

	normalise_bbox(b, world);
	//printf("min_x %f, min_y %f, max_x %f, max_y %f\n", b->min_x, b->min_y, b->max_x, b->max_y);
	//end = clock();
	//printf("time taken for stage 2 of preprep: %f\n",(double)(end-start)/CLOCKS_PER_SEC);

	//printf("x: %f %f, y: %f %f\n", b->min_x, b->max_x, b->min_y, b->max_y);
	//start = clock();
	rangelist *rl = getr2(b,base,ind_depth,tolerance); 
	*rangenum = rl->len;
	printf("help %d\n", *rangenum);
	if (rl->len == 0){
		printf("debug: box is %f %f %f %f\n", b->min_x, b->min_y, b->max_x, b->max_y);
		printf("debug: centre is %f %f\n", p->x, p->y);
	}
	free(b);
	free(world);
	free(p);
	double rad2 = rad * rad;
		int id;



	char s3[102400];
	sqlite3_stmt *stmt_2;
	int found = 0;
	char tmp[256];
      	snprintf(s3, sizeof(s3), "SELECT %s, ogc_fid FROM %s WHERE (%s BETWEEN %d AND %d) ",col,tab,ind,rl->ranges[0]->start, rl->ranges[0]->end);
	for (int i=1;i<rl->len;i++){
		snprintf(tmp,sizeof(tmp),"OR (%s BETWEEN %d AND %d) ", ind, rl->ranges[i]->start, rl->ranges[i]->end);
		strcat(s3,tmp);
	}
	//printf("%s\n", s3);
	if(sqlite3_prepare_v2(db,s3,-1,&stmt_2,NULL)!=SQLITE_OK){printf("err43:%s\n", sqlite3_errmsg(db));}
	sqlite3_bind_double(stmt_2,1,x);
	sqlite3_bind_double(stmt_2,2,y);
	sqlite3_bind_double(stmt_2,3,rad);
	while (sqlite3_step(stmt_2) == SQLITE_ROW){
		total++;
		id = sqlite3_column_int(stmt_2, 1);
	    	const void *blob = sqlite3_column_blob(stmt_2, 0);
		int blob_size = sqlite3_column_bytes(stmt_2, 0);
		gaiaGeomCollPtr geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);
		gaiaPointPtr pt = geom->FirstPoint;
		double dx = x-pt->X;
		double dy = y-pt->Y;
		double dist2 = (dx*dx + dy*dy);
		if (dist2 < rad2){
				//printf("id %d x: %f y: %f\n",id, pt->X,pt->Y);
			found++;
		}
			//printf("id %d x: %f y: %f\n",sqlite3_column_int(stmt_2,1),pt->X,pt->Y);
	    gaiaFreeGeomColl(geom);
	}
	sqlite3_finalize(stmt_2);
	free_rangelist(rl);
	printf("points found: %d\n", found);
	sqlite3_exec(db, "DELETE FROM candidates", NULL, NULL, NULL);
	//end = clock();
	//printf("time taken to execute query: %f\n",(double)(end-start)/CLOCKS_PER_SEC);
	return (double)found/(double)total;
}


int main(int argc, char *argv[]){
	if (argc < 1){exit(1);}
	struct timespec start, end;
	rule **s = malloc(2*sizeof(rule *));
	rule *r = get_hilbert_curve();
	//ah, good old-fashioned C programming
	//what fun!!!!
	rule *z = get_zorder_curve();
	s[0]=r;
	s[1]=z;
	char *a[] = {"hilb","zorder"};
	char name[256];
	char n2[256];
	char n3[256];
	snprintf(name,sizeof(name),"data/%s/trials.lizard",argv[1]);
	snprintf(n2,sizeof(n2),"data/%s.sqlite",argv[1]);
	snprintf(n3,sizeof(n3),"data/%s.queries",argv[1]);
	printf("%s,%s,%s\n", name,n2,n3);
	printf("ok!\n");
	int dep[] = {4,6,8,10};
	FILE *ext = fopen(name,"a");
	int qnum = 0;
	query **q = read_queries_from_file(n3,&qnum);
	printf("got queries\n");
	printf("queries are %d\n", qnum);
	int rangenum = 0;
	char indname[128];
	int xp = 0;
	sqlite3 *db = setup_db(n2);
	printf("opened!\n");
	int depth = atoi(argv[2]);
	int curve = atoi(argv[3]);
	snprintf(indname,sizeof(indname),"ind_%d_%d",dep[depth],curve);
	printf("%s\n", indname);
	//this seems to work!
	add_index(db,argv[1],"cent",s[curve],indname,dep[depth]);
	printf("add index\n");
	for (int i=0;i<2;i++){
		//tung tung tung sahur
		double tt=0;
		double th=0;
		double tc=0;
		xp =i * pow(2,dep[depth]/2);
		if (xp == 0){xp=1;}
		//now for each query
		for (int j=0;j<5;j++){
			clock_gettime(CLOCK_MONOTONIC, &start);
			double ret = make_range_with_index3(db,argv[1],"cent",indname,q[j]->x,q[j]->y,q[j]->rad,100000,s[curve],0,dep[depth],xp,&rangenum);
			clock_gettime(CLOCK_MONOTONIC, &end);
			double elapsed = (end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec) / 1e9;
			tt = tt + elapsed;
			if (ret > 0){
				th = th + ret;
				tc++;
			}
		}
		if (tc > 0){
		tt = tt/tc;
		th = th/tc;
		}


		fprintf(ext,"%s,%d,%d,%f,%f\n",a[curve],dep[depth],xp,tt,th);
	}
	fflush(ext);
	sqlite3_close(db);
	fclose(ext);
	printf("completed!\n");
}

