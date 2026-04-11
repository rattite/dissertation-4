#include "db.h"
#include "test_helper.h"

//from a database, makes copies with a sample of points
//
int main(int argc, char *argv[]){
	//arguments:
	//1: name of input file
	//2: name of tab in input
	//3: name of col in input
	//4: no of samples to create
	if (argc < 5){
		printf("usage: <input file> <output file> <samples>\n");
		return 1;
	}
	int samples = atoi(argv[4]);
	sqlite3 *db = setup_db(argv[1]); 
	unsigned int size = get_tab_size(db,argv[2]);
	size = (size / 5000) * 5000;
	char name[256];
	point **p = sample_random_points_from_table(db,argv[2],argv[3],size);
	sqlite3_close(db);
	for (int i=0;i<samples;i++){
		int s2 = (int)((size/(samples+1))*(i+1));
		snprintf(name,sizeof(name),"%s_%d",argv[2],i);
		create_db(name,name,p,s2);
	}
	for (int k=0;k<size;k++){free(p[k]);}
	free(p);
	printf("COMPLETED!\n");
	return 0;
}
