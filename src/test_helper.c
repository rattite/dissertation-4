#include "test_helper.h"



void reset_db(sqlite3 *db, char *tab){
	//removes all tables and columns aside from ogc_fid,GEOMETRY,cent in whatever the main name is
	//
	printf("%s\n", tab);
	sqlite3_stmt *stmt;
	const char *sql = "SELECT f_table_name, f_geometry_column FROM geometry_columns";
	if (sqlite3_prepare_v2(db,sql,-1,&stmt,NULL)!=SQLITE_OK){printf("error: %s\n", sqlite3_errmsg(db));}
	char **tables = malloc(1000*sizeof(char *));
	char **cols = malloc(1000*sizeof(char *));
	int count = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW){
		const char *t = sqlite3_column_text(stmt,0);
		const char *c = sqlite3_column_text(stmt,1);
		if (strcmp(t,tab) == 0){
			continue;
		}
		else{
			tables[count] = strdup(t);
			cols[count] = strdup(c);
			count++;
		}
	}
	sqlite3_finalize(stmt);
	//reallocates memory
	tables = realloc(tables,count*sizeof(char *));
	cols = realloc(cols,count*sizeof(char *));
	sqlite3_exec(db,"BEGIN",NULL,NULL,NULL);
	char sql2[256];
	for (int i=0;i<count;i++){
		//printf("tables[i] is %s\n", tables[i]);
		//we have a list of columns and the associated tables
		//none of them should be in the table we choose, so we're free to blankedly remove everything else!
		snprintf(sql2,sizeof(sql2),"SELECT DropTable(NULL,'%s')",tables[i]);
		if(sqlite3_exec(db,sql2,NULL,NULL,NULL)!=SQLITE_OK){printf("err2: %s\n", sqlite3_errmsg(db));}
		free(tables[i]);
		free(cols[i]);
	}
	sqlite3_exec(db,"COMMIT",NULL,NULL,NULL);
	sqlite3_exec(db, "SELECT UpdateLayerStatistics()",NULL,NULL,NULL);

	//frees memory
	free(tables);
	free(cols);
	//printf("COMPLETED! reset\n");
}

void remove_index_col(sqlite3 *db, char *tab, char *col){
	char sql[256];
	snprintf(sql,sizeof(sql), "ALTER TABLE %s DROP COLUMN %s", tab, col);
	sqlite3_exec(db,sql,NULL,NULL,NULL);
	//printf("removed index!\n");
}


query **read_queries_from_file(char *filename, int *qnum){
	FILE *f = fopen(filename, "r");
	if (f == NULL){
		printf("wow! error opening the file!\n");
		return NULL;
	}
	query **q = malloc(256*sizeof(query *));
	//format: one double on each line
	char *line = NULL;
    	size_t len = 0;
    	ssize_t read;
	int count = 0;
    	while ((read = getline(&line, &len, f)) != -1) {
		if (count % 3 == 0){
			query *new = malloc(sizeof(bbox));
			q[count/3] = new;
			new->x = atof(line);
		} else if (count % 3 == 1){
			q[count/3]->y = atof(line);
		} else if (count % 3 == 2){
			q[count/3]->rad = atof(line);
		} 
		count++;
	}
	fclose(f);
	//printf("count is %d\n", count);
	if (count % 3 != 0){
		printf("this file has an invalid number of lines!\n");
		return NULL;
	}
	query **temp = realloc(q,(count/3)*sizeof(query *));
	q = temp;
	(*qnum) = count/3;
	return q;
}

sqlite3 *setup_db(char *filename){
	sqlite3 *db;
	sqlite3_open(filename, &db);
	void *cache = spatialite_alloc_connection ();
	spatialite_init_ex (db, cache, 0);
	spatialite_init_geos();
	printf("initialised database\n");
	return db;
}



