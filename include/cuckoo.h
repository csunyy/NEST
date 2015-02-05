/*
 * cuckoo hasing
 * author: hdzhang
 * date: 2013.11.20
*/

#ifndef _CUCKOO_H_
#define _CUCKOO_H_

/*
 * 定义哈希函数类型
 */
typedef unsigned long (*hashfunc_t)(void *data, void *other); 

/*
 * 哈希表中桶的两种状态
 */
#define USED 1
#define FREE 0

/* 缓冲区大小 */
#define BUFSIZE 1024

/*
 * Item: 哈希表中的每个元素
 * data: 哈希表中存储的数据 
 * flag: 标记当前桶是否已经被占用, 0表示空闲， 1表示被占用
 * info: item中的一些附加信息
 */
typedef struct Item {

	void *data;
	char flag;
	void *info;
	int access_count;

} Item;

/*
 * HashTable: 哈希表数据结构
 * items: 哈希表
 * size: 哈希表大小
 * used: 哈希表已经使用的大小
 * load: 装载率, load = used/size
 * fnum: 哈希函数的数目
 * functions: 指向哈希函数数组的指针
 */
typedef struct HashTable {
	
	Item *items;
	unsigned long  size;
	unsigned long  used;
	float load;
	void (*free_data)(void *ptr);
	void (*free_info)(void *ptr);

} HashTable;

/*
 * 将数据插入哈希表中使用的中间数据结构
 * hv: 计算出的哈希值
 * func_index: 标示是第几个哈希函数计算出的哈希值
 * table_index: 标示哈希表
 */
typedef struct HashValue {

	unsigned long hv;
	int func_index;
	int table_index;
	int access_count;

} HashValue;

typedef struct CuckooHash CuckooHash;

/*
 * 定义用于查找可kick out的函数
 */
typedef int (*findOptPos_t)(CuckooHash *cuckoo_hash, HashValue *hash_value, void *data);


/*
 * cuckoohash: cuckoo hashing数据结构
 * hash_tables: 哈希表数组 
 * table_num: 哈希表数目
 * igned long hv)
 * functions: 哈希函数列表
 * func_num: 哈希函数数目
 * max_steps: 最大kick out次数，超过则认为失败
 * hv_arr: 用于管理可选位置的缓冲区
 * opt_pos: 可选择位置数目
 * 使用限制：hashtable的数目有两种情况，一种是只有一个，另一种是有多个
 * 如果只有一个hashtable，func_num的数目没有要求，kick out将在同一个哈希表中进行
 * 如果有多个hashtable，func_num的数目必须如之相同，kick out操作将在多个哈希表间进行
 */
struct CuckooHash {
	
	HashTable **hash_tables;	
	int table_num;	
	hashfunc_t *functions;
	int func_num;
	int max_steps;
	HashValue *hv_arr;
	int opt_pos;
	findOptPos_t  find_opt_pos;
	long long kickout_counter;

};



/* functions defined as macros */
#define TableSetFreeDataMethod(table, m)  ((table)->free_data = (m))
#define TableSetFreeInfoMethod(table, m)  ((table)->free_data = (m))
#define CuckooSetFindPosMethod(cuckoo, m) ((cuckoo)->find_opt_pos = (m))

/* functions  prototypes */

HashTable *tableCreate(unsigned long size);

void tableRelease(HashTable *table);

HashTable *tableInsert(HashTable *table, unsigned long hv, void *data, void *info);

void tableRemove(HashTable *table, unsigned long hv);

void *tableGetItemData(HashTable *table, unsigned long hv);

void *tableGetItemInfo(HashTable *table, unsigned long hv);

void tableKickOut(HashTable *table, unsigned long hv, void *data, void *info, void **old_data, void **old_info);

CuckooHash *cuckooInit(HashTable **hash_tables, int table_num, hashfunc_t *funcs, int func_num, int max_steps, int opt_pos);

void cuckooDestroy(CuckooHash *cuckoo_hash);

CuckooHash *cuckooInsertItem(CuckooHash *cuckoo_hash, void *data, void *info);

CuckooHash *cuckooInsertFile(CuckooHash *cuckoo_hash, const char * filename);

int cuckooFindPos(CuckooHash *cuckoo_hash, HashValue *hash_value, void *data);

void cuckooReport(CuckooHash *cuckoo_hash);

#define _EVALUATION_

#ifdef _EVALUATION_

extern double insert_items_time;
extern double insert_item_time;
extern double insert_file_time;

#endif


#endif

