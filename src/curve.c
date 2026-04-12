#include "curve.h"

//Hello! The way I see things at the moment, I have 3 main tasks
//1. Curve optimisation, to better preserve spatial locality (somehow!)
//2. Horizontal partitioning (I predict this will be very important for reducing query times)
//3. Better range query implementation (right now it's implemented naively, which is quite bad)
//
//Secondary goals:
//1. Dynamically calculate required index length for a certain precision

bbox *gen_bbox(double min_x, double min_y, double max_x, double max_y){
	bbox *b = malloc(sizeof(bbox));
	b->min_x = min_x;
	b->min_y = min_y;
	b->max_x = max_x;
	b->max_y = max_y;
	return b;
}
void bbox_get_lengths(bbox *b, double *x, double *y){
	*x = b->max_x-b->min_x;
	*y = b->max_y-b->min_y;
}

void free_rule(rule *r){
	/*we run into the issue here that a rule in next might already be freed*/
	//TODO: add conditionals and stuff for memory safety!!!!
	//
	if (r == NULL){return;}
	else{
		for (int j=0;j<4;j++){
			free_rule(r->next[j]);
		}
		free(r->trav);
		free(r->pos);
		for (int i=0;i<r->dim;i++){
			free(r->next[i]);
			r->next[i] = NULL;
	}
	free(r->next);
	}
}

void print_ibb(intbbox *a){
	printf("ib is x: %d %d y: %d %d\n", a->min_x, a->max_x, a->min_y, a->max_y);
}
int ibis(intbbox *a, intbbox *b){
	/* i like ibis
                                 .....
                            .e$$$$$$$$$$$$$$e.
                          z$$ ^$$$$$$$$$$$$$$$$$.
                        .$$$* J$$$$$$$$$$$$$$$$$$$e
                       .$"  .$$$$$$$$$$$$$$$$$$$$$$-
                      .$  $$$$$$$$$$$$$$$$***$$  .ee"
         z**$$        $$r ^**$$$$$$$$$*" .e$$$$$$*"
        " -\e$$      4$$$$.         .ze$$$""""
       4 z$$$$$      $$$$$$$$$$$$$$$$$$$$"
       $$$$$$$$     .$$$$$$$$$$$**$$$$*"
     z$$"    $$     $$$$P*""     J$*$$c
    $$"      $$F   .$$$          $$ ^$$
   $$        *$$c.z$$$          $$   $$
  $P          $$$$$$$          4$F   4$
 dP            *$$$"           $$    '$r
.$                            J$"     $"
$                             $P     4$
F                            $$      4$
                            4$%      4$
                            $$       4$
                           d$"       $$
                           $P        $$
                          $$         $$
                         4$%         $$
                         $$          $$
                        d$           $$
                        $F           "3
                 r=4e="  ...  ..rf   .  ""% Gilo94'
                $**$*"^""=..^4*=4=^""  ^"""
------------------------------------------------
*/

	//we have 3 different cases:
	//b completely within a (including touching edges) - returns 0
	//a intersects b (excluding touching edges) - returns 1
	//a and b have no relation - returns -1
	//
	if (a->min_x <= b->min_x && a->min_y <= b->min_y && a->max_x >= b->max_x && a->max_y >= b->max_y){
		return 0;
	}

	if (a->max_x <= b->min_x || a->max_y <= b->min_y || a->min_x >= b->max_x || a->min_y >= b->max_y){
		return -1;
	}
	return 1;
}
void gr(intbbox *query, intbbox *curr, rule *r, int prec, int depth, rangelist *rl, unsigned int start, int tolerance){
	int ib = ibis(query,curr);
	//print_ibb(query);
	//print_ibb(curr);
	//printf("start is %d, depth is %d, ibis case is %d\n", start, depth,ib);
	if (ib == -1){ //if there is no intersect at all!
		return;
	}

	if (ib == 0 || depth==prec){//if we're fully within the query area
		int end = start+(1<<(2*(prec-depth)))-1;
		if (rl->len > 0 && rl->ranges[rl->len-1]->end + tolerance >= start){
			rl->ranges[rl->len-1]->end = end;
		} else {
		range *r = malloc(sizeof(range));
		r->start = start;
		r->end = end;
		rl->ranges[rl->len]=r;
		rl->len++;
		}
		return;
	}

	//now we have to traverse all 4 subtrees
	//gets next bboxes, somehow
	intbbox quadrants[4];
	//eg: (1,5)(3,8) divides into bl(1525),tl(1628)
	unsigned int xmid = (curr->min_x+curr->max_x)/2;
	unsigned int ymid = (curr->min_y+curr->max_y)/2;
	quadrants[0] = (intbbox){curr->min_x, ymid+1,xmid,curr->max_y};
	quadrants[1] = (intbbox){curr->min_x,curr->min_y,xmid,ymid};
	quadrants[2] = (intbbox){xmid+1,ymid+1,curr->max_x,curr->max_y};
	quadrants[3] = (intbbox){xmid+1,curr->min_y,curr->max_x,ymid};
	for (int i=0;i<4;i++){
		unsigned int k = r->trav[i];
		//we want to check the quadrants in traversal order
		gr(query,&quadrants[k], r->next[k], prec, depth+1, rl, start+i*(1<<(2*(prec-depth)-1)-1),tolerance);
	}
	return;
}	

intbbox *unit_to_int(bbox *b, int prec){
	intbbox *i = malloc(sizeof(intbbox));
	i->min_x = (unsigned int)(b->min_x*pow(4,prec));
	i->min_y = (unsigned int)(b->min_y*pow(4,prec));
	i->max_x = (unsigned int)(b->max_x*pow(4,prec));
	i->max_y = (unsigned int)(b->max_y*pow(4,prec));
	return i;
}

intbbox *create_large(int prec){
	intbbox *i = malloc(sizeof(intbbox));
	i->min_x = 0;
	i->min_y = 0;
	i->max_x = pow(4,prec)-1;
	i->max_y = pow(4,prec)-1;
	return i;
}

rangelist *get_ranges_2(bbox *b, rule *r, int prec){
	rangelist *rl = malloc(sizeof(rangelist));
	rl->ranges = malloc(100000*sizeof(range *)); //we will realloc this later!!!!
	rl->len = 0;
	intbbox *q = unit_to_int(b,prec);
	//printf("%d %d %d %d\n", q->min_x, q->min_y, q->max_x, q->max_y);
	intbbox *world = create_large(prec);
	//printf("%d %d\n", world->min_x, world->max_x);
	//now we call the gr function
	gr(q,world,r,prec,0,rl,0,(1<<(prec-1))); //this is recursive, so we need only call it once
	//HOPEFULLY it should come out sorted
	//
	//for (int i=0;i<rl->len;i++){
	//	printf("range is %d %d\n", rl->ranges[i]->start, rl->ranges[i]->end);
	//}
	range **temp = realloc(rl->ranges, rl->len * sizeof(range *));
	rl->ranges = temp;
	free(q);
	free(world);
	return rl;

}

void free_rangelist(rangelist *r){
	for (int i = 0; i < r->len;i++){
		free(r->ranges[i]);
	}
	free(r->ranges);
	free(r);
}




int comp(const void *a, const void *b) {
	//Comparison function for qsort(int *)
    return (*(int *)a - *(int *)b);
}

rangelist *collect (int *arr, int n, int tolerance){
	qsort(arr, n, sizeof(int), comp);
	int b = arr[0];
	int count = 0;
	range **ranges = (range **)malloc(1000000*sizeof(range *));
	for (int i = 1; i<n; i++){
			if (arr[i] > arr[i-1]+tolerance+1){
				ranges[count] = (range *)malloc(sizeof(range));
				ranges[count]->start = b;
				ranges[count]->end = arr[i-1];
				b = arr[i];
				count++;
			}
		}
	ranges[count] = (range *)malloc(sizeof(range));
	ranges[count]->start = b;
	ranges[count]->end = arr[n-1];
	range **z = realloc(ranges, (count+1)*sizeof(range *));
	if (!z){printf("fail!\n");}
	ranges = z;

	rangelist *rl = (rangelist *)malloc(sizeof(rangelist));
	rl->ranges = ranges;
	rl->len = count+1;

	for (int j = 0; j<rl->len;j++){
		//printf("range %d %d\n", rl->ranges[j]->start, rl->ranges[j]->end);
	}
	return rl;
}
			//make new range, and add that into the array

int get_index(point *p, rule *base, int prec){
	//Gets the index value for a point (normalised to [0,1]^2) from a given base rule
	unsigned int x = floor(p->x * (1<<prec));
	unsigned int y = floor(p->y * (1<<prec));
	//printf("xy vals are %d, %d\n", x, y);
	unsigned int index = 0;
	rule *curr = base;
	for (int i=0;i<prec;i++){
		unsigned int mask = 1u << (prec - i - 1);
		unsigned int xbit = (x & mask) != 0;
		unsigned int ybit = (y & mask) != 0;

		unsigned int k = (xbit << 1) | (1-ybit);
		index = (index << 2) + (curr->pos)[k];
		curr = (curr->next)[k];
		//curr is type 
	}

	//printf("done! ind value is %d\n", index);
	return index;
}

void assign_trav(rule *r, short int *t, int dim){
	//Does all the malloc stuff for a traversal so I don't have to repeat it
	for (int i = 0;i<dim;i++){
		r->trav[i] = t[i];
		r->pos[t[i]] = i;
	}
}

	

rangelist *get_ranges(bbox *b, rule *base, int prec){
	//somehow this function is broken
	//somehow convert a point to index values
	//this is awful code and i am going to implement this properly this week
	int x1 = floor(b->min_x * pow(2,prec));
	int y1 = floor(b->min_y * pow(2,prec));
	int x2 = ceil(b->max_x * pow(2,prec))-1;
	int y2 = ceil(b->max_y * pow(2,prec))-1;
	//printf("min_x: %d max_x: %d min_y: %d, max_y: %d\n", x1, x2, y1, y2);
	int dim_x = (1+x2-x1);
	int dim_y = (1+y2-y1);
	int dim = dim_x * dim_y;
	int *q = (int *)malloc(dim*sizeof(int));
	point *p = (point *)malloc(sizeof(point));
	for (int i=0; i<dim_x; i++){
		for (int j=0; j<dim_y; j++){
			p->x = (2*(x1+i)+1)/pow(2,prec+1);
			p->y = (2*(y1+j)+1)/pow(2,prec+1);			       //todo: make get_index accept integers instead of points o algo	
			//printf("point coords: x %f y %f\n", p->x, p->y);
			q[i*dim_y+j] = get_index(p,base,prec);
			//printf("%d\n", q[i*dim_y+j]);

		}
	}
	free(p);
	rangelist *ranges = collect(q, dim,pow(2,prec-1));
	free(q);
	return ranges;

}
	

bbox *unindex(int ind, rule *base, int prec){
	//Gets a bounding box (normalised to [0,1]^2) from an index value
	//As with all things, assumes each rule splits into quadrants
	//TODO: potentially allow for more interesting rule types

	unsigned int x = 0;
	unsigned int y = 0;
	unsigned int temp = ind;
	unsigned int q;
	unsigned int j;
	rule *curr = base;
	for (int i=0;i<prec;i++){
		unsigned int shift = 2*(prec-i-1);
		q = (ind >> shift) & 3;
		j = curr->trav[q];
		unsigned int xbit = (j >> 1) & 1;
		unsigned int ybit = 1 - (j & 1);

		x = (x << 1) | xbit;
		y = (y << 1) | ybit;
		curr = curr->next[j];

		
	}
	bbox *b = (bbox *)malloc(sizeof(bbox));
	b->min_x = x * pow(2,-prec);
	b->min_y = y * pow(2,-prec);
	b->max_x = (x+1) * pow(2,-prec);
	b->max_y = (y+1) * pow(2,-prec);
	return b;


}

rule *rule_init(int dim){
	//Does all the malloc stuff for a rule
	rule *r = (rule *)malloc(sizeof(rule));
	r->dim = dim;
	r->next = (rule **)malloc(dim*sizeof(rule *));
	r->trav = (short int *)malloc(dim*sizeof(short int));
	r->pos = (short int *)malloc(dim*sizeof(short int));
	return r;
}





point *bbox_to_unit(point *p, bbox *b){
	/*converts a point in WGS84 to be between 0 and 1*/
	point *q = (point *)malloc(sizeof(point));
	q->x = (p->x - b->min_x) / (b->max_x-b->min_x);
	q->y = (p->y - b->min_y) / (b->max_y-b->min_y);
	return q;

/*there has to be a better way of doing this...*/
}

point *unit_to_bbox(point *p, bbox *b){
	/*converts a point in [0,1] to a WGS84 coordinate*/
	point *q = (point *)malloc(sizeof(point));
	q->x = b->min_x + p->x * (b->max_x-b->min_x);
	q->y = b->min_y + p->y * (b->max_y-b->min_y);
	return q;
}


bbox *get_bbox(point **p, int n){
	//Gets a bounding box for a list of points
	//Note: the lowest and highest values WILL be on the boundary of the box,
	//this may cause issues 
	bbox *b = (bbox *)malloc(sizeof(bbox));
	double min_x = 10000000;
	double max_x = -10000000;
	double min_y = 10000000;
	double max_y = -10000000;	
	for (int i=0;i<n;i++){
		if (p[i]->x < min_x){min_x = p[i]->x;};
		if (p[i]->x > max_x){max_x = p[i]->x;};
		if (p[i]->y < min_y){min_y = p[i]->y;};
		if (p[i]->y > max_y){max_y = p[i]->y;};

	}
	b->min_x = min_x;
	b->min_y = min_y;
	b->max_x = max_x;
	b->max_y = max_y;
	return b;
}

//well, it seems to work, and that's what matters;
int check_if_adjacent(bbox *b, bbox *c, int prec){
	//Given two bboxes, checks if they're next to each other in the grid
	//This code is not very good
	const float root_3 = 1.67;
	int flag = 1;
	int b_x = floor(b->max_x * (int)pow(2,prec));
	int b_y = floor(b->max_y * (int)pow(2,prec));
	int c_x = floor(c->max_x * (int)pow(2,prec));
	int c_y = floor(c->max_y * (int)pow(2,prec));
	if (abs(c_y - b_y) < 2 && abs(c_x-b_x)<2 && abs(c_y-b_y)|abs(c_x-b_x)){
		flag = 0;
	}
	return flag;
	

}

int measure_large_discontinuities(rule *base, int prec){
	//TODO: allow for rules of custom sizes, or maybe better grid divisions or something like that. right now we're just bitshifting, but we can do better than that
	//Given a whole curve (TODO: restrict to area), checks the number of "discontinuities",
	//which is when cells next to each other on the curve aren't adjacent in 2d space.
	//TODO: measure discontinuties by "severity"
	int bad = 0;
	for (int i = 0; i < (int)pow(4,prec)-1;i++){
		bbox *b = unindex(i,base,prec);
		bbox *c = unindex(i+1,base,prec);
		int flag = check_if_adjacent(b,c,prec);
		//we can probably get more information if we wanted to!
		if (flag == 1){
			bad++;
		}
		free(b);
		free(c);
	}
	printf("%d discontinuities\n", bad);
	return bad;
	}




