#include "m3.h"


Node2 **make_trees(point **points, int pnum, bbox *bounds, bbox **clusters, int cluster_count, int b_minp, int c_minp){
//We're doing a new approach
//we get some clusters from python xD
//then we treat each of those as a separate tree(not very deep), then make a tree with the rest of the area minus the points in the clusters
//to make a range query, we check if we're in the clusters and query those if necessary
//then we compute the XOR of the two queries 
//from then, it's easy: we can call the regular functions with the trees and such
//
	point ***p = malloc((cluster_count+1)*sizeof(point **));
	//printf("cluster count is %d\n", cluster_count);
	int pcount[cluster_count+1];
	for (int i = 0;i<cluster_count+1;i++){p[i] = malloc(pnum*sizeof(point *));pcount[i]=0;} //we can and will realloc later #this line is overflowing somehow when c_count > 1
	for (int i=0;i<pnum;i++){
		int flag = 0;
		for (int j=0;j<cluster_count;j++){
			if (point_in_bbox(points[i],clusters[j]) && (flag==0)){
				//adds point to p[i], increments pcount
				p[j+1][pcount[j+1]] = points[i];
				pcount[j+1]++;
				flag = 1;
			}
		}
		if (flag == 0){
		p[0][pcount[0]] = points[i];
		pcount[0]++; //0 is the index for any point not in a cluster!
		}
	}
	//now we make trees to each their own
	Node2 **n = malloc((cluster_count+1)*sizeof(Node2 *));
	int count = 0;
	n[0] = make_tree_2(p[0],pcount[0],bounds,b_minp,0,&count);
	for (int j=0;j<pcount[0];j++){free(p[0][j]);}
	free(p[0]);
	for (int i=1;i<cluster_count+1;i++){
		count = 0;
		n[i] = make_tree_2(p[i],pcount[i],clusters[i-1],c_minp,0,&count);
		for (int j=0;j<pcount[i];j++){free(p[i][j]);}
		free(p[i]);
	}
	//now we do partitioning lark
	free(p);
	printf("tree\n");
	return n; 
}
void partition_through_multiple_trees(sqlite3 *db, char *tab, char *col, char *ind, Node2 **n, char *partname, int c_count, rule *r,int base_depth, int c_depth){

	//makes database tables
	//printf("c_count is %d\n", c_count);
	char name[256];
	for (int i=0;i<c_count+1;i++){
		snprintf(name,sizeof(name),"%s_%d",partname,i);
		//printf("%s\n", name);
		add_node_part_help(db,tab,col,ind,n[i],name);
	}
	printf("added tables!\n");
	int count = 0;
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
	Node2 *start;
	int cnum;
	while (sqlite3_step(sel_stmt) == SQLITE_ROW){
			//iterates over all points, adds to the appropriate partition
			id = sqlite3_column_int(sel_stmt,0);
			const void *blob = sqlite3_column_blob(sel_stmt, 1);
			int blob_size = sqlite3_column_bytes(sel_stmt, 1);
	    		gaiaGeomCollPtr geom = gaiaFromSpatiaLiteBlobWkb(blob, blob_size);
   			c->x = geom->FirstPoint->X;
    			c->y = geom->FirstPoint->Y;
			cnum = 0;
			int d = base_depth;
			for (int i=1;i<c_count+1;i++){
				if (point_in_bbox(c,n[i]->boundaries)){
					cnum = i;
					d = c_depth;
					break;
				}
			}
			start = n[cnum];
			//gets the partition that we need!

			index = get_index_node(start,c,&partnum,r,d);
			snprintf(sql2,sizeof(sql2),"INSERT INTO %s_%s_%d_%d VALUES (?,?,?)",tab,partname,cnum,partnum);
			if (sqlite3_prepare_v2(db,sql2,-1,&ins_stmt,NULL)!=SQLITE_OK){printf("err2: %s\n", sqlite3_errmsg(db));}
			sqlite3_bind_int(ins_stmt,1,id);
			sqlite3_bind_int(ins_stmt,2,index);
			sqlite3_bind_blob(ins_stmt,3,blob,blob_size,SQLITE_TRANSIENT);
			sqlite3_step(ins_stmt);
			sqlite3_finalize(ins_stmt);
			gaiaFreeGeomColl(geom);
	}
    	strcpy (sql, "COMMIT");
    	sqlite3_exec (db, sql, NULL, NULL, NULL);
}
void range_4_help_3(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, bbox *query, int *found, int *checked, Node2 *n, rule *base, char *partname, int c_count, bbox **clusters, int partno, int ind_depth){
	bbox *intersect = get_intersect_help(query,n->boundaries);
	if (intersect == NULL){return;}
	if (n->leaf != 1){
		for (int qwerty=0;qwerty<4;qwerty++){
		//performs range queries on child nodes
		range_4_help_3(db,tab,col,ind,x,y,rad,query,found,checked,n->children[qwerty],base,partname,c_count,clusters,partno,ind_depth);
		}

	}else{
		if (clusters){
			for (int i = 0;i<c_count;i++){
				bbox *q2 = get_intersect_help(n->boundaries,query);
				if (bbox_in_bbox(q2, clusters[i])){return;}
			}
		}
		double rad2 = rad*rad;

		//gets name of partition for query use
		char name[64];
		snprintf(name,sizeof(name),"%s_%s_%d_%d", tab, partname, partno, n->ind);
		//printf("current node has boundaries %f %f %f %f\n", n->boundaries->min_x, n->boundaries->min_y, n->boundaries->max_x, n->boundaries->max_y);
		//printf("%s\n", name);
		//gets index ranges required for query
		normalise_bbox(intersect,n->boundaries);
		print_bbox(intersect);
		sqlite3_stmt *sel_stmt;
		if (intersect->min_x == 0 && intersect->max_x == 1 && intersect->min_y == 0 && intersect->max_y == 1){
			//if the entire partition is covered, then we get everything we can!
			char s2[256];
			snprintf(s2,sizeof(s2),"SELECT ogc_fid, %s FROM %s",col, name); //TODO: this is hardcoded!
			if(sqlite3_prepare_v2(db,s2,-1,&sel_stmt,NULL)!=SQLITE_OK){printf("err99: %s\n", sqlite3_errmsg(db));}
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
	sqlite3_exec(db, "DELETE FROM candidates", NULL, NULL, NULL);
	}
	}
}


double method_3_wrapper(sqlite3 *db, char *tab, char *col, char *ind, double x, double y, double rad, int limit, Node2 **n, rule *base, char *partname, int c_count, int base_depth, int c_depth){
	//for trees, we expect that index 0 is global, with the other indices being the clusters
	//does cluster searches, and then does global search
	bbox *b = malloc(sizeof(bbox));
	b->min_x = x-rad;
	b->max_x = x+rad;
	b->min_y = y-rad;
	b->max_y = y+rad;
	bbox **clusters = malloc(c_count*sizeof(bbox *));
	int found = 0;
	int checked = 0;
	for (int i =0;i<c_count;i++){
		clusters[i] = n[i+1]->boundaries;
		range_4_help_3(db,tab,col,ind,x,y,rad,b,&found,&checked,n[i+1],base,partname,0,NULL,i+1,c_depth);
	}
	range_4_help_3(db,tab,col,ind,x,y,rad,b,&found,&checked,n[0],base,partname,c_count,clusters,0,base_depth);
	printf("found %d points:\n", found);
	//printf("COMPLETED!\n");
	free(clusters);
	free(b);
	if (found == 0 || checked == 0){
		return (double)(-1);
	}
	return (double)found/(double)checked;
}

