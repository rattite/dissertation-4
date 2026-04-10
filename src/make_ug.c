//makes uniformly and gaussianly distributed databases, and such like that
//
#include "db.h"
int main(int argc, char *argv[]){
	if (argc != 2){
		printf("usage: make_ug <pnum>");
		exit(1);
	}
	int count = atoi(argv[1]);
	bbox *b = gen_bbox(-100000,-100000,100000,100000);
	point **p = create_random(count,b);
	create_db("ran",p,count);
	point **q = create_gaussian(count,b,0.2);
	create_db("gau",q,count);
	printf("data is COMPLETED!\n");
	return 0;
}
