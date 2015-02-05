#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "nest.h"

float computeDistance(float *point1, float *point2, int dimension);
int nestInsertFile(Nest *nest, const char *filename);

int main(int argc, char *argv[])
{
	char *config_file;
	char *trace_file;
	HashTable **hash_tables;
	Nest *nest;
	NNResult *res;

	int dimension = 5;
	float point[] = {84.0188, 39.4383, 78.3099, 79.8440, 91.1647};

	float *p;
	int i, j;

	if(argc != 2) {
		printf("Usage: %s trace_file\n", argv[0]); 
		return -1;
	}
	config_file = "nest.conf";
	trace_file = argv[1];
	

	if(initNestParam(config_file) < 0) {
		fprintf(stderr, "init nest param failed.\n");
		return -1;
	}
	
	hash_tables = hashTablesCreate(free, NULL);		
	if(hash_tables == NULL) {
		fprintf(stderr, "create hash tables failed.\n");
		return -1;
	}
	

	nest = nestCreate(hash_tables);
	if(nest == NULL) {
		fprintf(stderr, "create nest failed.\n");
		return -1;
	}
	
	nestInsertFile(nest, trace_file);		

	nestReport(nest);

	res = nestGetNN(nest, point); 

	printf("\n Input vector:\n");
	for(i = 0; i < dimension; i++) {
		printf("%.4f  ", point[i]);
	}
	
	printf("\n ANN result:\n");
	for(i = 0; i < res->num; i++) {

		p = ((float**)res->data)[i];	
		printf("\nDistance : %.4f\n", computeDistance(point, p, dimension));

		for(j = 0; j < dimension; j++) {
			printf("%.4f  ", p[j]);
		}
		printf("\n");
	}
	freeNNResult(res);		

	nestDestroy(nest);
	freeNestParam();
	
	return 0;	
}

float computeDistance(float *point1, float *point2, int dimension)
{
	float res = 0.0;	
	int i = 0;
	for(i = 0; i < dimension; i++) {
		res += (point1[i] - point2[i]) * (point1[i] - point2[i]);
	}
	return sqrt(res);
}

int nestInsertFile(Nest *nest, const char *filename)
{
        FILE *fd;
        int counter = 0;
        int dimension;
        float *point = NULL, f;
        int i = 0;

        dimension = global_lsh_param->config->dimension;

        if((fd = fopen(filename, "r")) == NULL) {
                fprintf(stderr, "can't open file %s\n", filename);
                return -1;
        }

        while(fscanf(fd, "%f", &f) != EOF) {
                if(NULL == point) {
                        point = (float*)malloc(sizeof(float)*dimension);
                        if(NULL == point) {
                                fprintf(stderr, "nestInsertFile: allocate memory failed.\n");
                                return counter;
                        }
                }
                point[i++] = f;
                if(i % dimension == 0) {
                        i = 0;
                        assert(point != NULL);
                        if(NULL == nestInsertItem(nest, point, NULL))
                                break;
                        counter++;
                        point = NULL;
                }
        }

        fclose(fd);
        return counter;
}
