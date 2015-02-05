CC = gcc
INC_DIR = ../include
LIB_DIR = ../lib
INCL = -I$(INC_DIR)
LIBS = -L$(LIB_DIR) -lnest -lm



all: nest_test lsh_test cuckoo_test string_generator vectors_generator

nest_test: nest_test.c $(INC_DIR)/nest.h $(INC_DIR)/lsh.h $(INC_DIR)/cuckoo.h
	$(CC) $(INCL) nest_test.c -o $@ $(LIBS)

lsh_test: lsh_test.c $(INC_DIR)/lsh.h
	$(CC) $(INCL) lsh_test.c -o $@ $(LIBS)

cuckoo_test: cuckoo_test.c $(INC_DIR)/cuckoo.h
	$(CC) $(INCL) cuckoo_test.c -o $@ $(LIBS)

string_generator: string_generator.c
	$(CC) string_generator.c -o $@

vectors_generator: vectors_generator.c $(INC_DIR)/lsh.h
	$(CC) $(INCL) vectors_generator.c -o $@ $(LIBS)

clean:
	rm nest_test lsh_test cuckoo_test string_generator vectors_generator


