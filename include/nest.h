#ifndef _LSH_CUCKOO_H_
#define _LSH_CUCKOO_H_

#include "cuckoo.h"
#include "lsh.h"

/*
 * 在每个哈希表中左右扩展的数目， 在Nest中为1
 */

typedef struct NNResult {
	int num;
	void **data;
	void **info;
} NNResult;

typedef CuckooHash Nest; 

//跳过的全零位向量计数
extern long skip_counter;

//虽然没有用到Nest，不过这可以保证不会在没有初始化配置时调用这个宏
#define NestGetDimension(nest) (global_lsh_param->config->dimension)

#define NestSetFindPosMethod(nest, m) ((nest)->find_opt_pos = (m))
#define NestSetHashMethods(nest, m) ((nest)->functions = (m))


int initNestParam(const char *config_file);

HashTable **hashTablesCreate(void (*free_data)(void *ptr), void (*free_info)(void *ptr));

Nest *nestCreate(HashTable **hash_tables);

Nest *nestInsertItem(Nest *nest, void *data, void *info);

Nest *nestRemoveItem(Nest *nest, void *data, int (*match)(void *data, void *ptr));

int nestFindOptPos(Nest *nest, HashValue *hash_value, void *data);

void nestReport(Nest *nest);

NNResult *nestGetNN(Nest *nest, void *data);

void *nestGetItem(Nest *nest, void *data, int (*match)(void *data, void *ptr));

void freeNestParam();

void nestDestroy(Nest *nest);

void freeNNResult(NNResult *res);

#endif
