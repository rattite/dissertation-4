#include "db.h"
#include "test_helper.h"

int main(int argc, char *argv[]){
	//arguments:
	//1: name of input file
	//2: name of tab in input
	//3: name of col in input
	//4: no of samples to create
	if (argc < 5){
		printf("usage: <input file> <output file> <sample size>\n");
		return 1;
	}
	int sample_size = atoi(argv[4]);
	sqlite3 *db = setup_db(argv[1]); 
	unsigned int size = get_tab_size(db,argv[2]);
	size = (size / 5000) * 5000;
	char name[256];
	point **p = sample_random_points_from_table(db,argv[2],argv[3],size);
	sqlite3_close(db);
	snprintf(name,sizeof(name),"%s_%d",argv[2],sample_size);
	create_db(name,name,p,sample_size);
	for (int k=0;k<size;k++){free(p[k]);}
	free(p);
	printf("COMPLETED!\n");
	return 0;
}
