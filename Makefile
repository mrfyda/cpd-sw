FLAGS   = -fopenmp -ansi -pedantic -Wall -g
CC      = gcc
CFLAGS  = $(FLAGS) $(INCLUDE)
LIBS    = 
CYGLIBS = 

C_FILES = $(wildcard *.c)
OBJ_FILES = $(patsubst %.c,obj/%.o,$(C_FILES))

IN      = $(C_FILES)
OUT     = -o bin/wolves-squirrels-serial

all: clean $(shell uname -o) clean run

GNU/Linux: 
	@$(CC) $(CFLAGS) $(IN) $(OUT) $(LIBDIR) $< $(LIBS)

Cygwin:
	@$(CC) $(FLAGS) $(IN) $(OUT).exe $< $(CYGLIBS)

clean:
	@mkdir -p bin
	@rm -rf *.a *.o wolves-squirrels-serial

run:
	./bin/wolves-squirrels-serial tests/ex3.in 3 4 4 4

test:
	./bin/wolves-squirrels-serial tests/t2.in 20 50 50 1000
