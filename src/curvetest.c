#include "curve.h"
#include "db.h"

void print(bbox *b){
	printf("bbox (%f %f) (%f %f)\n", b->min_x*4, b->max_x*4, b->min_y*4, b->max_y*4);
}
int main(){
	rule *r = get_hilbert_curve();
	for (int i=0;i<16;i++){
		bbox *q = unindex(i,r,2);
		print(q);
		point *p = malloc(sizeof(point));
		p->x = (q->min_x + q->max_x)/2;
		p->y = (q->min_y + q->max_y)/2;
		int j = get_index(p,r,2);
		printf("index is %d\n", j);
		free(p);
		free(q);
	}
	
}

