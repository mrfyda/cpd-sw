FLAGS   = -ansi -pedantic -Wall 
CC      = gcc
CFLAGS  = $(FLAGS) $(INCLUDE)
LIBS    = 
CYGLIBS = 

C_FILES = $(wildcard *.c)
OBJ_FILES = $(patsubst %.c,obj/%.o,$(C_FILES))

IN      = $(C_FILES)
OUT     = -o wolves-squirrels-serial

all: clean $(shell uname -o) clean run

GNU/Linux: 
	@$(CC) $(CFLAGS) $(IN) $(OUT) $(LIBDIR) $< $(LIBS)

Cygwin:
	@$(CC) $(FLAGS) $(IN) $(OUT).exe $< $(CYGLIBS)

clean:
	@rm -rf src/*.a src/*.o wolves-squirrels-serial

run:
	./wolves-squirrels-serial
