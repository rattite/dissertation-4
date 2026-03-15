#include "test_helper.h"
#include "m1.h"
int main(int argc, char *argv[]){
	if (argc != 3){
		printf("usage: ./reset <filename> <table>\n");
		return 1;
	}
	//resets the table to its default
	//argv[1] is filename, argv[2] is table name
	//failure to provide the correct value to argv[2] will result in the deletion of your database. so be careful!
	sqlite3 *db = setup_db(argv[1]);
	reset_db(db,argv[2]);
	destroy_part_structure(db,argv[2],256);
	sqlite3_close(db);
	printf("reset!\n");
	return 0;
}

