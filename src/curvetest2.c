#include "curve.h"
#include "db.h"
#include <time.h>


int main(int argc, char *argv[]){
	struct timespec start, end;
	rule *r = get_hilbert_curve();
	bbox *b = gen_bbox(0,0.1,0.3,0.7);
	for (int i=0;i<6;i++){
		clock_gettime(CLOCK_MONOTONIC, &start);
		rangelist *rl = get_ranges(b,r,4+i*2);
		clock_gettime(CLOCK_MONOTONIC, &end);
		double elapsed = (end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec) / 1e9;
		printf("time taken for bad %d : %.9f seconds\n", 4+i*2,elapsed);
		clock_gettime(CLOCK_MONOTONIC, &start);
		rangelist *rl2 = get_ranges_2(b,r,4+i*2);
		clock_gettime(CLOCK_MONOTONIC, &end);
		elapsed = (end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec) / 1e9;
		printf("time taken for good %d: %.9f seconds\n", 4+i*2,elapsed);
		if (rl->len!=rl2->len){
			printf("size discrepancy!\n");
		}
		printf("ranges no is %d\n", rl->len);
		for (int j=0;j<rl->len;j++){
			if (rl->ranges[j]->start != rl2->ranges[j]->start || rl->ranges[j]->end != rl2->ranges[j]->end){
				printf("one or more ranges is bad!\n");
				break;
			}
		}


	}
	printf("done!\n");
}
