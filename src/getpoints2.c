//gets points 2
#include "db.h"
#include "test_helper.h"

unsigned int poisson(gsl_rng *g, double lambda){
	double L = exp(-1*lambda);
	double k = 0;
	double p = 1;
	double f;
	while (p > L){
		k++;
		f = gsl_rng_uniform(g);
		p = p * f;
	}
	return k-1;
}

point *uniform_point(gsl_rng *g, bbox *b){
	double x =  gsl_rng_uniform(g);
	double y = gsl_rng_uniform(g);
	point *p = malloc(sizeof(point));
	p->x = b->min_x + (b->max_x-b->min_x)*x;
	p->y = b->min_y + (b->max_y-b->min_y)*y;
	return p;
}

point *new(gsl_rng *g, point *base, double sigma){
	double x = gsl_ran_gaussian(g, sigma);
	double y = gsl_ran_gaussian(g, sigma);
	point *p = malloc(sizeof(point));
	p->x = x+base->x;
	p->y = y+base->y;
	return p;
}
unsigned long scramble(unsigned long x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdUL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53UL;
    x ^= x >> 33;
    return x;
}

point **create_random_2(gsl_rng *g, int n, bbox *b){

	point **p = (point **)malloc(n*sizeof(point *));
	for (int i=0;i<n;i++){
		p[i] = (point *)malloc(sizeof(point));
		p[i]->x = gsl_rng_uniform(g);
		p[i]->y = gsl_rng_uniform(g);
		p[i] = unit_to_bbox(p[i],b);
	}
	return p;
}

int main(int argc, char *argv[]){

	//islands join hands 'neath heaven's sea

	gsl_rng *g;
	gsl_rng_env_setup();
       	g = gsl_rng_alloc(gsl_rng_mt19937);
	unsigned long seed = scramble(time(NULL));
	printf("seed is %d\n", seed);
	gsl_rng_set(g,seed);

	bbox *d = malloc(sizeof(bbox));
	d->min_x = -200000;
	d->min_y = -200000;
	d->max_x = 200000;
	d->max_y = 200000;
	double diag_dist = sqrt((d->max_y-d->min_y)*(d->max_x-d->min_x));
	double lp = 5;
	double mu = 20480;
	double sigma = 67;
	int samples = atoi(argv[1]);
	unsigned int cc = poisson(g,lp);
	if (cc < 6){cc=6;}
	int ran_count = mu*cc*2;

	printf("%d\n", cc);
	unsigned int *sizes = malloc(cc*sizeof(unsigned int));
	point ***c = malloc(cc*sizeof(point **));
	point **c_centres = create_random_2(g,cc,d);

	for (int i=0;i<cc;i++){
		printf("cent i is %f %f\n", c_centres[i]->x, c_centres[i]->y);
		unsigned int mu_i = mu + (unsigned int)(gsl_ran_gaussian(g,mu/4));
		printf("%d\n", mu_i);
		c[i] = malloc(mu_i * sizeof(point *));
		sizes[i] = mu_i;
		for (int j=0;j<mu_i;j++){
			c[i][j] = new(g,c_centres[i],0.0314 * diag_dist);
			//printf("%f %f\n", c[i][j]->x, c[i][j]->y);
			
		}
	}

	//generates random points
	bbox *b = malloc(sizeof(bbox));
	d->min_x = -240000;
	d->min_y = -240000;
	d->max_x = 240000;
	d->max_y = 240000;
	point **ran = create_random_2(g,ran_count,d);


	//now we reduce into one large array
	int stot = 0;
	for (int i=0;i<cc;i++){stot = stot + sizes[i];}
	point **large = malloc((stot)*sizeof(point *));
	int curr = 0;
	for (int i=0;i<cc;i++){
		for (int j=0;j<sizes[i];j++){
			large[curr+j]=c[i][j];
		}
		curr = curr + sizes[i];
	}
	create_db("synth0","synth0",large,stot);
	point **temp = realloc(large,(stot+ran_count/2)*sizeof(point *));
	large = temp;
	for (int i=stot;i<stot+ran_count/2;i++){large[i]=ran[i-stot];}
	create_db("synth1","synth1",large,stot+ran_count/2);
	point **temp2 = realloc(large,(stot+ran_count)*sizeof(point *));
	large = temp2;
	for (int i=stot+ran_count/2;i<stot+ran_count;i++){large[i]=ran[i-stot];}
	create_db("synth2","synth2",large,stot+ran_count);

	printf("completed!\n");
}
		

