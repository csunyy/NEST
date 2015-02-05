#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include "lsh.h"
#include "cuckoo.h"
#include "nest.h"

long skip_counter = 0;

int initNestParam(const char *config_file)
{
	LshParam *param;
	param = initLshParam(config_file);
	if(param == NULL) 
		return -1;

	return 1;
}

HashTable **hashTablesCreate(void (*free_data)(void *ptr), void (*free_info)(void *ptr))
{
	HashTable **hash_tables;
	int i = 0, index;	
	int num = global_lsh_param->config->l;
	int size = global_lsh_param->config->size;

	hash_tables = (HashTable**)malloc(sizeof(HashTable*)*num);	
	if(NULL == hash_tables) {
		goto error;
	}

	for( i = 0; i < num; i++) {
		index++;
		hash_tables[i] = tableCreate(size);		
		if(NULL == hash_tables[i]) {
			goto error;	
		}
		TableSetFreeDataMethod(hash_tables[i], free_data);
		TableSetFreeInfoMethod(hash_tables[i], free_info);
	}

	goto success;

	error:
		for(i = 0; i < index; i++) {
			tableRelease(hash_tables[i]);
		}		
		fprintf(stderr, "create hash tables failed.\n");
		return NULL;

	success:
		return hash_tables;	
}

Nest *nestCreate(HashTable **hash_tables)
{
	Nest *nest;
	hashfunc_t *hashfunc_list;
	int i = 0;
	int opt_pos;
	int table_num = global_lsh_param->config->l;
	int max_steps = global_lsh_param->config->max_steps;
	int func_num = table_num;
	int offset = global_lsh_param->config->offset;
	

	hashfunc_list = (hashfunc_t*)malloc(sizeof(hashfunc_t) * func_num);
	if(NULL == hashfunc_list)  goto error;
	for(i = 0; i < func_num; i++) {
		hashfunc_list[i] = computeLsh;
	}

	opt_pos = func_num * (2 * offset + 1);
	nest = cuckooInit(hash_tables, table_num, hashfunc_list, func_num, max_steps, opt_pos);	
	if(NULL == nest) goto error;	
	CuckooSetFindPosMethod(nest, nestFindOptPos);
	
	goto success;
		
	error:
		fprintf(stderr, "create nest failed.\n");	
		return NULL;

	success:
		return nest;
}

Nest *nestInsertItem(Nest *nest, void *data, void *info)
{
	return cuckooInsertItem(nest, data, info);
} 

int nestFindOptPos(Nest *nest, HashValue *hash_value, void *data)
{
	int num = 0, end = nest->func_num - 1;
	int start = 0, used_num = 0;
        unsigned long hv;
        HashTable *hash_table;
        Item *item;
        int func_index, table_index;
        int i = 0, j = 0;
	int offset;
	long new_hv;
	int nest_offset = global_lsh_param->config->offset;

        for(i = 0; i < nest->func_num; i++) {
                hash_table = nest->hash_tables[i];
                table_index = i;
		func_index = i;
                hv = nest->functions[i](data, (void*)table_index);
                hv = hv % hash_table->size;
                item = hash_table->items + hv;
                if(item->flag == USED) {
                        hash_value[end].hv = hv;
                        hash_value[end].func_index = func_index;
                        hash_value[end].table_index = table_index;
			hash_value[end].access_count = item->access_count;
                        end--;
			used_num++;
                } else {
                        hash_value[num].hv = hv;
                        hash_value[num].func_index = func_index;
                        hash_value[num].table_index = table_index;
			hash_value[num].access_count = item->access_count;
                        num++;
                }
        }
	
	start = nest->func_num;
	end = nest->opt_pos - 1; 

	if(num == 0) {
		for(i = 0; i < nest->func_num; i++) {
			hv = hash_value[i].hv;
			table_index = hash_value[i].table_index;
			func_index = hash_value[i].func_index;
			hash_table = nest->hash_tables[table_index];

			for(j = 0; j <= nest_offset * 2; j++) {
				offset = nest_offset - j;		
				if(offset == 0) continue;

				new_hv = (signed long)hv + offset;
				if(new_hv < 0 || new_hv >= hash_table->size) continue;					
				
				item = hash_table->items + new_hv;	

				if(item->flag == USED) {
					hash_value[start].hv = new_hv;
					hash_value[start].func_index = func_index;
					hash_value[start].table_index = table_index;  
					hash_value[start].access_count = item->access_count;
					start++;
					used_num++;
				} else {
					hash_value[end].hv = new_hv;
					hash_value[end].func_index = func_index;
					hash_value[end].table_index = table_index;
					hash_value[end].access_count = item->access_count;
					end--;
					num++;
				}
			}
		}	

		if(num == 0) return -used_num;	

		if(num > 0) {
			start = nest->opt_pos - num;
			for(i = 0; i < num; i++) {
				hash_value[i].hv = hash_value[start].hv;
				hash_value[i].func_index = hash_value[start].func_index;
				hash_value[i].table_index = hash_value[start].table_index;	
				hash_value[i].access_count = item->access_count;
				start++;
			}	
		}
	}

        return num;
}

void nestReport(Nest *nest)
{
	printf("total %ld data skiped.\n", skip_counter);
	cuckooReport(nest);
}

NNResult *nestGetNN(Nest *nest, void *data)
{
	NNResult *result;
	void **data_list, **info_list;
	HashTable **tables = nest->hash_tables;		
	HashTable *table;
	Item *items, *item;
	int table_num = nest->table_num;		
	int func_num = nest->func_num;
	int table_size;
	int i = 0, j = 0,  num = 0;
	int table_index;
	int offset;
	unsigned long hv;
	long new_hv;
	int nest_offset = global_lsh_param->config->offset;
	int opt_pos = func_num * (2 * nest_offset + 1);

	result = (NNResult*)malloc(sizeof(NNResult));
	if(NULL == result) {
		return NULL;
	}	
	data_list = (void**)malloc(sizeof(void*)*opt_pos);	
	if(NULL == data_list) {
		free(result);
		return NULL;
	}
	info_list = (void**)malloc(sizeof(void*)*opt_pos);
	if(NULL == info_list) {
		free(result);
		free(data_list);
		return NULL;
	}

	for(i = 0; i < table_num; i++) {
		table = tables[i];	
		table_index = i;
		table_size = table->size;
		items = table->items;
		hv = nest->functions[i](data, (void*)table_index); 
		hv = hv % table_size;

		for(j = 0; j <= nest_offset*2; j++) {
			offset = nest_offset - j; 
			new_hv = (signed long)hv + offset;	
			if(new_hv < 0 || new_hv >= table_size) continue;
			item = items + new_hv;
			if(item->flag == USED) {
				data_list[num] = item->data;	
				info_list[num] = item->info;
				num++;
			}		
		}	
	}
	result->num = num;
	result->data = data_list;	
	result->info = info_list;
			
	return result;	
}

void freeNNResult(NNResult *res)
{
	if(res != NULL) {
		if(res->data != NULL) free(res->data);
		if(res->info != NULL) free(res->info);
		free(res);
	}
}


void nestDestroy(Nest *nest)
{
	assert(nest != NULL);
	cuckooDestroy(nest);
}

void freeNestParam()
{
	freeLshParam(global_lsh_param);			
}

Nest *nestRemoveItem(Nest *nest, void *data, int (*match)(void *data, void *ptr)) 
{
	HashTable **tables = nest->hash_tables;		
	HashTable *table;
	Item *items, *item;
	int table_num = nest->table_num;		
	int func_num = nest->func_num;
	int table_size;
	int nest_offset = global_lsh_param->config->offset;
	int opt_pos = func_num * (2 * nest_offset + 1);
	int i = 0, j = 0,  num = 0;
	int table_index;
	int offset;
	unsigned long hv;
	long new_hv;

	for(i = 0; i < table_num; i++) {
		table = tables[i];	
		table_index = i;
		table_size = table->size;
		items = table->items;
		hv = nest->functions[i](data, (void*)table_index); 
		hv = hv % table_size;

		for(j = 0; j <= nest_offset*2; j++) {
			offset = nest_offset - j; 
			new_hv = (signed long)hv + offset;	
			if(new_hv < 0 || new_hv >= table_size) continue;
			item = items + new_hv;
			if(item->flag == USED) {
				if(NULL == match) return NULL;
				if(match(data, item->data) > 0) 
				{
					tableRemove(table, new_hv);
				}
				return nest;
			}		
		}	
	}
				
	return NULL;
}

void *nestGetItem(Nest *nest, void *data, int (*match)(void *data, void *ptr))
{
	HashTable **tables = nest->hash_tables;		
	HashTable *table;
	Item *items, *item;
	int table_num = nest->table_num;		
	int func_num = nest->func_num;
	int table_size;
	int nest_offset = global_lsh_param->config->offset;
	int opt_pos = func_num * (2 * nest_offset + 1);
	int i = 0, j = 0,  num = 0;
	int table_index;
	int offset;
	unsigned long hv;
	long new_hv;

	for(i = 0; i < table_num; i++) {
		table = tables[i];	
		table_index = i;
		table_size = table->size;
		items = table->items;
		hv = nest->functions[i](data, (void*)table_index); 
		hv = hv % table_size;

		for(j = 0; j <= nest_offset*2; j++) {
			offset = nest_offset - j; 
			new_hv = (signed long)hv + offset;	
			if(new_hv < 0 || new_hv >= table_size) continue;
			item = items + new_hv;
			if(item->flag == USED) {
				if(NULL == match) return NULL;
				if(match(data, item->data) > 0) 
				{
					return item->data;
				}
			}		
		}	
	}
	return NULL;	
}
