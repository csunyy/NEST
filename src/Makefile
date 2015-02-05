CC = gcc
INC_DIR = ../include
LIB_DIR = ../lib
INCL = -I$(INC_DIR)
LIBS = -L$(LIB_DIR) -lnest
OBJ = lsh.o cuckoo.o nest.o

all: $(OBJ) libnest.a

libnest.a: $(OBJ)
	ar rc $(LIB_DIR)/$@ $(OBJ)
	ranlib $(LIB_DIR)/$@

lsh.o: lsh.c $(INC_DIR)/lsh.h
	$(CC) $(INCL) -c lsh.c 

cuckoo.o: cuckoo.c $(INC_DIR)/cuckoo.h $(INC_DIR)/cuckoo.h
	$(CC) $(INCL) -c cuckoo.c

nest.o: nest.c $(INC_DIR)/nest.h $(INC_DIR)/lsh.h $(INC_DIR)/cuckoo.h
	$(CC) $(INCL) -c nest.c

clean:
	rm *.o
 
