#include "m2.h"

bbox **read_bboxes_from_file(char *filename, int *bnum){
	//reads the bboxes from a file
	//we structure the line with each float on each side. it's the responsibility of the file writer to make sure that it follows this format
	printf("cluster file is %s\n", filename);
	FILE *f = fopen(filename, "r");
	if (f == NULL){
		printf("wow! error opening the file!\n");
		return NULL;
	}
	bbox **b = malloc(256*sizeof(bbox *));
	//format: one double on each line
	char *line = NULL;
    	size_t len = 0;
    	ssize_t read;
	int count = 0;
    	while ((read = getline(&line, &len, f)) != -1) {
		if (count % 4 == 0){
			bbox *new = malloc(sizeof(bbox));
			b[count/4] = new;
			new->min_x = atof(line);
		} else if (count % 4 == 1){
			b[count/4]->min_y = atof(line);
		} else if (count % 4 == 2){
			b[count/4]->max_x = atof(line);
		} else if (count % 4 == 3){
			b[count/4]->max_y = atof(line);
		}
		count++;
	}
	fclose(f);
	//printf("count is %d\n", count);
	if (count % 4 != 0){
		printf("this file has an invalid number of lines!\n");
		return NULL;
	}
	bbox **temp = realloc(b,(count/4)*sizeof(bbox *));
	b = temp;
	(*bnum) = count/4;
	return b;
    }	

bbox *get_intersect_help(bbox *b, bbox *c){
	//returns the intersection of two bboxes
	//or returns NULL if they don't intersect
	double x1 = b->min_x >= c->min_x ? b->min_x : c->min_x;	
	double x2 = b->max_x <= c->max_x ? b->max_x : c->max_x;
	double y1 = b->min_y >= c->min_y ? b->min_y : c->min_y;
	double y2 = b->max_y <= c->max_y ? b->max_y : c->max_y;
	if (!(x1 <= x2 && y1 <= y2)){return NULL;}
	bbox *ret = malloc(sizeof(bbox));
	ret->min_x = x1;
	ret->min_y = y1;
	ret->max_x = x2;
	ret->max_y = y2;
	return ret;
}

int point_in_bbox(point *p, bbox*b){return ((p->x>=b->min_x && p->x <= b->max_x)&&(p->y>=b->min_y && p->y<=b->max_y));}

int bbox_in_bbox(bbox *s, bbox *l){return (s->min_x >= l->min_x && s->max_x <= l->max_x && s->min_y >= l->min_y && s->max_y <= l->max_y);}

void serialise_node_help(Node2 *n, FILE *f){
	//if you're not using AMD64, then good luck i suppose
	
	if (n == NULL){
		printf("n is null\n");
		fputc(0,f);
		return;
	}
	fputc(1,f);
	//writes out boundaries
	if (n->boundaries == NULL){
			printf("wow what a story!\n");}
	//n->boundaries is a bbox struct containing doubles	
	fwrite(&n->boundaries->min_x,sizeof(double),1,f);
	fwrite(&n->boundaries->min_y,sizeof(double),1,f);
	fwrite(&n->boundaries->max_x,sizeof(double),1,f);
	fwrite(&n->boundaries->max_y,sizeof(double),1,f);
	fwrite(&n->pnum,sizeof(int),1,f);
	fwrite(&n->depth,sizeof(int),1,f);
	fwrite(&n->leaf,sizeof(int),1,f);
	if (n->leaf == 1){
		fwrite(&n->ind,sizeof(int),1,f);
		fwrite(&n->x_med,sizeof(double),1,f);
		fwrite(&n->ly_med,sizeof(double),1,f);
		fwrite(&n->ry_med,sizeof(double),1,f);
		}
	else{

		serialise_node_help(n->children[0],f);
		serialise_node_help(n->children[1],f);
		serialise_node_help(n->children[2],f);
		serialise_node_help(n->children[3],f);
	}
}

void serial_wrapper(Node2 *n, char *filename){
	FILE *f = fopen(filename, "wb");
	serialise_node_help(n,f);
	fclose(f);
}

//TODO: add functions for serialising and deserialising

Node2 *read_node_help(FILE *f){

    int marker = fgetc(f);
    if (marker == EOF) {return NULL;}
	if (marker == 0) {return NULL;}
	bbox *b = malloc(sizeof(bbox));
	fread(&b->min_x, sizeof(double), 1, f);
	fread(&b->min_y, sizeof(double), 1, f);
	fread(&b->max_x, sizeof(double), 1, f);
	fread(&b->max_y, sizeof(double), 1, f);
	Node2 *n = malloc(sizeof(Node2));
	n->boundaries = b;
	fread(&n->pnum,sizeof(int),1,f);
	fread(&n->depth,sizeof(int),1,f);
	fread(&n->leaf,sizeof(int),1,f);

	if (n->leaf == 1){
		fread(&n->ind,sizeof(int),1,f);
		fread(&n->x_med,sizeof(double),1,f);
		fread(&n->ly_med,sizeof(double),1,f);
		fread(&n->ry_med,sizeof(double),1,f);

	}
	else{
		n->children = malloc(4*sizeof(Node2 *));
		n->children[0] = read_node_help(f);
		n->children[1] = read_node_help(f);
		n->children[2] = read_node_help(f);
		n->children[3] = read_node_help(f);
	}
	


	return n;
}

Node2 *read_wrapper(char *filename){
	FILE *f = fopen(filename, "rb");
	Node2 *n = read_node_help(f);
	fclose(f);
	return n;
}

//

Node2 *make_tree_2(point **p, int pnum, bbox *bounds, int min_count, int depth, int *count){
		
		Node2 *n = malloc(sizeof(Node2));
		n->depth = depth;
		n->boundaries = bounds;

		if (pnum <= min_count){
				n->pnum = pnum;
				n->leaf = 1;
				n->ind = *count;
				(*count)++;
				//printf("leaf node! %d\n",n->ind);
				return n;
		}else{
				//we have some points that we can divide, presumably
				//
	n->leaf = 0;
	n->ind = -1;
	n->pnum = -1;
    qsort(p, pnum, sizeof(point *), sort_points_x);

    int mid = pnum / 2;
    point **left = malloc(mid * sizeof(point *));
    point **right = malloc((pnum - mid) * sizeof(point *));
    memcpy(left, &p[0], mid * sizeof(point *));
    memcpy(right, &p[mid], (pnum - mid) * sizeof(point *));

    int lc = mid;
    int rc = pnum - mid;

    double x_med = left[lc - 1]->x; 
    n->x_med = x_med;

    qsort(left, lc, sizeof(point *), sort_points_y);
    qsort(right, rc, sizeof(point *), sort_points_y);

    int bli = lc / 2;      
    int tli = lc - bli;    
    int bri = rc / 2;      
    int tri = rc - bri;    
    double ly_med = left[bli - 1]->y;
    double ry_med = right[bri - 1]->y;

    n->ly_med = ly_med;
    n->ry_med = ry_med;

    bbox *tlb = create_bbox(bounds->min_x, ly_med, x_med, bounds->max_y);  
    bbox *blb = create_bbox(bounds->min_x, bounds->min_y, x_med, ly_med);  
    bbox *trb = create_bbox(x_med, ry_med, bounds->max_x, bounds->max_y);  
    bbox *brb = create_bbox(x_med, bounds->min_y, bounds->max_x, ry_med);
  	n->children = malloc(4*sizeof(Node2 *));	
    n->children[0] = make_tree_2(&left[bli], tli, tlb, min_count, depth + 1, count);
    n->children[1] = make_tree_2(left, bli, blb, min_count, depth + 1, count);
    n->children[2] = make_tree_2(&right[bri], tri, trb, min_count, depth + 1, count);
    n->children[3] = make_tree_2(right, bri, brb, min_count, depth + 1, count);

    free(left);
    free(right);

    return n;
}}



int get_index_node(Node2 *start, point *p, int *partnum, rule *r, int ind_depth){
		int next;
		point *q = malloc(sizeof(point));
		q->x = p->x;
		q->y = p->y;
		Node2 *n = start;
		while (n->leaf != 1){
			if (n == NULL){
				printf("wow!\n");}
        		if (p->x <= n->x_med) {
            			if (p->y <= n->ly_med){next = 1;}else{next=0;}}
				else{if (p->y <= n->ry_med){next = 3;}else{next=2;}}
        	n = n->children[next];
    }
		(*partnum) = n->ind;
		normalise(q,n->boundaries);
		int index = get_index(q,r,ind_depth);
		free(q);
		return index;
}

//partitioning is now weird, since we have to pass in a bunch of other things
//we now have MULTIPLE trees that we're working with, and we'll need logic to decide which partition a thing goes into
//plus side: do this and method 3 is done
//one more week to work on curves, and then we have a dissertation
void add_node_part_help(sqlite3 *db, char *tab, char *col, char *ind, Node2 *n, char *partname){
		if (n->leaf == 1){
				char name[256];
				char sql[256];
				snprintf(name,sizeof(name),"%s_%s_%d",tab,partname,n->ind);
				snprintf(sql,sizeof(sql), "CREATE TABLE %s (ogc_fid INTEGER NOT NULL PRIMARY KEY, %s INTEGER NOT NULL)",name, ind);
				if(sqlite3_exec (db, sql, NULL, NULL, NULL)!=SQLITE_OK){printf("error: %s\n", sqlite3_errmsg(db));}
				//add geometry column
    				snprintf(sql,sizeof(sql),"SELECT AddGeometryColumn(\'%s\', \'%s\', 3857, 'POINT', 2)",name,col);
    				if(sqlite3_exec (db, sql, NULL, NULL, NULL)!=SQLITE_OK){printf("error: %s\n", sqlite3_errmsg(db));}
		}else{
				for (int i=0;i<4;i++){
						add_node_part_help(db,tab,col,ind,n->children[i],partname);
				}
		}
}



void partition_help(sqlite3 *db, char *tab, char *col, char *ind, Node2 *start, rule *r, char *partname, int ind_depth){
	int count = 0;
	add_node_part_help(db,tab,col,ind,start,partname);
	char sql[256];
	snprintf(sql,sizeof(sql),"SELECT ogc_fid, %s FROM %s",col,tab);
	char sql2[256];
	sqlite3_stmt *sel_stmt, *ins_stmt;
	if(sqlite3_prepare_v2(db,sql,-1,&sel_stmt,NULL)!=SQLITE_OK){printf("err1: %s\n", sqlite3_errmsg(db));}
	int id;
    	strcpy (sql, "BEGIN");
    	sqlite3_exec (db, sql, NULL, NULL,NULL);
	point *c = malloc(sizeof(point));
	int partnum = 0;
	int index;
	while (sqlite3_step(sel_stmt) == SQLITE_ROW){
			//iterates over all points, adds to the appropriate partition
			id = sqlite3_column_int(sel_stmt,0);
			const void *blob = sqlite3_column_blob(sel_stmt, 1);
			int blob_size = sqlite3_column_bytes(sel_stmt, 1);
	    		gaiaGeomCollPtr geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);
   			c->x = geom->FirstPoint->X;
    			c->y = geom->FirstPoint->Y;
			index = get_index_node(start,c,&partnum,r,ind_depth);
			snprintf(sql2,sizeof(sql2),"INSERT INTO %s_%s_%d VALUES (?,?,?)",tab,partname,partnum);
			if (sqlite3_prepare_v2(db,sql2,-1,&ins_stmt,NULL)!=SQLITE_OK){printf("err2: %s\n", sqlite3_errmsg(db));}
			sqlite3_bind_int(ins_stmt,1,id);
			sqlite3_bind_int(ins_stmt,2,index);
			sqlite3_bind_blob(ins_stmt,3,blob,blob_size,SQLITE_TRANSIENT);
			sqlite3_step(ins_stmt);
			sqlite3_finalize(ins_stmt);
	}
	sqlite3_finalize(sel_stmt);
    	strcpy (sql, "COMMIT");
    	sqlite3_exec (db, sql, NULL, NULL, NULL);
}
void test_node_help(sqlite3 *db, Node2 *n, int ind_depth){
	rule *r = get_hilbert_curve();
	int a = 0;
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,"SELECT cent FROM large",-1,&stmt,NULL);
	point *c = malloc(sizeof(point));
	int *pcount = malloc(64*sizeof(int));
	for (int i=0;i<64;i++){pcount[i]=0;}
	int count = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW){
			count++;
			const void *blob = sqlite3_column_blob(stmt,0);
			int blob_size = sqlite3_column_bytes(stmt, 0);
	    	gaiaGeomCollPtr geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);
   			c->x = geom->FirstPoint->X;
    		c->y = geom->FirstPoint->Y;
			get_index_node(n,c,&a,r,ind_depth);
			pcount[a]++;
	}
	sqlite3_finalize(stmt);
	for (int i=0;i<64;i++){
			//printf("part %d has %d\n", i, pcount[i]);
	}
	//printf("count is %d\n", count);
}

void range_4_help(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, bbox *query, int *found, int *checked,Node2 *n, rule *base, char *partname, int ind_depth){
	bbox *intersect = get_intersect_help(query,n->boundaries);
	if (intersect == NULL){
		return;
	}
	if (n->leaf != 1){
		//performs range queries on child nodes
		free(intersect);
		for (int qwerty=0;qwerty<4;qwerty++){
		range_4_help(db,tab,col,ind,x,y,rad,query,found,checked,n->children[qwerty],base,partname,ind_depth);
		}

	}else{
		double rad2 = rad*rad;

		//gets name of partition for query use
		char name[256];
		snprintf(name,sizeof(name),"%s_%s_%d", tab, partname, n->ind);
		//printf("current node has boundaries %f %f %f %f\n", n->boundaries->min_x, n->boundaries->min_y, n->boundaries->max_x, n->boundaries->max_y);
		//printf("%s\n", name);
		//gets index ranges required for query
		normalise_bbox(intersect,n->boundaries);
		//print_bbox(intersect);
		sqlite3_stmt *sel_stmt;
		if (intersect->min_x == 0 && intersect->max_x == 1 && intersect->min_y == 0 && intersect->max_y == 1){
			//if the entire partition is covered, then we get everything we can!
			char s2[256];
			snprintf(s2,sizeof(s2),"SELECT ogc_fid, %s FROM %s",col, name); //TODO: this is hardcoded!
			if(sqlite3_prepare_v2(db,s2,-1,&sel_stmt,NULL)!=SQLITE_OK){printf("err111: %s\n", sqlite3_errmsg(db));}
			while (sqlite3_step(sel_stmt) == SQLITE_ROW){
				(*checked)++;
	    			const void *blob = sqlite3_column_blob(sel_stmt, 1);
	    			int blob_size = sqlite3_column_bytes(sel_stmt, 1);
	    			gaiaGeomCollPtr geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);
	    			gaiaPointPtr pt = geom->FirstPoint;
	    			//Euclidean distance calculation, this relies on us being in EPSG3857
				double dx = x-pt->X;
				double dy = y-pt->Y;
				double dist2 = (dx*dx + dy*dy);
				if (dist2 < rad2){
					//printf("id %d x: %f y: %f\n",sqlite3_column_int(sel_stmt,0),pt->X,pt->Y);
					(*found)++;
					}
				gaiaFreeGeomColl(geom);
				}
			sqlite3_finalize(sel_stmt);
			free(intersect);
			return;	
		} else{
			rangelist *rl = get_ranges_2(intersect,base,ind_depth);
			char s3[25600];
			char tmp[256];
			snprintf(s3, sizeof(s3), "SELECT %s, ogc_fid FROM %s WHERE (%s BETWEEN %d AND %d) ",col,name,ind,rl->ranges[0]->start, rl->ranges[0]->end);
			for (int j=1;j<rl->len;j++){
			snprintf(tmp,sizeof(tmp),"OR (%s BETWEEN %d AND %d) ", ind, rl->ranges[j]->start, rl->ranges[j]->end);
			strcat(s3,tmp);
			}
			if(sqlite3_prepare_v2(db,s3,-1,&sel_stmt,NULL)!=SQLITE_OK){printf("err4:%s\n", sqlite3_errmsg(db));}
			//selects all points from candidates
			while (sqlite3_step(sel_stmt) == SQLITE_ROW){
				(*checked)++;
				const void *blob = sqlite3_column_blob(sel_stmt, 0);
				int blob_size = sqlite3_column_bytes(sel_stmt, 0);
				gaiaGeomCollPtr geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);
				gaiaPointPtr pt = geom->FirstPoint;
				//Euclidean distance calculation, this relies on us being in EPSG3857
				double dx = x-pt->X;
				double dy = y-pt->Y;
				double dist2 = (dx*dx + dy*dy);
				if (dist2 < rad2){
					//printf("id %d x: %f y: %f\n",sqlite3_column_int(sel_stmt,1),pt->X,pt->Y);
					(*found)++;
					}
			    gaiaFreeGeomColl(geom);
		}
		sqlite3_finalize(sel_stmt);
		free(intersect);
		free_rangelist(rl);
		return ;
		}
	}
}

double range_wrapper_help(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, int limit, Node2 *n, rule *base, char *partname, int ind_depth){
	//computes range query box
	bbox *b = malloc(sizeof(bbox));
	b->min_x = x-rad;
	b->max_x = x+rad;
	b->min_y = y-rad;
	b->max_y = y+rad;
	//traverses quad tree, performs separate query for each node:
	//endforces limit
	int found = 0;
	int checked = 0;
	range_4_help(db,tab,col,ind,x,y,rad,b,&found,&checked,n,base,partname,ind_depth);
	printf("%d points found\n", found);
	free(b);

	if (found == 0 || checked == 0){
		return (double)(-1);
	}
	return (double)found/(double)checked;

}

//now to write test functions, and hope it all works i suppose!
