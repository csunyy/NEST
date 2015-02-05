/*
 *author: hdz
 *date: 2013.5.8
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include "lsh.h"

#define BUFSIZE 1024 
#define MAX_HASH_RND 536870912U
#define UH_PRIME_DEFAULT 4294967291U

LshParam *global_lsh_param = NULL;

LshParam *initLshParam(const char *config_file)
{
	LshConfig *lsh_conf = NULL;	
	UniformHashFunction *uhf = NULL;
	HashFamily *hash_family = NULL;
	LshParam *lsh_param;
	double *buf;
	
	
	lsh_conf = (LshConfig*)malloc(sizeof(LshConfig));
	if(NULL == lsh_conf) return NULL; 

	uhf = (UniformHashFunction*)malloc(sizeof(UniformHashFunction));
	if(NULL == uhf) {
		free(lsh_conf);
		return NULL;
	}

	if(loadConfigFile(config_file, lsh_conf) < 0)
                return NULL;

        if(initUniformHashFunction(uhf, lsh_conf) < 0)
                return NULL;
	
	hash_family = (HashFamily*)malloc(sizeof(HashFamily) * lsh_conf->l);
        if(NULL == hash_family)
        {
		free(lsh_conf);
		free(uhf);
		return NULL;
        }

        if(initHashFamily(lsh_conf, hash_family, uhf, lsh_conf->l) < 0)
                return NULL;

	buf = (double*)malloc(sizeof(double)*lsh_conf->k);
	if(buf == NULL) {
		free(lsh_conf);	
		free(uhf);
		free(hash_family);
		return NULL;
	}

	lsh_param = (LshParam*)malloc(sizeof(LshParam));
	if(NULL == lsh_param) {
		free(lsh_conf);
		free(uhf);
		free(hash_family);	
		free(buf);
		return NULL;
	}		

	lsh_param->config = lsh_conf;
	lsh_param->hash_family = hash_family;
	lsh_param->buf = buf;
	
	global_lsh_param = lsh_param;
	
	return lsh_param;
}

#define warning(param) 	do{\
	fprintf(stderr, \
	"param %s need to be setted correctly.\n", \
	param); \
	fclose(fp); \
	return -1; \
}while(0)


static inline void printConfig(LshConfig *conf)
{
	char *lsh_type = NULL;
	char *optimize_flag = NULL;
	if(conf->lsh_type == E2LSH) lsh_type = "E2LSH"; 
	else lsh_type = "Hamming LSH";
	
	if(conf->optimize_kickout == OPTIMIZE_ON) 	
		optimize_flag = "on";
	else optimize_flag = "off";

	printf("r = %.2f\n" \
		"w = %.2f\n" \
		"k = %d\n" \
		"l = %d\n" \
		"size = %d\n" \
		"offset = %d\n" \
		"dimension = %d\n" \
		"max_steps = %d\n" \
		"p = %.4f\n" \
		"lsh_type = %s\n" \
		"optimize %s\n\n",\
		conf->r, conf->w, conf->k, \
		conf->l, conf->size, conf->offset, conf->dimension, \
		conf->max_steps, conf->p, lsh_type, \
		optimize_flag);
}

int loadConfigFile(const char* filename, LshConfig *conf)
{
	assert(filename != NULL && conf != NULL);
	FILE *fp;
	char buf[BUFSIZE];
	char *p;
	int len;

	memset(conf, 0, sizeof(LshConfig));

	if(NULL == (fp = fopen(filename, "r")))
	{
		fprintf(stderr, "can't read configration file: %s.\n", filename);
		return -1;
	}

	while(NULL != fgets(buf, BUFSIZE, fp))
	{
		if(buf[0] == '#' || buf[0] == '\n')
			continue;

		len = strlen(buf);
		buf[len-1] = '\0';
		
		p = strchr(buf, '=');
		if(p == NULL)
		{
			fprintf(stderr, "read configuration file failed: %s.\n", filename);
			fclose(fp);
			return -1;
		}
		*p++ = '\0';
		if(strcmp("r", buf) == 0)
		{
			conf->r = atof(p);
			if(conf->r <= 0) warning("r");
			continue;
		}
		if(strcmp("w", buf) == 0)
		{
			conf->w = atof(p);
			if(conf->w <= 0) warning("w");
			continue;
		}
		if(strcmp("k", buf) == 0)
		{
			conf->k = atoi(p);
			if(conf->k <= 0) warning("k");
			continue;
		}
		if(strcmp("l", buf) == 0)
		{
			conf->l = atoi(p);
			if(conf->l <= 0) warning("l");
			continue;
		}
		if(strcmp("dimension", buf) == 0)
		{
			conf->dimension = atoi(p);
			if(conf->dimension <= 0) warning("dimension");
			continue;
		}
	
		if(strcmp("size", buf) == 0)
		{
			conf->size = atoi(p);
			if(conf->size <= 0) warning("size");
			continue;
		}

		if(strcmp("max_steps", buf) == 0)
		{
			conf->max_steps = atoi(p);
			if(conf->max_steps <= 0) warning("max_steps");
			continue;
		}

		if(strcmp("p", buf) == 0) 
		{
			conf->p = atof(p);
			if(conf->p <= 0.0) warning("p"); 
			continue;
		}		

		if(strcmp("lsh_type", buf) == 0)
		{
			if(strcmp("e", p) == 0)
				 conf->lsh_type = E2LSH;
			else if(strcmp("h", p) == 0)
				 conf->lsh_type = HAMMING;
			else
				warning("lsh_type");
			continue;
		}
	
		if(strcmp("optimize_kickout", buf) == 0) 
		{
			if(strcmp("on", p) == 0)
				conf->optimize_kickout = OPTIMIZE_ON;
			else if(strcmp("off", p) == 0) 
				conf->optimize_kickout = OPTIMIZE_OFF;	
			else 
				warning("optimize_kickout");
			continue;
		}
		
		if(strcmp("offset", buf) == 0)
		{
			conf->offset = atoi(p);
			if(conf->offset < 0) warning("offset");
			continue;
		}
		
	}

	fclose(fp);
	printConfig(conf);
	if(conf->lsh_type == HAMMING) {
		conf->w = 1;
	}
	return 0;
}

int initHashFamily(LshConfig *conf, HashFamily *hash_family, UniformHashFunction *uhf, int num)
{
	assert(conf != NULL && hash_family != NULL && num > 0);

	int i = 0, j = 0, k = 0;
	LSH_TYPE lsh_type = conf->lsh_type;
	double r1, r2, m, P = conf->p;
	
	memset(hash_family, 0, sizeof(HashFamily)*num);
	for(i = 0; i < num; i++)
	{
		hash_family[i].a = (float**)malloc(sizeof(float*)*conf->k);
		if(NULL == hash_family[i].a)
		{
			goto error;
		}
		hash_family[i].b = (float*)malloc(sizeof(float)*conf->k);
		if(NULL == hash_family[i].b)
		{
			goto error;
		}
		hash_family[i].c = (unsigned int*)malloc(sizeof(unsigned int)*conf->dimension);
		if(NULL == hash_family[i].c)
		{
			goto error;
		}
		memset(hash_family[i].a, 0, sizeof(float*)*conf->k);
		memset(hash_family[i].b, 0, sizeof(float)*conf->k);
		memset(hash_family[i].c, 0, sizeof(unsigned int)*conf->dimension);
	
	}

	for(i = 0; i < num; i++)
	{
		for(j = 0; j < conf->k; j++)
		{
			hash_family[i].a[j] = (float*)malloc(sizeof(float)*conf->dimension);
			if(NULL == hash_family[i].a[j])
				goto error;
			for( k = 0; k < conf->dimension; k++)
			{
				if(lsh_type == E2LSH)
					hash_family[i].a[j][k] = getGaussianRandom();
				else if(lsh_type == HAMMING) {
					r1 = getUniformRandom(0.0, 1.0);
					r2 = getUniformRandom(0.0, 1.0);
					m = M_PI * (r1 - 0.5);
					hash_family[i].a[j][k] = (sinf(P*m) / (powf(cosf(m), 1/P))) \
			       				* powf(cosf(m*(1-P))/(-1.0*log(r2)), (1-P)/P); 
								
				} else {
					fprintf(stderr, "unknown LSH type.\n");
					return -1;
				}
			}

			if(lsh_type == E2LSH) 
				hash_family[i].b[j] = getUniformRandom(0, conf->w);
			else if(lsh_type == HAMMING) 
				hash_family[i].b[j] = 0.0;
			else {
				fprintf(stderr, "unknown LSH type.\n");
				return -1;
			}

		}

		for(j = 0; j < conf->k; j++)
			hash_family[i].c[j] = uhf->u[j]; 
	}

	goto success;	

error:
	for(i = 0; i < num; i++)
	{
		if(hash_family[i].a != NULL)
		{
			for(j = 0; j < conf->k; j++)
			{
				if(hash_family[i].a[j] != NULL)
					free(hash_family[i].a[j]);
			}
			free(hash_family[i].a);
		}

		if(hash_family[i].b != NULL)
			free(hash_family[i].b);

		if(hash_family[i].c != NULL)
			free(hash_family[i].c);		
	}

	fprintf(stderr, "can't allocate memory for lsh param.\n");
	return -1;

success:
	return 0;

}

float getUniformRandom(float range_start, float range_end)
{
	float tmp;
	float ret;

	if(range_start > range_end)
	{
		tmp = range_start;
		range_start = range_end;
		range_end = tmp;
	}
	
	ret = range_start + ((range_end - range_start)*(float)random()/(float)RAND_MAX);
	assert(ret >= range_start && ret <= range_end);
	return ret;
}

float getGaussianRandom()
{
	float x1, x2;
	float z;

	do {
		x1 = getUniformRandom(0.0,1.0);
	} while(x1 == 0);
	x2 = getUniformRandom(0.0, 1.0);
	z = sqrt(-2.0*logf(x1))*cosf(2.0*M_PI*x2);
	return z;
}


unsigned int getRandomUns32(unsigned int range_start, unsigned int range_end)
{
	unsigned int ret;
	unsigned int tmp;
	if(range_start > range_end)
	{
		tmp = range_end;
		range_start = range_end;
		range_end = tmp;
	}
	if(RAND_MAX >= range_end - range_start)
	{
		ret = range_start + (unsigned int)((range_end - range_start + 1.0)*random() / (RAND_MAX + 1.0));
	}
	else
	{
		ret = range_start + (unsigned int)((range_end - range_start + 1.0)
			*((long long unsigned)random()*((long long unsigned)RAND_MAX + 1)
			+(long long unsigned)random())/((long long unsigned)RAND_MAX*((long long unsigned)RAND_MAX + 1)
			+(long long unsigned)RAND_MAX + 1.0));
	}
	return ret;	
}

int initUniformHashFunction(UniformHashFunction *uhf, LshConfig *conf)
{
	assert(uhf != NULL && conf != NULL);
	int i = 0;

	uhf->u = (unsigned int*)malloc(conf->k * sizeof(unsigned int));	
	if(uhf->u == NULL)
	{
		fprintf(stderr, "can't allocate memory for uniform hash function.\n");		
		return -1;
	}
	for(i = 0; i < conf->k; i++)
	{
		uhf->u[i] = getRandomUns32(1, MAX_HASH_RND);
	}	
	
	return 0;
}

int comp(const void *a, const void *b)
{
	double a1 = *(double*)a;
	double b1 = *(double*)b;
	if(a1 - b1 == 0.0) return 0;
	else if(a1 - b1 > 0.0) return 1;
	else return -1;
}

unsigned long computeLsh(void *data, void *other)
{
	float *points, *input_point;
	int dimension;
	float r;
        float w;
        int k;
        int size;
	float p;
        int i, j;
        double sum1 = 0;
        long long sum2 = 0;
        unsigned long ret;
	double *tmp;
	int table_index = (long)other;
	
	assert(data != NULL);

	if(global_lsh_param == NULL) {
		fprintf(stderr, "Error: Need to initialize lsh parameters, exit.\n");
		exit(-1);
	}

	LshConfig *conf = global_lsh_param->config;
	HashFamily *hash_family = &global_lsh_param->hash_family[table_index];
	tmp = global_lsh_param->buf;

	r = conf->r;
	dimension = conf->dimension;
	w = conf->w;
	k = conf->k;
	p = conf->p;
	size = conf->size;	

	input_point = (float*)data;
	points = (float*)malloc(sizeof(float)*dimension);	
	if(NULL == points) {
		fprintf(stderr, "computeLsh: allocate memory failed, exit!\n");
		exit(-1);
	}
	for(i = 0; i < dimension; i++) {
		points[i] = input_point[i] / r;	
	}

        for(i = 0; i < k; i++) {
                sum1 = 0;
                for(j = 0; j < dimension; j++)
                {
                        sum1 += points[j] * hash_family->a[i][j];
                }
                sum1 += hash_family->b[i];
                sum1 /= w;
                tmp[i] = fabs(sum1);
        }

	if(conf->lsh_type == E2LSH) {
        	sum2 = 0;
        	for(i = 0; i < k; i++)
        	{
               	 /* the number in table->c is too big, so we do pre-mod to avoid overflow.*/
               	 sum2 += (((int)tmp[i] * hash_family->c[i]) % UH_PRIME_DEFAULT) ;
        	}
	} else if(conf->lsh_type == HAMMING) {
		sum2 = 0;	
		for(i = 0; i < k; i++) {
			tmp[i] = powf(tmp[i], p); 
		}
		qsort(tmp, k, sizeof(double), comp); 	
		sum2 = (int)(tmp[k/2] * FACTOR);
	} else {
		fprintf(stderr, "Error for computer LSH : unknown LSH type\n");
		exit(-1);
	}

        ret = sum2 % UH_PRIME_DEFAULT;
	free(points);
        return ret;
}

void freeHashFamily(HashFamily *hash_family, int num)
{
	float **a = hash_family->a;	
	float *b = hash_family->b;
	unsigned int *c = hash_family->c;
	int i = 0;
	
	for(i = 0; i < num; i++) {
		if(NULL != a[i]) free(a[i]);
	}
	if(NULL != b) free(b);
	if(NULL != c) free(c);
} 
	
void freeLshParam(LshParam *lsh_param)
{
	LshConfig *lsh_config = lsh_param->config;
	double *buf = lsh_param->buf;
	HashFamily *hash_family = lsh_param->hash_family;
	int num = lsh_config->l;
	int k = lsh_config->k;
	int i = 0;

	if(NULL != lsh_config) free(lsh_config);			
	if(NULL != buf) free(buf);

	for(i = 0; i < num; i++) {
		freeHashFamily(&hash_family[i], k); 		
	}
}
