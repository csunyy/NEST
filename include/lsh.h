#ifndef _H_HASH
#define _H_HASH

/*
 * 用于保存lsh和nest配置文件的参数
 * 具体含义参见nest.conf文件
 */

#define FACTOR 1.425

#define OPTIMIZE_ON 1
#define OPTIMIZE_OFF 0

typedef enum LSH_TYPE {E2LSH, HAMMING} LSH_TYPE;


typedef struct LshConfig 
{ 
	float r;
	float w;
	int k;
	int l;
	int size;
	int dimension;
	int offset;
	int max_steps;
	float p;
	LSH_TYPE lsh_type;
	int optimize_kickout;

} LshConfig ;

/*
 * 用于lsh计算的函数族
 * 计算公式: (av + b) / w
 * 结构体中的变量a,b与公式中的相对应, c用于最后计算出哈希值
 */
typedef struct HashFamily
{
	float **a;
	float *b;
	unsigned int *c;

} HashFamily;

/*
 * 用于LSH的参数，集成了配置文件中的设置与函数族
 */
typedef struct LshParam {
		
	LshConfig *config;	
	HashFamily *hash_family;
	double *buf; //保存中间结果的缓冲区
	
} LshParam;

/*
 * 均匀分配的多维向量，用于LshConfig中的c变量
 */
typedef struct UniformHashFunction
{
	unsigned int *u;	

} UniformHashFunction;

extern LshParam *global_lsh_param;

LshParam *initLshParam(const char *config_file);

int loadConfigFile(const char* filename, LshConfig *conf);

int initHashFamily(LshConfig *conf, HashFamily *hash_family, UniformHashFunction *uhf,  int num);

int initUniformHashFunction(UniformHashFunction *uhf, LshConfig *conf);

void freeHashFamily(HashFamily *hash_family, int num);

float getUniformRandom(float range_start, float range_end);

float getGaussianRandom();

unsigned int getRandomUns32(unsigned int range_start, unsigned int range_end);

unsigned long computeLsh(void *data, void *other);

void freeHashFamily(HashFamily *hash_family, int num);

void freeLshParam(LshParam *lsh_param);


#endif


