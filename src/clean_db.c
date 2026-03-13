#include "test_helper.h"
int main(int argc, char *argv[]){
	//argv[1]: filename
	//argv[2]: table
	//argv[3]: column to save
	sqlite3 *db = setup_db(argv[1]);
	char sql[256];
	snprintf(sql,sizeof(sql), "SELECT name FROM pragma_table_info('%s') WHERE name != 'ogc_fid' AND name != '%s';",argv[2],argv[3]);
	const char **c = malloc(1000*sizeof(char *));
	sqlite3_stmt *stmt;
	if(sqlite3_prepare_v2(db,sql,-1,&stmt,NULL)!=SQLITE_OK){printf("err: %s\n", sqlite3_errmsg(db));}
	int count = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW){
		c[count] = strdup(sqlite3_column_text(stmt,0));
		printf("%s\n", c[count]);
		count++;
	}

	//reallocs c
	const char **v = realloc(c,count*sizeof(char *));
	c = v;

	sqlite3_finalize(stmt);
	
	sqlite3_exec(db,"BEGIN",NULL,NULL,NULL);
	for (int i=0;i<count;i++){
		printf("removing %s\n", c[i]);
		snprintf(sql,sizeof(sql),"ALTER TABLE %s DROP COLUMN '%s'", argv[2], c[i]);
		if(sqlite3_exec(db,sql,NULL,NULL,NULL)!=SQLITE_OK){printf("err: %s\n", sqlite3_errmsg(db));}
		}
	sqlite3_exec(db,"COMMIT",NULL,NULL,NULL);
		




	sqlite3_close(db);
	printf("COMPLETED!\n");
	return 0;
}
	

