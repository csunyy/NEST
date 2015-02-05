#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cuckoo.h"

/* hash functions */
unsigned long RSHash(void *data,  void *other);

unsigned long SDBMHash(void *data, void *other);

unsigned long JSHash(void *data, void *other);

unsigned long PJWHash(void *data, void *other);

unsigned long ELFHash(void *data, void *other);

unsigned long BKDRHash(void *data, void *other);

unsigned long DJBHash(void *data, void *other);

unsigned long APHash(void *data, void *other);

int main(int argc, char *argv[])
{
	char *filename;
	int table_num, func_num, max_steps;
	char *token, *str;
	char *delim = ":";
	unsigned long *size_list;
	HashTable **hash_tables;
	hashfunc_t *func_list;
	CuckooHash *cuckoo_hash;
	int i = 0;
	
	hashfunc_t hash_functions[] = {RSHash, SDBMHash, JSHash, PJWHash, ELFHash, BKDRHash, DJBHash, APHash};
	
	if(argc != 6) {
		printf("USAGE: %s trace_file table_num func_num(<=8) size1:size2:...:sizen max_steps \n", argv[0]);
		return -1;
	}	

	filename = argv[1];
	table_num = atoi(argv[2]);	
	func_num = atoi(argv[3]);
	max_steps = atoi(argv[5]);
	
	
	if(func_num > 8) {
		printf("Too many hash funtions, the max number is 8.\n");
		return -1;
	}
	
	size_list = (unsigned long*)malloc(sizeof(unsigned long)*table_num);
	hash_tables = (HashTable**)malloc(sizeof(HashTable *)*table_num);	
	func_list = (hashfunc_t*)malloc(sizeof(hashfunc_t)*func_num);
	if(size_list == NULL || hash_tables == NULL || func_list == NULL) 
		return -1;
	
	for(str = argv[4], i = 0; i < table_num ; str = NULL, i++) {
		token = strtok(str, delim);
		if(token == NULL) break;
		size_list[i] = atoi(token);
	}	
	if(i != table_num) {
		printf("the number of tablesize is wrong, should be %d.\n", table_num);
		return -1;
	}  
	
	for(i = 0; i < func_num; i++) {
		func_list[i] = hash_functions[i];
	}	

	for(i = 0; i < table_num; i++) {
		hash_tables[i] = tableCreate(size_list[i]);	
		if(hash_tables[i] == NULL) {
			printf("Init hash tables failed, exit!\n");
			return -1;
		}
		TableSetFreeDataMethod(hash_tables[i], free);	
		TableSetFreeInfoMethod(hash_tables[i], NULL);
		
	}
	
	cuckoo_hash = cuckooInit(hash_tables, table_num, func_list, func_num, max_steps, func_num);
	CuckooSetFindPosMethod(cuckoo_hash, cuckooFindPos);

	if(cuckoo_hash == NULL) {
		printf("Init cuckoo hash failed, exit!\n");
		return -1;
	}
	
	cuckooInsertFile(cuckoo_hash, filename);		
	cuckooReport(cuckoo_hash);

	return 0;	
}

unsigned long RSHash(void *data, void *other)
{
	unsigned long b = 378551;
	unsigned long a = 63689;
	unsigned long hash = 0;
	char *str = (char*)data;
	int size = strlen(str);
 
	while (size--)
	{
		hash = hash * a + (*str++);
		a *= b;
	}
 
	return (hash & 0x7FFFFFFF);
}



//SDBMHash
unsigned long SDBMHash(void *data, void *other)
{
	unsigned long hash = 0;
	char *str = (char*)data;	
	int size = strlen(str);
	 
	while (size--)
	{
		// equivalent to: hash = 65599*hash + (*str++);
		hash = (*str++) + (hash << 6) + (hash << 16) - hash;
	}
 
	return (hash & 0x7FFFFFFF);
}


//JSHash
unsigned long JSHash(void *data, void *other)
{
	unsigned long hash = 1315423911;
	char *str = (char*)data; 
	int size = strlen(str);
	
	while (size--)
	{
		hash ^= ((hash << 5) + (*str++) + (hash >> 2));
	}
 
	return (hash & 0x7FFFFFFF);	
}


//P.J.Weinberger Hash
unsigned long PJWHash(void *data, void *other)
{
	unsigned int BitsInUnignedInt = (unsigned int)(sizeof(unsigned int) * 8);
	unsigned int ThreeQuarters	= (unsigned int)((BitsInUnignedInt  * 3) / 4);
	unsigned int OneEighth = (unsigned int)(BitsInUnignedInt / 8);
	unsigned int HighBits = (unsigned int)(0xFFFFFFFF) << (BitsInUnignedInt - OneEighth); 

	unsigned long hash	= 0;
	unsigned long test	= 0;

	char *str = (char*)data;
	int size = strlen(str);

	while (size--)
	{
		hash = (hash << OneEighth) + (*str++);
		if ((test = hash & HighBits) != 0)
		{
			hash = ((hash ^ (test >> ThreeQuarters)) & (~HighBits));
		}
	}
 
	return (hash & 0x7FFFFFFF);
}


//ELF Hash
unsigned long ELFHash(void *data, void *other)
{
	unsigned long hash = 0;
	unsigned long x	= 0;
	char *str = (char*)data;
	int size = strlen(str);
 
	while (size--)
	{
		hash = (hash << 4) + (*str++);
		if ((x = hash & 0xF0000000L) != 0)
		{
			hash ^= (x >> 24);
			hash &= ~x;
		}
	}
 
	return (hash & 0x7FFFFFFF);
}


//BKDRHash
unsigned long BKDRHash(void *data, void *other)
{
	unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
	unsigned long  hash = 0;

	char *str = (char*)data;
	int size = strlen(str);
 
	while (size--)
	{
		hash = hash * seed + (*str++);
	}
 
	return (hash & 0x7FFFFFFF);
}



//DJB Hash
unsigned long DJBHash(void *data, void *other)
{
	unsigned long hash = 5381;
	char *str = (char*)data; 
	int size = strlen(str);

	while (size--)
	{
		hash += (hash << 5) + (*str++);
	}
 
	return (hash & 0x7FFFFFFF);
}


//AP Hash
unsigned long APHash(void *data, void *other)
{
	unsigned long hash = 0;
	int i;
	char *str = (char*)data;
	int size = strlen(str);
	
	for (i=0; i < size; i++)
	{
		if ((i & 1) == 0)
		{
			hash ^= ((hash << 7) ^ (*str++) ^ (hash >> 3));
		}
		else
		{
			hash ^= (~((hash << 11) ^ (*str++) ^ (hash >> 5)));
		}
	}
 
	return (hash & 0x7FFFFFFF);
}

