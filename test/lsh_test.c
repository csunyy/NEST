#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "lsh.h"



typedef struct Bucket {

	void *data;	
	unsigned long hv;
	struct Bucket *next;

} Bucket;

typedef struct Table {

	Bucket *bucket_list;
	int used;
	int size;

} Table;


void tableInsert(Table *table_list, void *points);
void tableReport(Table *table_list);
float computeMinDistance(Bucket *buckets);
float getMinDistance(Bucket *bucket, Bucket *bucket_list);


int main(int argc, char *argv[])
{
	int size;		
	int table_num;
	int dimension;
	int i, j;
	Table *table_list;
	Bucket *bucket_list;
	FILE *fd;
	float f;
	float *points = NULL;
	
	if(argc != 3) {
		printf("Usage: %s trace_file  config_file\n", argv[0]);
		return -1;
	}

	fd = fopen(argv[1], "r");
	if(fd == NULL) {
		fprintf(stderr, "can't open file : %s\n", argv[1]);
		return -1;
	}
	
	//init lsh parameters		
	if(initLshParam(argv[2]) == NULL) {
		fprintf(stderr, "init lsh parameters failed.\n");		
		return -1;
	}
	
	size = global_lsh_param->config->size;				
	table_num = global_lsh_param->config->l;
	dimension = global_lsh_param->config->dimension;

	//init hash table
	table_list = (Table*)malloc(sizeof(Table)*table_num);
	if(table_list == NULL) {
		printf("allocate memory for table_list failed, exit!\n");
		return -1;
	}
	for(i = 0; i < table_num; i++) {
		bucket_list = (Bucket*)malloc(sizeof(Bucket)*size);	
		if(bucket_list == NULL) exit(-1);	
		memset(bucket_list, 0, sizeof(Bucket)*size);
		table_list[i].bucket_list = bucket_list; 
		table_list[i].used = 0;
		table_list[i].size = size;
	}	

	i = 0;
	while(fscanf(fd, "%f", &f) != EOF) {
		if(NULL == points) {
			points = (float*)malloc(sizeof(float)*dimension);
		}			
		points[i++] = f;
		if(i % dimension == 0) {
			i = 0;
			tableInsert(table_list, points);
			points = NULL;
		}
	} 	

	//report result
	tableReport(table_list);
				
		
	fclose(fd);
	return 0;
}


void tableInsert(Table *table_list, void *points)
{
	int table_num, size;
	int i = 0;
	unsigned long  bucket_id, hv;
	Bucket *bucket_list;
	Bucket *bucket;
	Bucket *new_bucket;

	table_num = global_lsh_param->config->l;
	size = global_lsh_param->config->size;

	for(i = 0; i < table_num; i++) {
		hv = computeLsh(points, (void*)i);	
		bucket_id = hv % size;	
		bucket_list = table_list[i].bucket_list;
		bucket = &bucket_list[bucket_id];
		if(bucket->data == NULL) {
			table_list[i].used++;
			bucket->data = points;
			bucket->hv = hv;
			bucket->next = NULL; 
		} else {
			new_bucket = (Bucket*)malloc(sizeof(Bucket));		
			new_bucket->data = points;
			new_bucket->hv = hv;
			new_bucket->next = bucket->next;
			bucket->next = new_bucket;
		}
		
	}
} 



void tableReport(Table *table_list)
{
	int table_num, size, dimension;
	int i = 0, j = 0, k = 0;		
	float min_distance;
	char file_name[1024];
	char *name_prefix = "lsh_result";
	char *name_suffix = ".txt";
	FILE *fd;
	Bucket *bucket;
	Bucket *bucket_list;
	Bucket *current;
	float *points;
	
	table_num = global_lsh_param->config->l;
	size = global_lsh_param->config->size;
	dimension = global_lsh_param->config->dimension;
		
	for( i = 0; i < table_num; i++) {
		sprintf(file_name, "%s%d%s", name_prefix, i, name_suffix);
		fd = fopen(file_name, "w");
		if(fd == NULL) {
			fprintf(stderr, "can't open or create file : %s\n", file_name);
			return ;	
		} 	
		printf("table#%d:\n", i);
		printf("used: %d\n", table_list[i].used);

		bucket_list = table_list[i].bucket_list;

		for( j = 0; j < size; j++) {

			fprintf(fd, "\nbucket#%d:\n", j);
			bucket = &bucket_list[j]; 
			if(bucket->data == NULL) {
				fprintf(fd, "empty\n");
			} else {
				min_distance = computeMinDistance(bucket);
				fprintf(fd, "min distance: %.4f\n", min_distance);
				current = bucket;
				while(current != NULL) {
					points = (float*)current->data;
					fprintf(fd, "hv = %lu   ", current->hv);
					for(k = 0; k < dimension; k++) {
						fprintf(fd, "%.4f%s", points[k], "  "); 
					}	
					fprintf(fd, "\n");
					current = current->next;	
				}		
			}
		}			
		fclose(fd);	
		printf("results stored in %s\n", file_name);
	}
}



float computeMinDistance(Bucket *buckets)
{
	Bucket *current;	
	float min_distance = 1e20;
	float distance; 

	current = buckets;
	while(current != NULL) {
		
		distance = getMinDistance(current, current->next);	
		if(distance > 0 && distance < min_distance)
			min_distance = distance;
		current = current->next;	
	}
	return min_distance;
}


float getMinDistance(Bucket *bucket, Bucket *bucket_list)
{
	float *a_points, *b_points;
	float min_distance = 1e20;
	float distance;	
	Bucket *current = bucket_list;
	int dimension;
	int i;

	if(NULL == bucket_list) return -1;
	
	dimension = global_lsh_param->config->dimension;
	a_points = (float*)bucket->data;
	
	while(current != NULL) {
		b_points = (float*)current->data;	
		for(i = 0; i < dimension; i++) {
			distance += (a_points[i] - b_points[i])*(a_points[i] - b_points[i]);
		}	
		distance = sqrt(distance);
		if(distance < min_distance) 
			min_distance = distance;
		current = current->next;
	}	
	return min_distance;
}


