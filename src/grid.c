#include "db.h"
#include "grid.h"
//METHOD 1: index based, where each partition is a series of index ranges
//METHOD 2: grid based, where we essentially make a quadtree of sorts
//METHOD 3: k means clustering, normalise clusters into grids and split cities when required


int *pop_by_index_range(int *indices, int pcount, int max_index, int sample_freq){
	//eg for 0-15 and 4 samples: 0-3, 4-7, 8-11, 12-15
	int k = (int)(max_index / sample_freq)+1;
	int *population = malloc(k*sizeof(int));
	for (int a = 0; a < k; a++){
		population[a] = 0;
	}

	for (int j = 0; j < pcount; j++){
		population[indices[j]/sample_freq]++;
	}
	for (int a = 0; a < k; a++){
		printf("sample %d-%d has %d points\n", a*sample_freq, (a+1)*sample_freq-1, population[a]);
	}
	return population;

}

bbox *create_bbox(double min_x, double min_y, double max_x, double max_y){
	bbox *b = malloc(sizeof(bbox));
	b->min_x = min_x;
	b->min_y = min_y;
	b->max_x = max_x;
	b->max_y = max_y;
	return b;
}

void print_bbox(bbox *b){
	printf("bbox has min (%f %f), max (%f %f)\n", b->min_x, b->min_y, b->max_x, b->max_y);
}

rangelist *make_partitions_by_population(unsigned int *indices, int maxparts, int pcount, int depth, int grouping) {
    int max_idx = (int)pow(4, depth);
    int max_group = (int)pow(4,depth)/grouping;
    int max_per_part = (pcount / maxparts)*1.1;
    
    rangelist *r = malloc(sizeof(rangelist));
    r->ranges = malloc(maxparts * sizeof(range*));

    int pnum = 0;
    int pstart = 0;
    int current_total = 0;

    for (int i = 0; i < max_group; i++) {
        int val = indices[i];
        if (current_total + val > max_per_part && pnum < maxparts - 1) {
            r->ranges[pnum] = malloc(sizeof(range));
            r->ranges[pnum]->start = pstart * grouping;
            r->ranges[pnum]->end = (i*grouping) - 1; 
            printf("Partition %d: indices %d-%d, points: %d\n", pnum, pstart*grouping, (i*grouping) - 1, current_total);

            pnum++;
            pstart = i;
            current_total = val;
	} else {
            current_total += val;
        }
    }
    r->ranges[pnum] = malloc(sizeof(range));
    r->ranges[pnum]->start = pstart*grouping;
    r->ranges[pnum]->end = max_idx - 1; 
    
    printf("Partition %d: indices %d-%d, points: %d\n", pnum, pstart, max_idx - 1, current_total);
    
    pnum++;
    r->len = pnum;

    return r;
}


rangelist *make_good_partitioning(int *indices, int maxparts, int pcount){

	//find "clusters" of ranges, and then group them together when possible, at the expense of other clusters when possible
	//idea: make too many clusters, and then merge as appropriate
	//but obviously that depends on the dataset
	//and i don't think that's too general
	//or we just take index ranges and merge as appropriate
	//we also need some criterion for splitting or keeping a city cluster	

	//TODO: add later!


}


///METHOD 2: divide by x and y and keep dividing until we hit some criterion or something like that
int sort_points_x(const void *p, const void *q) {
	point *p1 = *(point **)p;  
	point *p2 = *(point **)q;
	if (p1->x < p2->x) return -1;
	else if (p1->x > p2->x) return 1;
	return 0;  // Return 0 if x values are equal
}

int sort_points_y(const void *p, const void *q) {
	point *p1 = *(point **)p;  
	point *p2 = *(point **)q;  
	if (p1->y < p2->y) return -1;
	else if (p1->y > p2->y) return 1;
	return 0;  // Return 0 if y values are equal
}

Node *make_node(bbox *bounds){
	Node *n = malloc(sizeof(Node));
	n->children = calloc(4,sizeof(Node *));
	n->boundaries = bounds;
	n->pnum = 0;
	n->ind = -1;
	n->leaf = 0;
	return n;
}

//i need to make some sort of edit to the split condition
//we want to only split horizontally if the data's distributed horizontally
//and vertically if it's only vertical
//divide into grids and then 
Node *grid_division(point **p, int pnum, bbox *bounds, int minp, int maxp, int depth, int *count) {
	bbox *b = bounds;
	Node *n = make_node(b);
	n->depth = depth;
	printf("pnum is %d\n", pnum);
	if (pnum / 4 < minp || p == NULL) {
		printf("leaf node! points 0 is %f %f\n", p[0]->x, p[0]->y);
		n->pnum = pnum;
		n->ind = *count;
		printf("boundary minx %f\n", b->min_x);
		(*count)++;
	return n;
	}

	qsort(p, pnum, sizeof(point *), sort_points_x);
	
	int lr = pnum / 2;
	double xbound = p[lr]->x;
	point **left = &p[0];
	point **right = &p[lr];
	int lc = lr;
	int rc = pnum - lr;

	qsort(left, lc, sizeof(point *), sort_points_y);
	qsort(right, rc, sizeof(point *), sort_points_y);

	int tlc = lc/ 2;
	int trc = rc/ 2;
	int blc = lc-tlc;
	int brc = rc-trc;
	double ly = left[tlc]->y;
	double ry = right[trc]->y;
	n->x_med = xbound;
	n->ly_med = ly;
	n->ry_med = ry;

	bbox *tlb = create_bbox(bounds->min_x, ly, xbound, bounds->max_y);
	bbox *blb = create_bbox(bounds->min_x, bounds->min_y, xbound, ly);
	bbox *trb = create_bbox(xbound, ry, bounds->max_x, bounds->max_y);
	bbox *brb = create_bbox(xbound, bounds->min_y, bounds->max_x, ry);

	n->depth = depth;
	n->children[0] = grid_division(left, tlc, tlb, minp, maxp,depth+1,count);
	n->children[1] = grid_division(&left[tlc], blc, blb, minp, maxp,depth+1,count);
 
	n->children[2] = grid_division(right, trc, trb, minp, maxp,depth+1,count);
	n->children[3] = grid_division(&right[trc], brc, brb, minp, maxp,depth+1,count);


	//TODO:
	//FIND BETTER WAY OF CONSTRUCTING INDEX RANGES
	//FINDING SMARTER WAY OF DIVIDING GRID (IE. MAKING 2 DIVISION WHEN NECESSARY)


//if (n->children[0]->points != NULL) free(tlb);
	//if (n->children[1]->points != NULL) free(blb);
	//if (n->children[2]->points !=NULL) free(trb);
	//if (n->children[3]->points !=NULL) free(brb);
	return n;
}
Node *grid_division_x(point **p, int pnum, bbox *bounds, int min_count, int depth, int *count){
		
		Node *n = malloc(sizeof(Node));
		n->depth = depth;

		if (pnum < min_count){
				n->pnum = pnum;
				n->leaf = 1;
				n->ind = *count;
				(*count)++;
				printf("leaf node!\n");
				return n;
		}else{
				//we have some points that we can divide, presumably
				//
	n->leaf = 0;
	n->ind = -1;
	n->pnum = -1;
    qsort(p, pnum, sizeof(point *), sort_points_x);

    int mid = pnum / 2;
    point **left = malloc(mid * sizeof(point *));
    point **right = malloc((pnum - mid) * sizeof(point *));
    memcpy(left, &p[0], mid * sizeof(point *));
    memcpy(right, &p[mid], (pnum - mid) * sizeof(point *));

    int lc = mid;
    int rc = pnum - mid;

    double x_med = left[lc - 1]->x; 
    n->x_med = x_med;

    qsort(left, lc, sizeof(point *), sort_points_y);
    qsort(right, rc, sizeof(point *), sort_points_y);

    int bli = lc / 2;      
    int tli = lc - bli;    
    int bri = rc / 2;      
    int tri = rc - bri;    
    double ly_med = left[bli - 1]->y;
    double ry_med = right[bri - 1]->y;

    n->ly_med = ly_med;
    n->ry_med = ry_med;

    bbox *tlb = create_bbox(bounds->min_x, ly_med, x_med, bounds->max_y);  
    bbox *blb = create_bbox(bounds->min_x, bounds->min_y, x_med, ly_med);  
    bbox *trb = create_bbox(x_med, ry_med, bounds->max_x, bounds->max_y);  
    bbox *brb = create_bbox(x_med, bounds->min_y, bounds->max_x, ry_med);
  	n->children = malloc(4*sizeof(Node *));	
    n->children[0] = grid_division_x(&left[bli], tli, tlb, min_count, depth + 1, count);   // TL
    n->children[1] = grid_division_x(left, bli, blb, min_count, depth + 1, count);        // BL
    n->children[2] = grid_division_x(&right[bri], tri, trb, min_count, depth + 1, count); // TR
    n->children[3] = grid_division_x(right, bri, brb, min_count, depth + 1, count);       // BR

    free(left);
    free(right);

    return n;
}}

void destroy_tree(Node *n){
	if (n->pnum > 0){
	} else{for (int i=0;i<n->pnum;i++){free(n->children[i]);}
	free(n->children);
	free(n);}
}

void test_node(Node *n){
	//traverses quadtree
	printf("bbox: (%f %f),(%f %f)\n", n->boundaries->min_x, n->boundaries->min_y, n->boundaries->max_x, n->boundaries->max_y);
	if (n == NULL){printf("null node\n");return;}
	print_bbox(n->boundaries);

		if (n->pnum > 0){
			if (n->leaf != 1){printf("bad!\n");}
			printf("end of the line! pnum is %d, depth is %d, leaf is %d, ind is %d\n", n->pnum,n->depth,n->leaf,n->ind);
		}
		else if (n->children != NULL){
			//printf("xmed lymed rymed %f %f %f", n->x_med, n->ly_med, n->ry_med);
			for (int i=0;i<4;i++){
				test_node(n->children[i]);
			}
		}
}
//make it return a quadtree somehow...go by quadtree order for partitioning


//METHOD 3: k means clustering
//we also need to find approximate bboxes for large clusters
//
//
double dist(point *p, point *q){
	return sqrt((p->x-q->x)*(p->x-q->x)+(p->y-q->y)*(p->y-q->y));
}


void dbscan(point **p, double epsilon, int min_points);
void dbscan(point **p, double epsilon, int min_point){
	int c = 0;




}




unsigned short int *k_means(point **p, int k, int pnum) {
	unsigned short int *clusters = malloc(pnum * sizeof(unsigned short int));
	point **centroids = malloc(k * sizeof(point*));
	point **cent2 = malloc(k * sizeof(point*));
	point **sums = malloc(k * sizeof(point*));

	for (int i = 0; i < k; i++) {
	cent2[i] = malloc(sizeof(point));
	centroids[i] = malloc(sizeof(point));
	sums[i] = malloc(sizeof(point));
	}
	int *counts = malloc(k * sizeof(int));

	gsl_rng *g;
	gsl_rng_env_setup();
	g = gsl_rng_alloc(gsl_rng_default);
	int index;
	int converged = 0;
	double min_dist;
	int closest = 0;
	double distance = 0;
	double sum_dist;

	for (int i = 0; i < k; i++) {
	index = gsl_rng_uniform_int(g, pnum-1);
	centroids[i]->x = p[index]->x;
	centroids[i]->y = p[index]->y;
	}

	while (!converged) {
	sum_dist = 0;

	for (int i = 0; i < pnum; i++) {
		closest = 0;
		min_dist = dist(p[i], centroids[0]);
		for (int j = 1; j < k; j++) {
		distance = dist(p[i], centroids[j]);
		if (distance < min_dist) {
			closest = j;
			min_dist = distance;
		}
		}
		clusters[i] = closest;
	}

	for (int i = 0; i < k; i++) {
		sums[i]->x = 0;
		sums[i]->y = 0;
		counts[i] = 0;
	}

	for (int i = 0; i < pnum; i++) {
	if (clusters[i] < 0 || clusters[i] >= k){printf("wow! %d", clusters[i]);}

	//printf("%d: %f %f\n",i, sums[clusters[i]]->x, sums[clusters[i]]->y);
	//printf("p[i] is %f %f\n", p[i]->x, p[i]->y);
		sums[clusters[i]]->x += p[i]->x;
		sums[clusters[i]]->y += p[i]->y;
		counts[clusters[i]]++;
	}

	sum_dist = 0;
	for (int l = 0; l < k; l++) {
		if (counts[l] > 0) {
		cent2[l]->x = sums[l]->x / counts[l];
		cent2[l]->y = sums[l]->y / counts[l];
		}
		sum_dist += dist(cent2[l], centroids[l]);
	}
	if (sum_dist < 1) {
		converged = 1;
	}

	for (int i = 0; i < k; i++) {
		centroids[i]->x = cent2[i]->x;
		centroids[i]->y = cent2[i]->y;
	}
	}

	gsl_rng_free(g);
	for (int i = 0; i < k; i++) {
	free(centroids[i]);
	free(cent2[i]);
	free(sums[i]);
	}
	free(sums);
	free(centroids);
	free(cent2);
 
	free(counts);

	//for (int i=0;i<pnum;i++){printf("%d\n", clusters[i]);}
	//this seems to work...
	return clusters;
}

int *get_cluster_sizes(unsigned short int *cluster, int k, int pnum){
	int *c_sizes = malloc(k*sizeof(int));
	for (int i=0;i<k;i++){c_sizes[i]=0;}
	for (int i = 0;i<pnum;i++){
		//printf("%f\n", cluster[i]);
		c_sizes[cluster[i]]++;
	}
	//for (int i=0;i<k;i++){printf("cluster %d has %d\n", i, c_sizes[i]);}
	return c_sizes;
}

double silhouette_score(point **p, unsigned short int *cluster, int pnum, int k){
	double scores[pnum];
	double sums[k];
	double distan[k];
	for (int i=0;i<k;i++){sums[i]=0;distan[i]=0;}
	int *c_counts = get_cluster_sizes(cluster, k, pnum);
	int min_index;
	double min_dist;
	double a_i;
	double b_i;
	printf("test point: %f %f\n", p[21]->x, p[21]->y);
	for (int i=0;i<pnum;i++){
		scores[i] = 0;
		//computes silhouette score!\n
		//calculates summed distances between p[i] and the points in each cluster
		for (int j=0;j<pnum;j++){
			if (i != j){sums[cluster[j]] += dist(p[i], p[j]);}
		}

		for (int j = 0; j < k; j++){
			//printf("sum for cluster %d is %f\n", j, sums[j]);
			//printf("clustnum for cluster %d is %d\n", j, c_counts[j]);
		}

		distan[cluster[i]] = (1/((double)c_counts[cluster[i]]-1))*sums[cluster[i]];
		//calculates mean distances between p[i] and the points in each cluster
		for (int j = 0;j<k;j++){
			//printf("sum for cluster %d is %f\n", j, sums[j]);
			if (cluster[i] != j){distan[j] = (1/(double)c_counts[j])*sums[j];}
		}
		//calculates minimum
		min_index = 0;
		min_dist = distan[(cluster[i]+1)%k];
		for (int j=0;j<k;j++){
			//printf("meandist for cluster %d is %f\n", j, distan[j]);

			if (cluster[i]!=j && distan[j] <= min_dist){
				min_dist = distan[j];
				min_index = j;
			}
		}

		a_i = distan[cluster[i]];
		b_i = distan[min_index];
		//printf("a_i is %f, b_i is %f\n", a_i, b_i);

		if (a_i == b_i){scores[i]=0;}
		else if (a_i > b_i){scores[i]=((b_i/a_i)-1);}
		else if (a_i < b_i){scores[i] = 1-(a_i/b_i);}
	}
	//calculate mean silhouette score!
	double m_sil;
	for (int i=0;i<pnum;i++){m_sil+=scores[i];}
	m_sil = m_sil/pnum;
	printf("mean silhouette score is %f\n", m_sil);
	return m_sil;	
}

double davies_bouldin(point **p, unsigned short int *cluster, int pnum, int k){

	//calculates the centroid of each cluster
	point centroids[k];
	for (int i=0;i<k;i++){
		centroids[i].x = 0;
		centroids[i].y = 0;
	}
	int *c_counts = get_cluster_sizes(cluster, k, pnum);
	for (int i=0;i<pnum;i++){
		centroids[cluster[i]].x += p[i]->x;
		centroids[cluster[i]].y += p[i]->y;
	}
	for (int i=0;i<k;i++){
		centroids[i].x = centroids[i].x/(double)c_counts[i];
		centroids[i].y = centroids[i].y/(double)c_counts[i];
	}

	//now we have the centroids of each cluster!
	
	double s[k];
	for (int i=0;i<pnum;i++){s[cluster[i]] += dist(p[i],&centroids[cluster[i]])*dist(p[i],&centroids[cluster[i]]);}
	for (int i=0;i<k;i++){s[i] = sqrt(s[i]/(double)c_counts[i]);}

	double metric[k][k];
	double rij[k][k];
	for (int i=0;i<k;i++){
		for (int j=0;j<k;j++){
			if (i==j){metric[i][j] = 0;rij[i][j]=0;}
			else{
				metric[i][j] = dist(&centroids[i],&centroids[j]);
				rij[i][j] = (s[i]+s[j])/metric[i][j];
			}
		}
	}

	double ri[k];
	double max;
	int maxindex;
	for (int i = 0; i<k;i++){
		max = rij[i][(i+1)%k];
		for (int j=0;j<k;j++){
			if (rij[i][j] > max){
				max = rij[i][j];
				maxindex = j;
			}
		}
		ri[i] = max;
	}
	double sum = 0;
	double db;
	for (int i=0;i<k;i++){sum+=ri[i];}
	db = sum/(double)k;
	printf("db is %f\n", db);
	return db;



}

bbox **partitions_to_bboxes(point **p, unsigned short int *cluster, int k, int pnum){
	//returns the bbox for each partition as determined by "cluster"
	bbox **out = malloc(k*sizeof(bbox *));
	for (int i=0;i<k;i++){
		out[i] = malloc(sizeof(bbox));
		out[i]->min_x = 10000000;
		out[i]->min_y = 10000000;
		out[i]->max_x = -10000000;
		out[i]->max_y = -10000000;
	}

	for (int i = 0; i < pnum; i++){
		if (p[i]->x < out[cluster[i]]->min_x){out[cluster[i]]->min_x = p[i]->x;}
		if (p[i]->x > out[cluster[i]]->max_x){out[cluster[i]]->max_x = p[i]->x;}
		if (p[i]->y < out[cluster[i]]->min_y){out[cluster[i]]->min_y = p[i]->y;}
		if (p[i]->y > out[cluster[i]]->max_y){out[cluster[i]]->max_y = p[i]->y;}
	}
}

/*
//TODO: write query code for determing needed partitions for a query
int main(){
	bbox *b = malloc(sizeof(bbox));
	b->min_x = 0;
	b->max_x = 1;
	b->min_y = 0;
	b->max_y = 1;
	point **p = create_gaussian(10000,b);
	rule *r = get_zorder_curve();
	int *q = malloc(10000*sizeof(int));
	for (int i = 0;i<10000;i++){
		q[i] = get_index(p[i], r, 6);
	}
	pintfa("%d %d %d\n", q[100],q[200],q[9999]);
	printf("ready to go!\n");
	rangelist *rl = make_partitions_by_population(q,6,10000);
	sqlite3 *db;
	sqlite3_open("large.sqlite", &db);
	void *cache = spatialite_alloc_connection ();
	spatialite_init_ex (db, cache, 0);
	spatialite_init_geos();
	point **p1 = sample_random_points_from_table(db,"large","cent",2048);
	//int *q1 = malloc(2048*sizeof(int));
	bbox *c = get_db_boundaries(db,"large","cent");
	sqlite3_close(db);
	printf("table boundary %f %f %f %f \n", c->min_x, c->max_x, c->min_y, c->max_y);
	printf("ready to go again!\n");
	for (int j = 0; j < 2048; j++){
		if (p1[j] == NULL){
			printf("wow! a null point!\n");
		}
	}
		//printf("point %f %f\n", p1[j]->x, p1[j]->y);
		//normalise(p1[j],c);
		//q1[j] = get_index(p1[j],r,6);
	printf("readier\n");
	printf("%d %d %d\n", q1[100],q1[200],q1[999]);
	rangelist *rl2 = make_partitions_by_population(q1,6,2048);

	pop_by_index_range(q1,2048, 4096, 128);*/
	//print_bbox(c);
	//Node *n = grid_division(p1, 2048, c, 40, 512,0);
	//test_node(n);
	//unsigned short int *k = k_means(p1, 6, 2048);
	//double s = silhouette_score(p1, k, 2048, 6);
	//double d = davies_bouldin(p1, k, 2048, 6);
	//well, it doesn't seem to segfault somehow
	//destroy_tree(n);
	//printf("COMPLETED!\n");	

	//test kmeans
	//printf("%f %f %d\n", p1[100]->x, p1[100]->y, k[100]);

	//tries again with real data
	//
	//idea for improvement: sample points per n indices and use that to make partitions somehow


