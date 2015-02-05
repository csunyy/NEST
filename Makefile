CC = gcc
LIB_DIR = ./lib
SRC_DIR = ./src
INC_DIR = ./include
TEST_DIR = ./test

all: test_example libnest.a

libnest.a:
	make -C $(SRC_DIR) $@

test_example:
	make -C $(TEST_DIR)  

clean:
	make -C $(SRC_DIR) $@; \
	make -C $(TEST_DIR) $@; \

distclean: clean
	rm -f $(LIB_DIR)/*
