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

typedef struct {
    int x;
    int y;
} position;

void readFile(char *path, world ***board, int *worldSize, int sbp, int wbp, int wsp);
void printBoard(world **board, int worldSize);
void debug(const char *format, ...);
void processSquirrel(world ***board, int worldSize, position pos);
void processWolf(world ***board);
void processConflicts(world ***board);
void processCell(world ***board, int worldSize, position pos);


int main(int argc, char *argv[]) {
    int wolfBreedingPeriod;
    int squirrelBreedingPeriod;
    int wolfStarvationPeriod;
    int numberOfGenerations;
    int worldSize;
    world **board = NULL;
    int g;
    position currentPos;

    if (argc != 6)
        debug("Unexpected number of input: %d\n", argc);

    wolfBreedingPeriod = atoi(argv[2]);
    debug("Wolf breeding period: %d\n", wolfBreedingPeriod);

    squirrelBreedingPeriod = atoi(argv[3]);
    debug("Squirrel breeding period: %d\n", squirrelBreedingPeriod);

    wolfStarvationPeriod = atoi(argv[4]);
    debug("Wolf starvation period: %d\n", wolfStarvationPeriod);

    debug("Reading from file: %s\n", argv[1]);
    readFile(argv[1], &board, &worldSize, squirrelBreedingPeriod, wolfBreedingPeriod, wolfStarvationPeriod);

    numberOfGenerations = atoi(argv[5]);
    debug("Number of generations: %d\n", numberOfGenerations);

    for (g = 0; g < numberOfGenerations; g++) {
        for (currentPos.x = 0; currentPos.x < worldSize; currentPos.x++) {
            for (currentPos.y = currentPos.x % 2; currentPos.y < worldSize; currentPos.y += 2) {
                processCell(&board, worldSize, currentPos);
            }
        }
        for (currentPos.x = 0; currentPos.x < worldSize; currentPos.x++) {
            for (currentPos.y = 1 - (currentPos.y % 2); currentPos.y < worldSize; currentPos.y += 2) {
                processCell(&board, worldSize, currentPos);
            }
        }
    }

    printBoard(board, worldSize);

    for (currentPos.x = 0; currentPos.x < worldSize; currentPos.x++) {
        free(board[currentPos.x]);
    }
    free(board);

    return 0;
}

void readFile(char *path, world ***board, int *worldSize, int sbp, int wbp, int wsp) {
    char line[80];
    FILE *fr = fopen (path, "rt");

    if (fgets(line, 80, fr) != NULL) {
        int i, j;
        sscanf(line, "%d", worldSize);
        debug("World size: %d\n", *worldSize);

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

            switch (symbol) {
            case SQUIRRELONTREE:
            case SQUIRREL:
                (*board)[x][y].breeding_period = sbp;
                (*board)[x][y].starvation_period = 0;
                break;
            case WOLF:
                (*board)[x][y].breeding_period = wbp;
                (*board)[x][y].starvation_period = wsp;
                break;
            default :
                (*board)[x][y].breeding_period = 0;
                (*board)[x][y].starvation_period = 0;
            }
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

void processCell(world ***board, int worldSize, position pos) {
    switch ((*board)[pos.x][pos.y].type) {
    case SQUIRRELONTREE:
    case SQUIRREL:
        processSquirrel(board, worldSize, pos);
        break;
    case WOLF:
        processWolf(board);
        break;
    }
}

void processSquirrel(world ***board, int worldSize, position pos) {
    /* TODO: Do stuff :) */
}

void processWolf(world ***board) {
    /* TODO: Do stuff */
}
