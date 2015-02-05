/*
 *cuckoo hasing
 *author: hdzhang
 *date: 2013.11.20
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include "cuckoo.h"
#include "lsh.h"

#ifdef _EVALUATION_
double insert_items_time = 0.0;
double insert_item_time = 0.0;
double insert_file_time = 0.0;
#endif

HashTable *tableCreate(unsigned long size)
{
	HashTable *table;
	Item *items;

	if((table = (HashTable*)malloc(sizeof(HashTable))) == NULL) {
		fprintf(stderr, "memory allocates failed, %s:%d", __FILE__, __LINE__);
		return NULL;
	}

	if((items = (Item*)malloc(sizeof(Item)*size)) == NULL) {
		free(table);
		fprintf(stderr, "memory allocates failed, %s:%d", __FILE__, __LINE__);
		return NULL;
	}

	table->size = size;
	table->free_data = NULL;
	table->free_info = NULL;
	table->used = 0;
	table->load = 0.0;

	memset(items, 0, sizeof(Item)*size);
	table->items = items;
	return table;	
}

void tableRelease(HashTable *table)
{
	int i = 0;
	int size = table->size;
	Item *items = table->items;
	if(table == NULL)
		return ;
		
	for(i = 0; i < size; i++) {
		if(items[i].data != NULL && table->free_data != NULL) 
			table->free_data(items[i].data);
		if(items[i].info != NULL && table->free_info != NULL)
			table->free_info(items[i].info);
	}

	free(items);
	free(table);
}

HashTable *tableInsert(HashTable *table, unsigned long hv, void *data, void *info)
{
	Item *item;		
	unsigned long size = table->size;
	hv = hv % size;
	item = table->items + hv;
	if(item->flag == USED) {
		return NULL;		
	} else {
		item->data = data;	
		item->info = info;
		item->flag = USED;
		table->used++;
	}
	return table;
}

void tableRemove(HashTable *table, unsigned long hv)
{
	Item *item;
	unsigned long size = table->size;
	hv = hv % size;
	item = table->items + hv;
	if(item->flag == FREE) {
		return;
	} else {
		if(table->free_data && item->data != NULL) {
			table->free_data(item->data);	
		}
		if(table->free_info && item->info != NULL) {
			table->free_info(item->info);
		}
		item->flag = FREE;
		table->used--;			
	}
}

void *tableGetItemData(HashTable *table, unsigned long hv)
{
	Item *item;
	unsigned long size = table->size;	
	hv = hv % size;
	item = table->items + hv;
	if(item->flag == FREE) {
		return NULL;
	} else {
		return item->data;
	} 
}

void *tableGetItemInfo(HashTable *table, unsigned long hv)
{		
	Item *item;
	unsigned long size = table->size;
	hv = hv % size;
	item = table->items + hv;
	if(item->flag == FREE) {
		return NULL;
	} else {
		return item->info;
	}
}

void tableKickOut(HashTable *table, unsigned long hv ,void *data, void *info, void **old_data, void **old_info)
{
	Item *item;
	hv = hv % table->size;		
	item = table->items + hv;
	assert(item->flag == USED);
	*old_data = item->data;
	*old_info = item->info;
	item->data = data;
	item->info = info;
	item->access_count++;
}

int cuckooFindPos(CuckooHash *cuckoo_hash, HashValue *hash_value, void *data)
{
	int num = 0, end = cuckoo_hash->opt_pos - 1;
	unsigned long hv;
	HashTable *hash_table;
	Item *item;
	int func_index, table_index;
	int i = 0;		
	
	for(i = 0; i < cuckoo_hash->func_num; i++) {
		func_index = i;
		if(cuckoo_hash->table_num == 1) {
			hash_table = cuckoo_hash->hash_tables[0];
			table_index = 0;
		} else {
			hash_table = cuckoo_hash->hash_tables[i];
			table_index = i;
		}
		hv = cuckoo_hash->functions[i](data, NULL);
		hv = hv % hash_table->size;
		item = hash_table->items + hv;
		if(item->flag == USED) {
			hash_value[end].hv = hv;
			hash_value[end].func_index = func_index;	
			hash_value[end].table_index = table_index;
			end--;
		} else {
			hash_value[num].hv = hv;
			hash_value[num].func_index = func_index;			
			hash_value[num].table_index = table_index;
			num++;
		}	
	}
	if(num == 0) return -(cuckoo_hash->func_num);
	return num;
} 

CuckooHash *cuckooInit(HashTable **hash_tables, int table_num, hashfunc_t *funcs, int func_num, int max_steps, int opt_pos)
{
	CuckooHash *cuckoo_hash;
		
	if(table_num !=1 && table_num != func_num) {
		return NULL;
	}

	if((cuckoo_hash = (CuckooHash*)malloc(sizeof(CuckooHash))) == NULL) {
		return NULL;
	}
	
	cuckoo_hash->hash_tables = hash_tables;
	cuckoo_hash->functions = funcs;
	cuckoo_hash->table_num = table_num;
	cuckoo_hash->func_num = func_num;
	cuckoo_hash->max_steps = max_steps;
	cuckoo_hash->hv_arr = (HashValue*)malloc(sizeof(HashValue)*opt_pos);
	if(NULL == cuckoo_hash->hv_arr) {
		free(cuckoo_hash);
		return NULL;
	}
	memset(cuckoo_hash->hv_arr, 0, sizeof(HashValue)*opt_pos);
	cuckoo_hash->opt_pos = opt_pos;
	cuckoo_hash->kickout_counter = 0;
	
	return cuckoo_hash;
}

void cuckooDestroy(CuckooHash *cuckoo_hash)
{
	assert(cuckoo_hash != NULL);
	int table_num = cuckoo_hash->table_num;
	HashTable **tables = cuckoo_hash->hash_tables;
	hashfunc_t *functions = cuckoo_hash->functions;
	HashValue *hv_arr = cuckoo_hash->hv_arr;
	int i = 0;
	
	for(i = 0; i < table_num; i++) {
		if(NULL != tables[i])
			tableRelease(tables[i]);	
	}

	if(NULL != functions) free(functions);
	if(NULL != hv_arr) free(hv_arr);

	free(cuckoo_hash);	
}

int ch_comp(const void *a, const void *b)
{
	HashValue hv1 = *(HashValue*)a;			
	HashValue hv2 = *(HashValue*)b;
	return hv1.access_count - hv2.access_count;
}

CuckooHash* cuckooInsertItem(CuckooHash *cuckoo_hash, void *data, void *info) 
{
	int func_num = cuckoo_hash->func_num;
	int max_steps = cuckoo_hash->max_steps; 
	int mid = max_steps / 2;
	int free_pos, pos, table_index;
	void *old_data, *old_info;
	int opt_pos;

	int pre_kickout_flag = 0; 

	HashValue pre_hash_value;

	HashValue *hv_arr;	
	
	int optimize_flag = global_lsh_param->config->optimize_kickout;

	hv_arr = cuckoo_hash->hv_arr;

	#ifdef  _EVALUATION_
	struct timeval start, end;
	gettimeofday(&start, NULL);
	#endif

	while(max_steps >= 0) {
		free_pos = cuckoo_hash->find_opt_pos(cuckoo_hash, hv_arr, data);
		if(free_pos > 0) {
			pre_kickout_flag = 0;

			if(optimize_flag == OPTIMIZE_ON && max_steps < mid) {
				qsort(hv_arr, free_pos, sizeof(HashValue), ch_comp); 
				pos = 0;
			} else {  
				pos = rand() % free_pos;	
			}
			table_index = hv_arr[pos].table_index;
			if(tableInsert(cuckoo_hash->hash_tables[table_index], hv_arr[pos].hv, data, info) == NULL) {
				fprintf(stderr, "Error: insert used position, have to exit!\n");				
				exit(-1);
			}	
			break;
		} else if(max_steps != 0 && free_pos != 0) { 

			opt_pos = -free_pos;

			if(optimize_flag == OPTIMIZE_ON && max_steps < mid) {
				qsort(hv_arr, opt_pos, sizeof(HashValue), ch_comp); 
				pos = 0;
			} else {  
				pos = rand() % opt_pos;
			}

			table_index = hv_arr[pos].table_index;

			if(pre_kickout_flag == 1) {
				 if(table_index == pre_hash_value.table_index && \
				     hv_arr[pos].hv == pre_hash_value.hv) {
					pos++;
					pos = pos % opt_pos;
					table_index = hv_arr[pos].table_index;
				}   
			}

			tableKickOut(cuckoo_hash->hash_tables[table_index], hv_arr[pos].hv, data, info, &old_data, &old_info);				
			data = old_data;
			info = old_info;
	
			pre_kickout_flag = 1;
			pre_hash_value.hv = hv_arr[pos].hv;
			pre_hash_value.func_index = hv_arr[pos].func_index;
			pre_hash_value.table_index = hv_arr[pos].table_index;
		}

		max_steps--;
	}	

	cuckoo_hash->kickout_counter += (cuckoo_hash->max_steps - max_steps);

	#ifdef _EVALUATION_
	gettimeofday(&end, NULL);
	insert_item_time = 1000000*(end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
	insert_items_time += insert_item_time;	
	#endif

	if(max_steps < 0) {
		return NULL;	
	}
	return cuckoo_hash;
}

CuckooHash *cuckooInsertFile(CuckooHash *cuckoo_hash, const char * filename)
{
	FILE *fd;	
	char buf[BUFSIZE]; 
	char *data;
	int len;

	if((fd = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "can't open file %s\n", filename);
		return NULL;
	}	
	#ifdef _EVALUATION_
	struct timeval start, end;
	gettimeofday(&start, NULL);
	insert_items_time = 0.0;
	#endif
	while(fgets(buf, BUFSIZE, fd)) {
		len = strlen(buf);
		data = (char*)malloc(len);
		if(data == NULL) {
			fprintf(stderr, "Allocate memory failed: exit!\n");
			exit(-1);
		}	
		memcpy(data, buf, len);	
		data[len-1] = '\0';
		if(cuckooInsertItem(cuckoo_hash, data, NULL) == NULL) {
			break;	
		} 
	}	

	#ifdef _EVALUATION_
	gettimeofday(&end, NULL);
	insert_file_time = 1000000*(end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
	#endif

	fclose(fd);
	return cuckoo_hash;
}

void cuckooReport(CuckooHash *cuckoo_hash)
{
	HashTable *hash_table;
	int num = cuckoo_hash->table_num;
	unsigned long  size_sum = 0, used_sum = 0;
	int i = 0;
	float load;
	printf("\t size \t used \t load\n");
	for(i = 0; i < num; i++) {
		hash_table = cuckoo_hash->hash_tables[i];
		load = (float)hash_table->used / (float)hash_table->size;
		size_sum += hash_table->size;
		used_sum += hash_table->used;
		printf("table%d\t%ld\t%ld\t%f\n", i, hash_table->size, hash_table->used, load);
	}
		
	printf("SUM\t%ld\t%ld\t%f\n", size_sum, used_sum, (float)used_sum / (float)size_sum);
	printf("Kick out Times : %lld\n", cuckoo_hash->kickout_counter);
	#ifdef _EVALUATION_
	printf("Insert Items Time: %.2lfus\n", insert_items_time);	
	printf("Insert Item Time(last one) : %.2lfns\n", insert_item_time * 1000);
	#endif
}

