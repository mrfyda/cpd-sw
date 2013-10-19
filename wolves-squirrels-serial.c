/*
wolves-squirrels-serial.c

**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define DEBUG 1

#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3

#define EMPTY '.'
#define WOLF 'w'
#define SQUIRREL 's'
#define TREE 't'
#define ICE 'i'
#define SQUIRRELONTREE '$'

typedef struct {
    int type;
    int breeding_period;
    int starvation_period;
} world;


void readFile(char *path, world ***board, int *worldSize);
void printBoard(world **board, int worldSize);
void debug(const char *format, ...);
void processSquirrel(world ***board);
void processWolf(world ***board);
void processConflicts(world ***board);


int main(int argc, char *argv[]) {
    int wolfBreedingPeriod;
    int squirrelBreedingPeriod;
    int wolfStarvationPeriod;
    int numberOfGenerations;
    int worldSize;
    world **board = NULL;

    if (argc != 6)
        debug("Unexpected number of input: %d\n", argc);

    debug("Reading from file: %s\n", argv[1]);
    readFile(argv[1], &board, &worldSize);

    wolfBreedingPeriod = atoi(argv[2]);
    debug("Wolf breeding period: %d\n", wolfBreedingPeriod);

    squirrelBreedingPeriod = atoi(argv[3]);
    debug("Squirrel breeding period: %d\n", squirrelBreedingPeriod);

    wolfStarvationPeriod = atoi(argv[4]);
    debug("Wolf starvation period: %d\n", wolfStarvationPeriod);

    numberOfGenerations = atoi(argv[5]);
    debug("Number of generations: %d\n", numberOfGenerations);

    printBoard(board, worldSize);

    return 0;
}

void readFile(char *path, world ***board, int *worldSize) {
    char line[80];
    FILE *fr = fopen (path, "rt");

    if (fgets(line, 80, fr) != NULL) {
        int i, j;
        sscanf(line, "%d", worldSize);
        debug("Grid size: %d\n", *worldSize);

        *board = (world **) malloc(*worldSize * sizeof(world *));
        for (i = 0; i < *worldSize; i++) {
            (*board)[i] = (world *) malloc(*worldSize * sizeof(world));

            for (j = 0; j < *worldSize; j++) {
                (*board)[i][j].type = EMPTY;
            }
        }

        while (fgets(line, 80, fr) != NULL) {
            int x, y;
            char symbol;

            sscanf(line, "%d %d %c", &x, &y, &symbol);
            debug("Read from file: %d %d %c\n", x, y, symbol);

            (*board)[x][y].type = symbol;
        }
    }

    fclose(fr);
}

void printBoard(world **board, int worldSize) {
    int i, j;

    for (i = 0; i < worldSize; i++) {
        for (j = 0; j < worldSize; j++) {
            debug("%c ", board[i][j].type);
        }
        debug("\n");
    }
}

void debug(const char *format, ...) {
    if (DEBUG) {
        va_list arg;

        va_start (arg, format);
        vfprintf (stdout, format, arg);
        va_end (arg);

        fflush(stdout);
    }
}
