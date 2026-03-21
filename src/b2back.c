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

	printf("found %d points!%\n", found);

	
       		


}

