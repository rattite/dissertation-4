#include "db.h"

int file_exists(char *filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}
void normalise(point *p, bbox *b){

	p->x = (p->x-b->min_x)/(b->max_x-b->min_x);
	p->y = (p->y-b->min_y)/(b->max_y-b->min_y);

	//eg. p = (-15,-10)
	//eg. b = (-20,20)to(-10,20)
	//-15 -> (-15-(-20))=5 / (

}

void normalise_bbox(bbox *b, bbox *ref){
	b->min_x = (b->min_x-ref->min_x)/(ref->max_x-ref->min_x);
	b->min_y = (b->min_y-ref->min_y)/(ref->max_y-ref->min_y);
	b->max_x = (b->max_x-ref->min_x)/(ref->max_x-ref->min_x);
	b->max_y = (b->max_y-ref->min_y)/(ref->max_y-ref->min_y);
	//adds processing for when we're at poles and such
	if (b->min_x < 0){b->min_x = 0;}
	if (b->min_y < 0){b->min_y = 0;}
	if (b->max_x > 1){b->max_x = 1;}
	if (b->max_y > 1){b->max_y = 1;}

}


bbox *get_db_boundaries(sqlite3 *db, char *tab, char *col){
	//gets the boundaries for a particular geometry column
	//important: it has to be registered properly!
	//clock_t start = clock();
	char sql[256];
	sqlite3_stmt *stmt;
	//sqlite3_exec(db, "SELECT UpdateLayerStatistics()",NULL,NULL,NULL);
 	snprintf(sql,sizeof(sql), "SELECT extent_min_x, extent_min_y, extent_max_x, extent_max_y FROM geometry_columns_statistics WHERE f_table_name = \'%s\' AND f_geometry_column = \'%s\'",tab, col);
	if(sqlite3_prepare_v2(db,sql,-1,&stmt,NULL)!=SQLITE_OK){printf("error %s\n", sqlite3_errmsg(db));}
	bbox *b = (bbox *)malloc(sizeof(bbox));
	sqlite3_step(stmt);
	b->min_x = sqlite3_column_double(stmt, 0);
	b->max_x = sqlite3_column_double(stmt, 2);
	b->min_y = sqlite3_column_double(stmt, 1);
	b->max_y = sqlite3_column_double(stmt, 3);
	//clock_t end = clock();
	//printf("time taken to get db boundaries: %f\n",(double)(end-start)/CLOCKS_PER_SEC);
	sqlite3_finalize(stmt);

	return b;
}


double make_range_with_index(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, int lim, rule *base, int verbose, int ind_depth){
	//Makes a GOOD range query on a particular table
	clock_t start = clock();
	point *p = (point *)malloc(sizeof(point));
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
	printf("min_x %f, min_y %f, max_x %f, max_y %f\n", b->min_x, b->min_y, b->max_x, b->max_y);
	//end = clock();
	//printf("time taken for stage 2 of preprep: %f\n",(double)(end-start)/CLOCKS_PER_SEC);

	//printf("x: %f %f, y: %f %f\n", b->min_x, b->max_x, b->min_y, b->max_y);
	//start = clock();
	rangelist *rl = get_ranges_2(b,base,ind_depth); 
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
	printf("%s\n", s3);
	if(sqlite3_prepare_v2(db,s3,-1,&stmt_2,NULL)!=SQLITE_OK){printf("err4:%s\n", sqlite3_errmsg(db));}
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
	printf("points found: %d\n", found);
	sqlite3_exec(db, "DELETE FROM candidates", NULL, NULL, NULL);
	//end = clock();
	//printf("time taken to execute query: %f\n",(double)(end-start)/CLOCKS_PER_SEC);
	return (double)found/(double)total;
}

void write_points_to_file(sqlite3 *db, char *tab, char *col){

	sqlite3_stmt *stmt;
	char sql[256];
	snprintf(sql,sizeof(sql), "SELECT %s FROM %s",col,tab);
	char out[64];
	sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
	//opens file pointer
	FILE *f;
	f = fopen("out.dat", "w");
	while (sqlite3_step(stmt) == SQLITE_ROW){
	    const void *blob = sqlite3_column_blob(stmt, 0);
	    int blob_size = sqlite3_column_bytes(stmt, 0);
	    gaiaGeomCollPtr geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);
	    gaiaPointPtr pt = geom->FirstPoint;
		double x = pt->X;
		double y = pt->Y;
		fprintf(f, "%f,%f\n", x, y);
		gaiaFreeGeomColl(geom);
	}
	fclose(f);
}

		
unsigned int get_tab_size(sqlite3 *db, char *tab){
	char sql[256];
	sqlite3_stmt *stmt;
	snprintf(sql,sizeof(sql),"SELECT COUNT(*) FROM %s", tab);
	if(sqlite3_prepare_v2(db,sql,-1,&stmt,NULL)!=SQLITE_OK){printf("error %s\n", sqlite3_errmsg(db));}
	unsigned int out;
	while (sqlite3_step(stmt) == SQLITE_ROW){
		out = (unsigned int)sqlite3_column_int(stmt,0);
	}
	sqlite3_finalize(stmt);
	return out;
}

unsigned int make_naive_range(sqlite3 *db, char *tab, char *col, double x, double y, double rad, int lim, int verbose){
	//Makes a naive range query. By definition, it's not very good
	//TODO: look into making a query a struct somehow for future tests
	sqlite3_stmt *stmt;
	char sql[256];
	int count = 0;
	snprintf(sql,sizeof(sql), "SELECT ogc_fid, %s FROM %s WHERE ST_Distance(%s,MakePoint(?,?,3857)) < ? LIMIT ?",col,tab,col);
	//printf("%s\n", sql);
	if(sqlite3_prepare_v2(db,sql,-1,&stmt,NULL)!=SQLITE_OK){printf("error %s\n", sqlite3_errmsg(db));}
	if (sqlite3_bind_double(stmt, 1, x)!=SQLITE_OK){printf("%s\n", sqlite3_errmsg(db));}
	if (sqlite3_bind_double(stmt,2,y)!=SQLITE_OK){printf("%s\n", sqlite3_errmsg(db));}
	if(sqlite3_bind_double(stmt,3,rad)!=SQLITE_OK){printf("%s\n",sqlite3_errmsg(db));}
	if (sqlite3_bind_double(stmt,4,lim)!=SQLITE_OK){printf("%s\n", sqlite3_errmsg(db));}
	while (sqlite3_step(stmt) == SQLITE_ROW){
    	const void *blob = sqlite3_column_blob(stmt, 1);
    	int blob_size = sqlite3_column_bytes(stmt, 1);
    	gaiaGeomCollPtr geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);
	gaiaPointPtr pt = geom->FirstPoint;
	count++;
    	if(verbose){printf("POINT, id: %d, (%f %f)\n", pt->X, pt->Y, sqlite3_column_int(stmt,0));}
	}
	sqlite3_finalize(stmt);
	printf("found %d\n points!\n", count);
	return count;
}

void add_index(sqlite3 *db, char *tab, char *col, rule *base, char *name, int depth){
	//Adds an index for a given column and a given curve.
	//This takes a while!

	char check[256];
	sqlite3_stmt *chk;
	snprintf(check,sizeof(check), "SELECT name FROM PRAGMA table_info(%s) WHERE name = %s", tab, name);
	sqlite3_prepare_v2(db,check,-1,&chk,NULL);
	if (sqlite3_step(chk) == SQLITE_ROW){
		sqlite3_finalize(chk);
		return;
	}
	sqlite3_finalize(chk);
	char sql[256];
	snprintf(sql, sizeof(sql), "ALTER TABLE %s ADD %s INT NULL", tab,name);
	char add[256];
	snprintf(add, sizeof(add),"UPDATE %s SET %s = ? WHERE ogc_fid = ?",tab,name);

	//adds the new column to the database
	sqlite3_stmt *alter_stmt;
	sqlite3_stmt *stmt;
	sqlite3_stmt *add_stmt;
	if(sqlite3_exec(db,sql,NULL,NULL,NULL)!=SQLITE_OK){printf("error: %s\n", sqlite3_errmsg(db));}
	sqlite3_exec(db,"BEGIN TRANSACTION", NULL, NULL, NULL);
	int index = 0;
	int id = 0;
	point *c = (point *)malloc(sizeof(point));
	bbox *b = get_db_boundaries(db,tab,col);
	char get[256];
	snprintf(get,sizeof(get), "SELECT ogc_fid, %s FROM %s",col,tab);

	//does the index stuff
	if (sqlite3_prepare_v2(db, get, -1, &stmt, NULL)!=SQLITE_OK){printf("err1%s\n", sqlite3_errmsg(db));}
	if (sqlite3_prepare_v2(db, add, -1, &add_stmt, NULL)!=SQLITE_OK){printf("err2%s\n",sqlite3_errmsg(db));}
	while (sqlite3_step(stmt) == SQLITE_ROW) {
	id = sqlite3_column_int(stmt, 0);
	//printf("id is %d\n", id);
	    const void *blob = sqlite3_column_blob(stmt, 1);
	    int blob_size = sqlite3_column_bytes(stmt, 1);
	    gaiaGeomCollPtr geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);

	    c->x = geom->FirstPoint->X;
	    c->y = geom->FirstPoint->Y;
	    //printf("c is %f %f\n", c->x, c->y);

	    normalise(c,b);
	    gaiaFreeGeomColl(geom);
	    index = get_index(c,base,depth); //TODO: change up how precision is calculated
	//printf("%d\n", index);
	    if(sqlite3_bind_int(add_stmt,1,index)!=SQLITE_OK){printf("err2: %s\n", sqlite3_errmsg(db));}
	    if(sqlite3_bind_int(add_stmt,2,id)!=SQLITE_OK){printf("err2: %s\n", sqlite3_errmsg(db));}
	    //this line specifically is giving errors
	    if(sqlite3_step(add_stmt)!=SQLITE_DONE){printf("err: 2%s\n", sqlite3_errmsg(db));}
	    sqlite3_reset(add_stmt);
	}
	sqlite3_finalize(stmt);
	sqlite3_finalize(add_stmt);
	sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
	free(c);
	free(b);
	char ch[256];
	snprintf(ch,sizeof(ch), "CREATE INDEX idx_%s ON %s(%s)", col,tab,col);
	sqlite3_exec(db, ch,NULL,NULL,NULL);
}





point **create_random(int n, bbox *b){

	point **p = (point **)malloc(n*sizeof(point *));
	for (int i=0;i<n;i++){
		p[i] = (point *)malloc(sizeof(point));
		p[i]->x =(double)rand() / (double)RAND_MAX ;
		p[i]->y =(double)rand() / (double)RAND_MAX ;
		p[i] = unit_to_bbox(p[i],b);
	}
	return p;
}

point **create_gaussian(int n, bbox *b, double sigma){
	gsl_rng *g;
	gsl_rng_env_setup();
       	g = gsl_rng_alloc(gsl_rng_default);
	point **p = (point **)malloc(n*sizeof(point *));
	double x;
	double y;
	for (int i=0;i<n;i++){
		p[i] = (point *)malloc(sizeof(point));
		int flag = 0;
		while (flag == 0) {
    			x = gsl_ran_gaussian(g, sigma) + 0.5;
    			y = gsl_ran_gaussian(g, sigma) + 0.5;
    			if (x > 0 && x < 1 && y > 0 && y < 1) {
				//Tries again if the points are out of range
				//Very inefficient, but it works
				//
        			p[i]->x = x;
        			p[i]->y = y;
				p[i] = unit_to_bbox(p[i],b);
        			flag = 1;
    				}
			}
		}
	printf("p[67] is %f %f\n", p[67]->x, p[67]->y);
	printf("p[68] is %f %f\n", p[68]->x, p[68]->y);

	return p;
}

//TODO: add functionality for clustered data, and so forth

sqlite3 *create_db(char *name, char *tabname, point **points, int pnum){

	//Creates a new SQLITE database with the given points
	sqlite3 *db;
	char meta_sql[256];
    	gaiaGeomCollPtr geo = NULL;
    	unsigned char *blob;
    	int blob_size;
    	int pk;
	int flag = 0;
	char fi[64];
	snprintf(fi,sizeof(fi),"data/%s.sqlite",name);

	
    if (file_exists(name)) {flag = 1;}

	
	sqlite3_open_v2 (fi, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	void *cache = spatialite_alloc_connection();
	spatialite_init_ex (db, cache, 0);
	if (flag == 1){return db;}
	 
	//creates table
	char init_sql[256];
	snprintf(init_sql,sizeof(init_sql),"CREATE TABLE %s (ogc_fid INTEGER PRIMARY KEY AUTOINCREMENT)",tabname);
	if(sqlite3_exec(db,init_sql,NULL,NULL,NULL)!=SQLITE_OK){printf("err: %s\n", sqlite3_errmsg(db));}
	//initialises spatial metadata
	sqlite3_exec (db, "SELECT InitSpatialMetadata(1)", NULL, NULL, NULL);
	char add_sql[256];
    	snprintf(add_sql,sizeof(add_sql),"SELECT AddGeometryColumn(\'%s\', 'cent', 3857, 'POINT', 2)",tabname);
    	if(sqlite3_exec(db, add_sql, NULL, NULL, NULL)!=SQLITE_OK){printf("err2: %s\n", sqlite3_errmsg(db));}
	//copies each point in
	if(sqlite3_exec(db,"BEGIN",NULL,NULL,NULL)!=SQLITE_OK){printf("err3: %s\n", sqlite3_errmsg(db));}

	char ins_sql[256];
	snprintf(ins_sql,sizeof(ins_sql),"INSERT INTO %s (cent) VALUES (MakePoint(?,?,3857))",tabname);
	sqlite3_stmt *ins_stmt;
	if(sqlite3_prepare_v2(db,ins_sql,-1,&ins_stmt,NULL)!=SQLITE_OK){printf("err4: %s\n", sqlite3_errmsg(db));}
	for (int i=0;i<pnum;i++){
		sqlite3_bind_double(ins_stmt,1,points[i]->x);
		sqlite3_bind_double(ins_stmt,2,points[i]->y);
		sqlite3_step(ins_stmt);
		sqlite3_reset(ins_stmt);


	}
	sqlite3_finalize(ins_stmt);
	if(sqlite3_exec(db,"COMMIT",NULL,NULL,NULL)!=SQLITE_OK){printf("err3: %s\n", sqlite3_errmsg(db));}
	sqlite3_exec(db,"SELECT UpdateLayerStatistics()",NULL,NULL,NULL);
	printf("CREATED!\n");
	sqlite3_close(db);


}




void make_3857_col(sqlite3 *db, point **p, char *name, int lim){
	//adds data as a geometry column in table 'name'
	sqlite3_stmt *stmt;
	int ret;
	char *err_msg;
    	gaiaGeomCollPtr geo = NULL;
    	unsigned char *blob;
    	int blob_size;
    	int pk;

	char check[256];
	sqlite3_stmt *chk;
	snprintf(check,sizeof(check), "SELECT name FROM PRAGMA table_list WHERE name = %s", name);
	sqlite3_prepare_v2(db,check,-1,&chk,NULL);
	if (sqlite3_step(chk) == SQLITE_ROW){
		sqlite3_finalize(chk);
		return;
	}
	sqlite3_finalize(chk);


	char sql[256];
	snprintf(sql,sizeof(sql), "CREATE TABLE %s (id INTEGER NOT NULL PRIMARY KEY)",name);
	ret = sqlite3_exec (db, sql, NULL, NULL, &err_msg);
	char sql2[256];
    snprintf(sql,sizeof(sql),"SELECT AddGeometryColumn(\'%s\', 'geom', 3857, 'POINT', 2)",name);
    ret = sqlite3_exec (db, sql, NULL, NULL, &err_msg);
 
    strcpy (sql, "BEGIN");
    ret = sqlite3_exec (db, sql, NULL, NULL, &err_msg);
 
 
    snprintf(sql,sizeof(sql),"INSERT INTO %s (id, geom) VALUES (?, ?)", name);
    printf("preparing to insert\n");
    if(sqlite3_prepare_v2(db, sql,-1,&stmt, NULL)!=SQLITE_OK){printf("error %s\n", sqlite3_errmsg(db));}
    pk = 0;
    for (int i = 0; i < lim; i++){
    		pk++;
                geo = gaiaAllocGeomColl();
                geo->Srid = 3857;
                gaiaAddPointToGeomColl (geo, p[i]->x, p[i]->y);
 
                gaiaToSpatiaLiteBlobWkb (geo, &blob, &blob_size);
 
                gaiaFreeGeomColl (geo);
                sqlite3_reset (stmt);
                sqlite3_clear_bindings (stmt);

 
                sqlite3_bind_int64 (stmt, 1, pk);
                sqlite3_bind_blob (stmt, 2, blob, blob_size, free);
 
                if(sqlite3_step(stmt)!=SQLITE_DONE){;}
    }
    sqlite3_finalize (stmt);
 
    strcpy (sql, "COMMIT");
    ret = sqlite3_exec (db, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
          printf ("COMMIT error: %s\n", err_msg);
          sqlite3_free (err_msg);
      }
	sqlite3_exec(db, "SELECT UpdateLayerStatistics()",NULL,NULL,NULL); 
}



void remove_col(sqlite3 *db, char *tab, char *col){
	//Removes a column from the database

	sqlite3_stmt *stmt;
	char sql[256];
	snprintf(sql,sizeof(sql),"ALTER TABLE \'%s\' DROP COLUMN \'%s\'",tab,col);
	sqlite3_exec(db,sql,NULL,NULL,NULL);
	printf("COMPLETED!\n");
}

rule *get_hilbert_curve(){
	//NOTE: i know this is horrible and i'll find a better way of doing this, believe me
	//probably based on reading in from some input file
	rule *r = rule_init(4);
	rule *s = rule_init(4);
	rule *t = rule_init(4);
	rule *u = rule_init(4);
	short int a1[4] = {1,0,2,3};
	short int a2[4] = {2,0,1,3};
	short int a3[4] = {2,3,1,0};
	short int a4[4] = {1,3,2,0};
	rule *a[4] = {r,u,r,s};
	rule *b[4] = {s,s,t,r};
	rule *c[4] = {u,t,s,t};
	rule *d[4] = {t,r,u,u};
	assign_trav(r,a1,4);
	assign_trav(s,a2,4);
	assign_trav(t,a3,4);
	assign_trav(u,a4,4);
	for (int i = 0; i< 4; i++){
	r->next[i] = a[i];
	s->next[i] = b[i];
	t->next[i] = c[i];
	u->next[i] = d[i];
	}
	return r;
}
rule *get_zorder_curve(){
	//As above
	rule *z = rule_init(4);
	short int a[4] = {0,2,1,3};
	assign_trav(z,a,4);
	rule *b[4] = {z,z,z,z};
	for (int i=0;i<4;i++){
	z->next[i] = b[i];
	}
	return z;
}


point **sample_random_points_from_table(sqlite3 *db, char *table, char *col, int n){
	//naive random sampling from a table. 
	//there's probably a more interesting way to do this, but we'll do it later... 
	int k;
	if (n == -1){
		sqlite3_stmt *stmt_2;
		char sql2[256];
		snprintf(sql2,sizeof(sql2), "SELECT COUNT(1) FROM %s",table);
		sqlite3_prepare_v2(db,sql2,-1,&stmt_2,NULL);
		sqlite3_step(stmt_2);
		k = sqlite3_column_int(stmt_2,0);
		sqlite3_finalize(stmt_2);




	} else {k=n;}
	sqlite3_stmt *stmt;
	char sql[256];
	snprintf(sql,sizeof(sql), "SELECT ST_Transform(ST_Centroid(%s),3857) FROM %s WHERE %s IS NOT NULL ORDER BY RANDOM() LIMIT ?", col, table, col);
	if(sqlite3_prepare_v2(db,sql,-1,&stmt,NULL)!=SQLITE_OK){printf("error: %s\n",sqlite3_errmsg(db));}
	sqlite3_bind_int(stmt,1,k);
	point **p = malloc(k*sizeof(point *));
	int i = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW){
		if (sqlite3_column_type(stmt,0) == SQLITE_NULL){
			printf("wow something went wrong!\n");
		} else{
		p[i] = malloc(sizeof(point));
		const void *blob = sqlite3_column_blob(stmt, 0);
	    	int blob_size = sqlite3_column_bytes(stmt, 0);
	    	gaiaGeomCollPtr geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);
	    	gaiaPointPtr pt = geom->FirstPoint;
		p[i]->x = pt->X;
		p[i]->y = pt->Y;
		i++;
		gaiaFreeGeomColl(geom);
		}
	}

	point **q = realloc(p, i*sizeof(point *));
	p = q;


	sqlite3_finalize(stmt);

	for (int j = 0; j<i; j++){
		if (p[j] == NULL){
			printf("wow! a null point!\n");
		}
	}

	printf("got points!\n");
	return p;



}
		


