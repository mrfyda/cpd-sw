FLAGS   = -ansi -pedantic -Wall -g -lm
CC      = mpicc
CFLAGS  = $(FLAGS) $(INCLUDE)
LIBS    = 
USER    = $(shell cat user.conf)

C_FILES   = $(wildcard *.c)
OBJ_FILES = $(patsubst %.c,obj/%.o,$(C_FILES))

IN      = $(C_FILES)
OUT     = -o bin/wolves-squirrels-serial

all: clean GNU/Linux clean run

local: clean GNU/Linux clean lrun

GNU/Linux:
	@$(CC) $(IN) $(OUT) $(CFLAGS) $(LIBDIR) $< $(LIBS)

clean:
	@mkdir -p bin
	@rm -rf *.a *.o wolves-squirrels-serial

run:
	scp wolves-squirrels-serial.c condor ist1${USER}@cluster.rnl.ist.utl.pt:/mnt/nimbus/pool/CPD/groups/15/${USER}
	ssh ist1${USER}@cluster.rnl.ist.utl.pt 'bash -c "\
		cd /mnt/nimbus/pool/CPD/groups/15/${USER}; \
		$(CC) $(IN) -o wolves-squirrels-serial $(CFLAGS) $(LIBDIR) $< $(LIBS); \
		rm wolves-squirrels-serial.c && \
		condor_submit condor && \
		tail -f outputs/out"'

lrun:
	@mpirun -np 4 ./bin/wolves-squirrels-serial tests/ex3.in 3 4 4 4

whoami:
	@echo ${USER}
