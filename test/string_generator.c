#include <stdio.h>
#include <stdlib.h>
#include <time.h>


void generate_random_string(int num, const char *filename, int max_len, int min_len);
void adjust(int *max, int *min);

void generate_random_string(int num, const char *filename, int max_len, int min_len)
{
	int len = 0;
	char *buf = NULL;
	char dataset[36];
	FILE *fd;
	int i = 0, j = 0;	
	int pos;
	
	for(i = 0; i < 26; i++) {
		dataset[i] = 'a' + i;
	}

	for(; i < 36; i++) {
		dataset[i] = '0' + i - 26;
	}

	if((fd = fopen(filename, "w+")) == NULL) {
		fprintf(stderr, "can't open or create %s.\n", filename);
		return;
	}	

	adjust(&max_len, &min_len);

	if((buf = (char*)malloc(max_len + 1)) == NULL) {
		fclose(fd);
		return ;
	}	

	srand((unsigned)time(NULL));

	for(i = 0; i < num; i++) {		
		if(max_len == min_len) {
			len = max_len; 
		} else {
			len = min_len + rand()%(max_len - min_len + 1);
		}
			
		for(j = 0; j < len; j++) {
			pos = rand()%36;			
			buf[j] = dataset[pos];
		}
		buf[j] = '\0';
		fprintf(fd, "%s\n", buf);
	}	

	free(buf);
	fclose(fd);
} 


void adjust(int *max, int *min)
{
	int tmp;
	if(*max >= *min) { 
		return ;
	}
	else {
		tmp = *max;
		*max = *min;
		*min = tmp;	
	}
}


int main(int args, char *argv[])
{
	int num, max_len, min_len;
	char *filename;
	if(args != 5) {
		fprintf(stdout, "Usage: %s num  filename  max_len  min_len\n", argv[0]);
		fprintf(stdout, "num : number of strings\n");
		fprintf(stdout, "filename : the file stores the strings\n");
		fprintf(stdout, "max_len : maxium length of strings\n");
		fprintf(stdout, "min_len : minium length of strings\n");
		exit(-1);
	}
	
	num = atoi(argv[1]);
	filename = argv[2];
	max_len = atoi(argv[3]);
	min_len = atoi(argv[4]);

	generate_random_string(num, filename, max_len, min_len);

	fprintf(stdout, "OK!\n");
	
	return 0;
}


