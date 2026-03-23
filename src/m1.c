#include "m1.h"

//METHOD 1: we have a series of index ranges. this is alright, because we can just compute index ranges and then go from there
//this is not very clever at the moment. in particular, we need a better way of determining which partitions we need for a given range query,
//and we need a better way of actually selecting the ranges
//(code for that is in grid.c, by the way)
// i have other things (grid-based) to work on, so that'll probably be it for now (as of monday night, when i'm writing this)
//

void destroy_part_structure(sqlite3 *db, char *tab, int part_no){
	printf("destroying parts!\n");
	char sql[64];
	snprintf(sql,sizeof(sql),"DROP TABLE %s_parts_ind", tab);
	sqlite3_exec(db,sql,NULL,NULL,NULL);
	for (int i=0;i<part_no;i++){
		snprintf(sql,sizeof(sql), "SELECT DiscardGeometryColumn('%s_part_%d, 'geom')", tab, i);
		sqlite3_exec(db,sql,NULL,NULL,NULL);

		snprintf(sql,sizeof(sql), "DROP TABLE %s_part_%d", tab, i);
		sqlite3_exec(db,sql,NULL,NULL,NULL);
	}
	//printf("partitioning destroyed!\n");
}

void partition_col_by_index_ranges(sqlite3 *db, char *tab, char *col,char *ind, rangelist *ranges, rule *base, int ind_depth){

	//TODO: add some metadata column to track the ranges stored by each partition or something
	
	char sql[256];
	char name[64];
	char ins[256];
	char sel[256];
	int ret;
	int index;
	point *c = (point *)malloc(sizeof(point));
	bbox *b = get_db_boundaries(db,tab,col);

	for (int i = 0; i<ranges->len;i++){
		snprintf(name,sizeof(name),"%s_part_%d", tab, i);
	//create table
	snprintf(sql,sizeof(sql), "CREATE TABLE %s (ogc_fid INTEGER NOT NULL PRIMARY KEY)",name);
	ret = sqlite3_exec (db, sql, NULL, NULL, NULL);
	//add geometry column
    	snprintf(sql,sizeof(sql),"SELECT AddGeometryColumn(\'%s\', 'geom', 3857, 'POINT', 2)",name);
    	ret = sqlite3_exec (db, sql, NULL, NULL, NULL);
	//add index column:
	snprintf(sql, sizeof(sql), "ALTER TABLE %s ADD %s INT NULL", name, ind);
	ret = sqlite3_exec(db,sql,NULL,NULL,NULL);


		//this should handle creating and registering the column
	}

	//now we take each point, compute index value, and put it in the appropriate partition
    	strcpy (sql, "BEGIN");
    	sqlite3_exec (db, sql, NULL, NULL,NULL);

    	sqlite3_stmt *sel_stmt;
    	sqlite3_stmt *ins_stmt;
	snprintf(sel,sizeof(sel),"SELECT ogc_fid, ST_Centroid(%s) FROM %s WHERE %s NOT NULL", col, tab, col);
	//printf("%s\n", sel);
	sqlite3_prepare_v2(db,sel,-1,&sel_stmt,NULL);

	int k;
	char ch[256];
	while (sqlite3_step(sel_stmt) == SQLITE_ROW){
		int id = sqlite3_column_int(sel_stmt,0);
		const void *blob = sqlite3_column_blob(sel_stmt, 1);
	    	int blob_size = sqlite3_column_bytes(sel_stmt, 1);
	    	gaiaGeomCollPtr geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);
   		c->x = geom->FirstPoint->X;
    		c->y = geom->FirstPoint->Y;
		normalise(c,b);
	    	gaiaFreeGeomColl(geom);
	    	index = get_index(c,base,ind_depth);
		//find the partition that we need
		for (int i = 0; i< ranges->len; i++){
			if (index >= ranges->ranges[i]->start && index <= ranges->ranges[i]->end){
				k = i;
			}
		}
		snprintf(ins,sizeof(ins), "INSERT INTO %s_part_%d VALUES (?,?,?)", tab, k);
		sqlite3_prepare_v2(db,ins,-1,&ins_stmt,NULL);
		sqlite3_bind_int(ins_stmt,1,id);
		sqlite3_bind_blob(ins_stmt,2,blob,blob_size,SQLITE_TRANSIENT);
		sqlite3_bind_int(ins_stmt,3,index);
		sqlite3_step(ins_stmt);
	}
	sqlite3_finalize(ins_stmt);
	sqlite3_finalize(sel_stmt);
    	strcpy (sql, "COMMIT");
    	sqlite3_exec (db, sql, NULL, NULL, NULL);


	sqlite3_stmt *meta;
	snprintf(sql,sizeof(sql),"CREATE TABLE %s_parts_ind (name VARCHAR NOT NULL, start INTEGER NOT NULL, end INTEGER NOT NULL)", tab,tab);
	sqlite3_exec(db,sql,NULL,NULL,NULL);
	snprintf(sql,sizeof(sql),"INSERT INTO %s_parts_ind VALUES (?,?,?)", tab);
	if(sqlite3_prepare_v2(db,sql,-1,&meta,NULL)!=SQLITE_OK){printf("err: can't create index %s\n", sqlite3_errmsg(db));}
	char bx[64];
	for (int i = 0; i < ranges->len; i++) {

    		snprintf(bx, sizeof(bx), "%s_part_%d", tab, i);

    		sqlite3_bind_text(meta, 1, bx, -1, SQLITE_TRANSIENT);
    		sqlite3_bind_int(meta, 2, ranges->ranges[i]->start);
    		sqlite3_bind_int(meta, 3, ranges->ranges[i]->end);

    		sqlite3_step(meta);
    		sqlite3_reset(meta);

		snprintf(ch,sizeof(ch), "CREATE INDEX idx_%s_%d ON %s(%s)",tab,i,bx,ind);
		if(sqlite3_exec(db, ch,NULL,NULL,NULL)!=SQLITE_OK){printf("err: %s\n", sqlite3_errmsg(db));}

	}
}
/*
void partitioned_index_range_query(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, int lim, rule *base, int verbose);
void partitioned_index_range_query(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, int lim, rule *base, int verbose){


	//ASSUMPTIONS:
	//-the original data exists //TODO: fix later
	//the partitions are stored in a database called %tab_parts_ind, which contains the table name for the partition, and start and end index values
	//Makes a GOOD range query on a particular table. 
	clock_t c_start = clock();
	point *p = (point *)malloc(sizeof(point));
	p->x = x;
	p->y = y;
	bbox *world = get_db_boundaries(db,tab,col);
	//start = clock();
	normalise(p,world);
	bbox *b = (bbox *)malloc(sizeof(bbox));
	//TODO: add processing for when we're at poles or dateline
	b->min_x = x-rad;
	b->max_x = x+rad;
	b->min_y = y-rad;
	b->max_y = y+rad;
	normalise_bbox(b, world);
	rangelist *rl = get_ranges(b,base,8); 
	free(b);
	free(world);
	free(p);
	

	//GOAL: find the tables that have the partitions that we're interested in
	//finds the minimum and maximum of rl
	//note: this way is not very good!
	//TODO: find a better implementation
	int min_r = INT_MAX;
	int max_r = 0;
	for (int i = 0; i<rl->len; i++){
		printf("%d, %d\n", rl->ranges[i]->start, rl->ranges[i]->end);
		if (rl->ranges[i]->start < min_r){min_r = rl->ranges[i]->start;}
		if (rl->ranges[i]->end > max_r){max_r = rl->ranges[i]->end;}
	}

	//ok, so we are given two lists of ranges
	//we need to find a subset of list b such that all of list a is contained within
	//naive way: go through everything in list a, and set flags for list b if a[i] intersects with b[j]
	//this is O(mn) where m = |a| and n = |b| unless we find some cleverer way to do it

	char sql[256];
	sqlite3_stmt *stmt, *stmt_2;
	char sql2[256];
	printf("let's go!\n");
	snprintf(sql,sizeof(sql),"CREATE TEMP TABLE IF NOT EXISTS candidates(ogc_fid INTEGER PRIMARY KEY, ind INTEGER, FOREIGN KEY(ogc_fid) REFERENCES %s(ogc_fid))",tab);
	sqlite3_exec(db,sql, NULL, NULL, NULL);
	//gets the metadata for the partition
	snprintf(sql,sizeof(sql), "SELECT start, end, name FROM %s_parts_ind", tab);
	if(sqlite3_prepare_v2(db,sql,-1,&stmt,NULL)!=SQLITE_OK){printf("err 3: %s\n", sqlite3_errmsg(db));}
	int start;
	int end;
	const unsigned char *name;
	printf("min is %d, max is %d\n", min_r, max_r);
	sqlite3_exec(db, "BEGIN", NULL, NULL, NULL);
	while (sqlite3_step(stmt) == SQLITE_ROW){
		start = sqlite3_column_int(stmt,0);
		end = sqlite3_column_int(stmt,1);
		name = sqlite3_column_text(stmt,2);
		printf("%s name, %d %d", name, start, end);
		if (start <= max_r && min_r <= end){
			if (start >= min_r && end <= max_r){snprintf(sql2,sizeof(sql2), "INSERT OR IGNORE INTO CANDIDATES(ogc_fid,ind) SELECT ogc_fid,ind FROM %s", name);}
			else {snprintf(sql2,sizeof(sql2), "INSERT OR IGNORE INTO CANDIDATES(ogc_fid,ind) SELECT ogc_fid,ind FROM %s WHERE ind BETWEEN %d AND %d", name,min_r,max_r);}
			if(sqlite3_prepare_v2(db,sql2,-1,&stmt_2,NULL)!=SQLITE_OK){printf("5 error: %s\n", sqlite3_errmsg(db));}
			sqlite3_step(stmt_2);
			sqlite3_reset(stmt_2);
			sqlite3_finalize(stmt_2);

		}
	}
	sqlite3_finalize(stmt);
	sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
	printf("made temp table\n");

	//TODO: change this to not require the original data

	char s3[256];
	sqlite3_stmt *stmt_3;
	snprintf(s3, sizeof(s3), "SELECT s.%s, s.ogc_fid FROM candidates c JOIN %s s ON c.ogc_fid = s.ogc_fid", col, tab);
	if(sqlite3_prepare_v2(db,s3,-1,&stmt_3,NULL)!=SQLITE_OK){printf("err4:%s\n", sqlite3_errmsg(db));}
	double rad2 = rad*rad;
	while (sqlite3_step(stmt_3) == SQLITE_ROW){
	    const void *blob = sqlite3_column_blob(stmt_3, 0);
	    int blob_size = sqlite3_column_bytes(stmt_3, 0);
	    gaiaGeomCollPtr geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);
	    gaiaPointPtr pt = geom->FirstPoint;
	    //Euclidean distance calculation, this relies on us being in EPSG3857
		double dx = x-pt->X;
		double dy = y-pt->Y;
		double dist2 = (dx*dx + dy*dy);
		if (dist2 < rad2){
			printf("id %d x: %f y: %f\n",sqlite3_column_int(stmt_3,1),pt->X,pt->Y);
		}
	    gaiaFreeGeomColl(geom);
	}
	printf("completed!\n");
	sqlite3_finalize(stmt_3);
	sqlite3_exec(db, "DELETE FROM candidates", NULL, NULL, NULL);
	clock_t c_end = clock();
	printf("time taken to execute query: %f\n",(double)(c_end-c_start)/CLOCKS_PER_SEC);
}
*/

int i_max(int a, int b){
	if (a >= b){
		return a;
	} return b;
}

int i_min(int a, int b){
	if (a < b){
		return a;
	} return b;
}

void beter15(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, int lim, rule *base, int verbose, int ind_depth){
	point *p = (point *)malloc(sizeof(point));
	p->x = x;
	p->y = y;
	bbox *world = get_db_boundaries(db,tab,col);
	normalise(p,world);
	bbox *b = (bbox *)malloc(sizeof(bbox));
	//TODO: add processing for when we're at poles or dateline
	b->min_x = x-rad;
	b->max_x = x+rad;
	b->min_y = y-rad;
	b->max_y = y+rad;
	normalise_bbox(b, world);
	rangelist *rl = get_ranges(b,base,ind_depth); 
		
	char r_sql[256];
	sqlite3_stmt *r_stmt;
	snprintf(r_sql,sizeof(r_sql),"SELECT start, end FROM %s_parts_ind", tab);
	if(sqlite3_prepare_v2(db,r_sql,-1,&r_stmt,NULL)!=SQLITE_OK){printf("err: %s\n", sqlite3_errmsg(db));}
	rangelist *dbr = malloc(sizeof(rangelist));
	int p_count = 0;
	dbr->ranges = malloc(1024*sizeof(range *));
	while (sqlite3_step(r_stmt)==SQLITE_ROW){
		range *newr = malloc(sizeof(range));
		newr->start = sqlite3_column_int(r_stmt,0);
		newr->end = sqlite3_column_int(r_stmt,1);
		dbr->ranges[p_count] = newr;
		p_count++;
	}
	printf("rl len is %d\n", rl->len);
	printf("p_count is %d\n", p_count);
	range **new_r = realloc(dbr->ranges,p_count*sizeof(range *));
	dbr->ranges = new_r;
	dbr->len = p_count;
	//now we do the range check
	//
	int *parts = malloc(p_count*sizeof(int));
	int r_count = 0;
	int a1 = 0;
	int a2 = 0;
	while (a1 < rl->len && a2 < dbr -> len){
		if (rl->ranges[a1]->start <= dbr->ranges[a2]->end && dbr->ranges[a2]->start <= rl->ranges[a1]->end){
			parts[r_count] = a2;
			r_count++;
		} 
		if (rl->ranges[a1]->end <= dbr->ranges[a2]->end){
			a1++;
		} else{a2++;}
	}
	int *parts_temp = realloc(parts,r_count*sizeof(int));
	parts = parts_temp;	

	//we have the ranges now, somehow
	int found= 0;
	char sql[256];
	sqlite3_stmt *get_stmt;
	int id;
	double rad2 = rad * rad;
	for (int i =0;i<r_count;i++){
		snprintf(sql,sizeof(sql),"SELECT ogc_fid, geom FROM %s_part_%d", tab, parts[i]);
		sqlite3_prepare_v2(db,sql,-1,&get_stmt,NULL);
		while (sqlite3_step(get_stmt)==SQLITE_ROW){
			id = sqlite3_column_int(get_stmt, 0);
	    		const void *blob = sqlite3_column_blob(get_stmt, 1);
	    		int blob_size = sqlite3_column_bytes(get_stmt, 1);
	    		gaiaGeomCollPtr geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);
	    		gaiaPointPtr pt = geom->FirstPoint;
			double dx = x-pt->X;
			double dy = y-pt->Y;
			double dist2 = (dx*dx + dy*dy);
			if (dist2 < rad2){
				//printf("id %d x: %f y: %f\n",id, pt->X,pt->Y);
				found++;
			}
	    		gaiaFreeGeomColl(geom);
		}
		sqlite3_finalize(get_stmt);
	}

	printf("found %d points!%\n", found);

	
       		


}
void beter2(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, int lim, rule *base, int verbose, int ind_depth){
	point *p = (point *)malloc(sizeof(point));
	p->x = x;
	p->y = y;
	bbox *world = get_db_boundaries(db,tab,col);
	normalise(p,world);
	bbox *b = (bbox *)malloc(sizeof(bbox));
	//TODO: add processing for when we're at poles or dateline
	b->min_x = x-rad;
	b->max_x = x+rad;
	b->min_y = y-rad;
	b->max_y = y+rad;
	normalise_bbox(b, world);
	rangelist *rl = get_ranges(b,base,ind_depth); 
	for (int i=0;i<rl->len;i++){
		printf("range is %d %d\n", rl->ranges[i]->start, rl->ranges[i]->end);
	}
		
	char r_sql[256];
	sqlite3_stmt *r_stmt;
	snprintf(r_sql,sizeof(r_sql),"SELECT start, end FROM %s_parts_ind", tab);
	if(sqlite3_prepare_v2(db,r_sql,-1,&r_stmt,NULL)!=SQLITE_OK){printf("err: %s\n", sqlite3_errmsg(db));}
	rangelist *dbr = malloc(sizeof(rangelist));
	int p_count = 0;
	dbr->ranges = malloc(1024*sizeof(range *));
	while (sqlite3_step(r_stmt)==SQLITE_ROW){
		range *newr = malloc(sizeof(range));
		newr->start = sqlite3_column_int(r_stmt,0);
		newr->end = sqlite3_column_int(r_stmt,1);
		dbr->ranges[p_count] = newr;
		p_count++;
	}
	printf("rl len is %d\n", rl->len);
	printf("p_count is %d\n", p_count);
	range **new_r = realloc(dbr->ranges,p_count*sizeof(range *));
	dbr->ranges = new_r;
	dbr->len = p_count;
	//now we do the range check
	//
	sqlite3_finalize(r_stmt);
	int *parts = malloc(p_count*sizeof(int));
	int r_count = 0;
	int a1 = 0;
	int a2 = 0;

	rangelist *part_ranges = malloc(sizeof(rangelist));
	part_ranges->ranges = malloc(p_count*sizeof(range *));
	int aptr = 0;
	for (int i=0;i<p_count;i++){
		int first = -1;
		int last = -1;
		while (aptr < rl->len && rl->ranges[aptr]->end < dbr->ranges[i]->start){
			aptr++;
		}
		int ta = aptr;
		while (ta < rl->len && rl->ranges[ta]->start <= dbr->ranges[i]->end){
			int is = i_max(dbr->ranges[i]->start, rl->ranges[ta]->start);
			int ie = i_min(dbr->ranges[i]->end, rl->ranges[ta]->end);
			if (is <= ie){
				if (first == -1){first = is;}
				last = ie;
			}
			ta++;
		}
		if (first != -1){
			parts[r_count] = i;
			range *tr = malloc(sizeof(range));
			tr->start = first;
			tr->end = last;
			part_ranges->ranges[r_count] = tr;
			r_count++;
		}
	}



	//reallocates memory and such, just for shortening
	int *parts_temp = realloc(parts,r_count*sizeof(int));
	parts = parts_temp;
	range **rtemp = realloc(part_ranges->ranges, r_count*sizeof(range *));
	part_ranges->ranges = rtemp;
	part_ranges->len = r_count;

	//we have the ranges now, somehow
	int found= 0;
	char sql[256];
	sqlite3_stmt *get_stmt;
	int id;
	double rad2 = rad * rad;
	for (int i =0;i<r_count;i++){
		printf("part is %d\n", i);
		printf("range is %d %d\n", part_ranges->ranges[i]->start, part_ranges->ranges[i]->end);
		snprintf(sql,sizeof(sql),"SELECT ogc_fid, geom FROM %s_part_%d WHERE %s BETWEEN ? AND ?", tab, parts[i], ind);
		sqlite3_prepare_v2(db,sql,-1,&get_stmt,NULL);
		sqlite3_bind_int(get_stmt,1,part_ranges->ranges[i]->start);
		sqlite3_bind_int(get_stmt,2,part_ranges->ranges[i]->end);
		while (sqlite3_step(get_stmt)==SQLITE_ROW){
			id = sqlite3_column_int(get_stmt, 0);
	    		const void *blob = sqlite3_column_blob(get_stmt, 1);
	    		int blob_size = sqlite3_column_bytes(get_stmt, 1);
	    		gaiaGeomCollPtr geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);
	    		gaiaPointPtr pt = geom->FirstPoint;
			double dx = x-pt->X;
			double dy = y-pt->Y;
			double dist2 = (dx*dx + dy*dy);
			if (dist2 < rad2){
				//printf("id %d x: %f y: %f\n",id, pt->X,pt->Y);
				found++;
			}
	    		gaiaFreeGeomColl(geom);
		}
		sqlite3_finalize(get_stmt);
	}

	//frees memory and such
	
	printf("found %d points!%\n", found);

	
       		


}
void beter(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, int lim, rule *base, int verbose, int ind_depth){
	///this seems to be about twice as fast as the other method
	///i don't know why
	//clock_t c_start = clock();
	point *p = (point *)malloc(sizeof(point));
	p->x = x;
	p->y = y;
	bbox *world = get_db_boundaries(db,tab,col);
	print_bbox(world);
	//start = clock();
	normalise(p,world);
	bbox *b = (bbox *)malloc(sizeof(bbox));
	//TODO: add processing for when we're at poles or dateline
	b->min_x = x-rad;
	b->max_x = x+rad;
	b->min_y = y-rad;
	b->max_y = y+rad;
	normalise_bbox(b, world);
	print_bbox(b);

	rangelist *rl = get_ranges(b,base,ind_depth); 
	//TODO: better range approximation
	//printf("%d %d\n", rl->ranges[0]->start, rl->ranges[0]->end);

	free(b);
	free(world);
	free(p);
	printf("rl->len is %d\n", rl->len);
	int min_r = INT_MAX;
	int max_r = 0;
	for (int i = 0; i<rl->len; i++){
		printf("%d, %d\n", rl->ranges[i]->start, rl->ranges[i]->end);
		if (rl->ranges[i]->start <= min_r){min_r = rl->ranges[i]->start;}
		if (rl->ranges[i]->end >= max_r){max_r = rl->ranges[i]->end;}
	}
	//TODO: better way of approximating ranges
	//we could just iterate over the db and check if the db range intersects anything in the query list
	//but that's O(mn) in the number of partitions and the number of ranges
	//still, i don't think it'll be that expensive, probably
	//printf("min_r %d max_r %d\n", min_r, max_r);
	double rad2 = rad * rad;	
	char sql[256];
	sqlite3_stmt *stmt, *stmt_2;
	char sql2[256];
	snprintf(sql,sizeof(sql), "SELECT DISTINCT start, end, name FROM %s_parts_ind WHERE start <= %d AND %d <= end", tab,max_r,min_r);
	if(sqlite3_prepare_v2(db,sql,-1,&stmt,NULL)!=SQLITE_OK){printf("err: %s\n", sqlite3_errmsg(db));}
	int start;
	int end;
	char *name;
	int id;
	//iterates over partitions
	int found = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW){
		start = sqlite3_column_int(stmt,0);
		end = sqlite3_column_int(stmt,1);
		name = sqlite3_column_text(stmt,2);
		//printf("name is %s\n", name);
		//if we intersect!
		if (start >= min_r && end <= max_r){snprintf(sql2,sizeof(sql2), "SELECT DISTINCT ogc_fid, geom FROM %s", name);}
		else {snprintf(sql2,sizeof(sql2), "SELECT DISTINCT ogc_fid,geom FROM %s WHERE %s BETWEEN %d AND %d", name,ind,min_r,max_r);}
		sqlite3_prepare_v2(db,sql2,-1,&stmt_2,NULL);
		while (sqlite3_step(stmt_2) == SQLITE_ROW){
			id = sqlite3_column_int(stmt_2, 0);
	    		const void *blob = sqlite3_column_blob(stmt_2, 1);
	    		int blob_size = sqlite3_column_bytes(stmt_2, 1);
	    		gaiaGeomCollPtr geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);
	    		gaiaPointPtr pt = geom->FirstPoint;
			double dx = x-pt->X;
			double dy = y-pt->Y;
			double dist2 = (dx*dx + dy*dy);
			if (dist2 < rad2){
				//printf("id %d x: %f y: %f\n",id, pt->X,pt->Y);
				found++;
			}
	    	gaiaFreeGeomColl(geom);
		}
		sqlite3_reset(stmt_2);
		sqlite3_finalize(stmt_2);
		}
	sqlite3_finalize(stmt);
	clock_t c_end = clock();
	//printf("time taken for part query: %f\n", (double)(c_end-c_start)/CLOCKS_PER_SEC);
	printf("points found: %d\n", found);
	}



//This needs to be refactored for higher index depths
unsigned int *get_all_indices(sqlite3 *db, char *tab, char *ind, int depth, int grouping, int sample, int *count){
	char sql[256];
	if (sample == 0){
	snprintf(sql,sizeof(sql),"SELECT %s FROM %s WHERE %s IS NOT NULL", ind, tab, ind);
	} else{
		snprintf(sql,sizeof(sql), "SELECT %s FROM %s WHERE %s IS NOT NULL ORDER BY RANDOM() LIMIT %d", ind, tab, ind, sample);
	}
	unsigned int *indices = malloc((pow(4,depth)/grouping)*sizeof(unsigned int));
	for (int i=0;i<(pow(4,depth)/grouping);i++){
		indices[i] = 0;
	}
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
       	while (sqlite3_step(stmt) == SQLITE_ROW){
		int index = sqlite3_column_int(stmt,0);
		indices[index/grouping]++;
		(*count)++;
	}
	sqlite3_finalize(stmt);
	return indices;
}



