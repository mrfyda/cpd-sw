FLAGS   = -ansi -pedantic -Wall -g
CC      =  mpicc
CFLAGS  = $(FLAGS) $(INCLUDE)
LIBS    = 
CYGLIBS = 

C_FILES = $(wildcard *.c)
OBJ_FILES = $(patsubst %.c,obj/%.o,$(C_FILES))

IN      = $(C_FILES)
OUT     = -o bin/wolves-squirrels-mpi

all: clean $(shell uname -o) clean run

lall: clean $(shell uname -o) clean lrun

GNU/Linux:
	@$(CC) $(CFLAGS) $(IN) $(OUT) $(LIBDIR) $< $(LIBS)

Cygwin:
	@$(CC) $(FLAGS) $(IN) $(OUT).exe $< $(CYGLIBS)

clean:
	@mkdir -p bin
	@rm -rf *.a *.o wolves-squirrels-serial

run:
	scp bin/wolves-squirrels-mpi ist169637@cluster.rnl.ist.utl.pt:/mnt/nimbus/pool/CPD/groups/15
	scp condor ist169637@cluster.rnl.ist.utl.pt:/mnt/nimbus/pool/CPD/groups/15
	ssh ist169637@cluster.rnl.ist.utl.pt 'bash -c "condor_submit condor; tail -f outputs/log"'
	# scp -r ist169637@cluster.rnl.ist.utl.pt:/mnt/nimbus/pool/CPD/groups/15/outputs outputs

lrun:
	mpirun -np 4 ./bin/wolves-squirrels-mpi

