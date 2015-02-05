#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lsh.h"

#define SIM 1
#define RAN 2
#define SIM_FACTOR 10
#define OFFSET_NUM 7 

void usage(const char *arg)
{
	printf("Usage: %s dimension max_value points_num trace_file S|R \n", arg);
	printf("dimension: dimension of vector\n");
	printf("max_value: max value in elements of vector\n");
	printf("points_num: number of points\n");
	printf("trace_file: trace output file\n");
	printf("S|R: 'S' means to generator similary vectors, 'R' means random vectors.\n");
	
}

int main(int argc, char *argv[])
{
	int dimension;	
	float  max_value;
	int n_points;
	char *trace_file;
	FILE *fd;
	int i, j, k;
	float res;
	int simi_factor;
	int offset[] = {-3, -2, -1, 0, 1, 2, 3};
	int offset_index;
	float *points;
	char tmp;
	int simi_flag  = -1;
	int counter = 0;

	if(argc != 6) {
		usage(argv[0]);
		return -1;
	}	
	
	dimension = atoi(argv[1]);

	tmp = argv[5][0];	
	if(tmp == 's' || tmp == 'S') {
		simi_flag = SIM;	
	}
	if(tmp == 'r' || tmp == 'R') {
		simi_flag = RAN;
	}
	if(simi_flag == -1) {
		usage(argv[0]);	
		return -1;
	}


	points = (float*)malloc(sizeof(float)*dimension);		

	if(NULL == points) {
		fprintf(stderr, "memory error ,exit.\n");
		exit(-1);
	} 

	max_value = atof(argv[2]);

	if(max_value <= 0) {
		fprintf(stderr, "max_value must be positive.\n");
		return -1;
	}

	n_points = atoi(argv[3]);
	trace_file = argv[4];

	fd = fopen(trace_file, "w");
	if(fd == NULL) {
		fprintf(stderr, "can't open file : %s\n", trace_file);
		return -1;
	}	

	if(simi_flag == SIM) {
		simi_factor = SIM_FACTOR; 
		n_points = n_points / simi_factor;
	} else {
		simi_factor = 0;	
	} 

	for(i = 0; i < n_points; i++) {
		for(j = 0; j < dimension; j++) {
			res = getUniformRandom(0, max_value);	
			points[j] = res;
			fprintf(fd, "%.4f%s", res, " "); 
		}
		fprintf(fd, "\n");
		counter++;

		for(j = 0; j < simi_factor - 1; j++) {
			for(k = 0; k < dimension; k++) {
				offset_index = rand() % OFFSET_NUM;		
				fprintf(fd, "%.4f%s", points[k] + offset[offset_index], " ");
			}
			fprintf(fd, "\n");
			counter++;
		}
	}	

	fclose(fd);	
	printf("total %d, OK!\n", counter);
	return 0;
}
