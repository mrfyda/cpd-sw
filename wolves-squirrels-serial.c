/*
wolves-squirrels-serial.c

**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define DEBUG 1
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

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

#define MOVES 4

typedef struct {
    int type;
    int breeding_period;
    int starvation_period;
} world;

typedef struct {
    int x;
    int y;
} position;

void readFile(char *path, world ***readBoard, world ***writeBoard, int *worldSize);
void printBoard(world **board, int worldSize);
void debug(const char *format, ...);
void processSquirrel(world ***board, int worldSize, position pos, int sBreedingPeriod);
void processWolf(world ***board, int worldSize, position pos, int wBreedingPeriod, int wStarvationPeriod);
void processConflictSameType(world *currentCell, world *newCell);
void processConflict(world *currentCell, world *newCell);
void processCell(world **readBoard, world ***writeBoard, int worldSize, position pos, int wBreedingPeriod, int sBreedingPeriod, int wStarvationPeriod);


int main(int argc, char *argv[]) {
    int wolfBreedingPeriod;
    int squirrelBreedingPeriod;
    int wolfStarvationPeriod;
    int numberOfGenerations;
    int worldSize;
    world **readBoard = NULL, **writeBoard = NULL;
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
    readFile(argv[1], &readBoard, &writeBoard, &worldSize);

    numberOfGenerations = atoi(argv[5]);
    debug("Number of generations: %d\n", numberOfGenerations);

    for (g = 0; g < numberOfGenerations; g++) {
        printBoard(readBoard, worldSize);

        for (currentPos.x = 0; currentPos.x < worldSize; currentPos.x++) {
            for (currentPos.y = currentPos.x % 2; currentPos.y < worldSize; currentPos.y += 2) {
                processCell(readBoard, &writeBoard, worldSize, currentPos, wolfBreedingPeriod, squirrelBreedingPeriod, wolfStarvationPeriod);
            }
        }

        printBoard(readBoard, worldSize);

        for (currentPos.x = 0; currentPos.x < worldSize; currentPos.x++) {
            for (currentPos.y = 1 - (currentPos.x % 2); currentPos.y < worldSize; currentPos.y += 2) {
                processCell(readBoard, &writeBoard, worldSize, currentPos, wolfBreedingPeriod, squirrelBreedingPeriod, wolfStarvationPeriod);
            }
        }

        readBoard = writeBoard;
        /* clean writeBoard */
    }

    printBoard(readBoard, worldSize);

    for (currentPos.x = 0; currentPos.x < worldSize; currentPos.x++) {
        free(readBoard[currentPos.x]);
        free(writeBoard[currentPos.x]);
    }
    free(readBoard);
    free(writeBoard);

    return 0;
}

void readFile(char *path, world ***readBoard, world ***writeBoard, int *worldSize) {
    char line[80];
    FILE *fr = fopen (path, "rt");

    if (fgets(line, 80, fr) != NULL) {
        int i, j;
        sscanf(line, "%d", worldSize);
        debug("World size: %d\n", *worldSize);

        *readBoard = (world **) malloc(*worldSize * sizeof(world *));
        *writeBoard = (world **) malloc(*worldSize * sizeof(world *));
        for (i = 0; i < *worldSize; i++) {
            (*readBoard)[i] = (world *) malloc(*worldSize * sizeof(world));
            (*writeBoard)[i] = (world *) malloc(*worldSize * sizeof(world));

            for (j = 0; j < *worldSize; j++) {
                (*readBoard)[i][j].type = EMPTY;
                (*readBoard)[i][j].breeding_period = 0;
                (*readBoard)[i][j].starvation_period = 0;
                (*writeBoard)[i][j].type = EMPTY;
                (*writeBoard)[i][j].breeding_period = 0;
                (*writeBoard)[i][j].starvation_period = 0;
            }
        }

        while (fgets(line, 80, fr) != NULL) {
            int x, y;
            char symbol;

            sscanf(line, "%d %d %c", &x, &y, &symbol);
            debug("Read from file: %d %d %c\n", x, y, symbol);

            (*readBoard)[x][y].type = symbol;
            if (symbol != SQUIRREL || symbol != WOLF) {
                (*writeBoard)[x][y].type = symbol;
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

void processCell(world **readBoard, world ***writeBoard, int worldSize, position pos, int wBreedingPeriod, int sBreedingPeriod, int wStarvationPeriod) {
    switch (readBoard[pos.x][pos.y].type) {
    case SQUIRRELONTREE:
    case SQUIRREL:
        debug("Processing Squirrel @[%d, %d]...\n", pos.x, pos.y);
        processSquirrel(&readBoard, worldSize, pos, sBreedingPeriod);
        break;
    case WOLF:
        debug("Processing Wolf @[%d, %d]...\n", pos.x, pos.y);
        processWolf(&readBoard, worldSize, pos, wBreedingPeriod, wStarvationPeriod);
        break;
    }
}

void processConflictSameType(world *currentCell, world *newCell) {
    if (currentCell->starvation_period < newCell->starvation_period) {
        newCell->starvation_period = currentCell->starvation_period;
        newCell->breeding_period = currentCell->breeding_period;
    } else if (currentCell->starvation_period > newCell->starvation_period) {
        /* newCell is already up to date */
    } else {
        newCell->breeding_period = MAX(currentCell->breeding_period, newCell->breeding_period);
    }
}

void processConflict(world *currentCell, world *newCell) {
    newCell->type = WOLF;
    newCell->starvation_period = currentCell->starvation_period - newCell->starvation_period;
    newCell->breeding_period = currentCell->breeding_period;
}

/*This function returns 0 if the animal can't move to the cell. When the animal is a wolf, this function verifies
if the cell has a squirrel (returns 1) or it's empty (returns 2). When the animal is a squirrel, this function return 1
if the squirrel can move to the cell.*/
int canMove(int type, world cell) {
    if (type == SQUIRREL && cell.type != WOLF && cell.type != ICE) return 1;
    else if (type == WOLF && cell.type != TREE && cell.type != ICE && cell.type != SQUIRRELONTREE) {
        if (cell.type == SQUIRREL) return 1;
	else return 2;
    }
    else return 0;
}

/*********************************************Squirrel Rules*********************************************/
/********************************************************************************************************/
void moveSquirrel(world *currentCell, world *newCell, int sBreedingPeriod) {
    if (newCell->type == SQUIRREL || newCell->type == SQUIRRELONTREE) {
        processConflictSameType(currentCell, newCell);
    } else if (newCell->type == TREE) {
        newCell->type = SQUIRRELONTREE;
        newCell->starvation_period = currentCell->starvation_period + 1;
        newCell->breeding_period = currentCell->breeding_period + 1;
    } else {
        newCell->type = SQUIRREL;
        newCell->starvation_period = currentCell->starvation_period + 1;
        newCell->breeding_period = currentCell->breeding_period + 1;
    }


    if (currentCell->breeding_period < sBreedingPeriod) {
        if (currentCell->type == SQUIRRELONTREE) {
            currentCell->type = TREE;
        } else if (currentCell->type == SQUIRREL) {
            currentCell->type = EMPTY;
        }
    }

    currentCell->breeding_period = 0;
    currentCell->starvation_period = 0;
}

int calculateSquirrelMoves(world ***board, int worldSize, position pos, world **movePossibilities) {
    int possibleMoves = 0;

    /* UP */
    if (pos.x - 1 > -1 && canMove(SQUIRREL, (*board)[pos.x - 1][pos.y])) {
        movePossibilities[possibleMoves++] = &(*board)[pos.x - 1][pos.y];
    }

    /* RIGHT */
    if (pos.y + 1 < worldSize && canMove(SQUIRREL, (*board)[pos.x][pos.y + 1])) {
        movePossibilities[possibleMoves++] = &(*board)[pos.x][pos.y + 1];
    }

    /* DOWN */
    if (pos.x + 1 < worldSize && canMove(SQUIRREL, (*board)[pos.x + 1][pos.y])) {
        movePossibilities[possibleMoves++] = &(*board)[pos.x + 1][pos.y];
    }

    /* LEFT */
    if (pos.y - 1 > -1 && canMove(SQUIRREL, (*board)[pos.x][pos.y - 1])) {
        movePossibilities[possibleMoves++] = &(*board)[pos.x][pos.y - 1];
    }

    return possibleMoves;
}

void processSquirrel(world ***board, int worldSize, position pos, int sBreedingPeriod) {
    int possibleMoves;
    world **movePossibilities;
    world *currentCell, *newCell;

    movePossibilities = (world **) malloc(MOVES * sizeof(world *));
    currentCell = &(*board)[pos.x][pos.y];

    possibleMoves = calculateSquirrelMoves(board, worldSize, pos, movePossibilities);

    if (possibleMoves > 1) {
        int c = pos.x * worldSize + pos.y;
        newCell = movePossibilities[c % possibleMoves];
    } else if (possibleMoves == 1) {
        newCell = movePossibilities[0];
    } else {
        free(movePossibilities);
        return;
    }

    moveSquirrel(currentCell, newCell, sBreedingPeriod);

    free(movePossibilities);
}
/*******************************************Squirrel Rules End*******************************************/
/********************************************************************************************************/


/***********************************************Wolf Rules***********************************************/
/********************************************************************************************************/
void moveWolf(world *currentCell, world *newCell, int wBreedingPeriod, int wStarvationPeriod) {
    if (newCell->type == WOLF) {
        processConflictSameType(currentCell, newCell);
    } else if (newCell->type == SQUIRREL) {
        newCell->type = WOLF;
        newCell->starvation_period = 0;
        newCell->breeding_period = currentCell->breeding_period + 1;
    } else if ((currentCell->starvation_period + 1) < wStarvationPeriod) {
        newCell->type = WOLF;
        newCell->starvation_period = currentCell->starvation_period + 1;
        newCell->breeding_period = currentCell->breeding_period + 1;
    }


    if (currentCell->breeding_period < wBreedingPeriod) {
        currentCell->type = EMPTY;
    }

    currentCell->breeding_period = 0;
    currentCell->starvation_period = 0;
}

int calculateWolfMoves(world ***board, int worldSize, position pos, world **movePossibilities) {
    int possibleMoves = 0;
    int squirrelFounded = 0;
    int canMoveRes;

    /* UP */
    if (pos.x - 1 > -1 && (canMoveRes = canMove(WOLF, (*board)[pos.x - 1][pos.y]))) {
        if (canMoveRes == 1) squirrelFounded = 1;
        movePossibilities[possibleMoves++] = &(*board)[pos.x - 1][pos.y];
    }

    /* RIGHT */
    if (pos.y + 1 < worldSize && (canMoveRes = canMove(WOLF, (*board)[pos.x - 1][pos.y]))) {
        if (canMoveRes == 1) {
            if (!squirrelFounded) {
                squirrelFounded = 1;
                possibleMoves = 0;
            }
            movePossibilities[possibleMoves++] = &(*board)[pos.x][pos.y + 1];
        } else if (!squirrelFounded)
            movePossibilities[possibleMoves++] = &(*board)[pos.x][pos.y + 1];
    }

    /* DOWN */
    if (pos.x + 1 < worldSize && (canMoveRes = canMove(WOLF, (*board)[pos.x + 1][pos.y]))) {
        if (canMoveRes == 1) {
            if (!squirrelFounded) {
                squirrelFounded = 1;
                possibleMoves = 0;
            }
            movePossibilities[possibleMoves++] = &(*board)[pos.x + 1][pos.y];
        } else if (!squirrelFounded)
            movePossibilities[possibleMoves++] = &(*board)[pos.x + 1][pos.y];
    }

    /* LEFT */
    if (pos.y - 1 > -1 && (canMoveRes = canMove(WOLF, (*board)[pos.x][pos.y - 1]))) {
        if (canMoveRes == 1) {
            if (!squirrelFounded) {
                squirrelFounded = 1;
                possibleMoves = 0;
            }
            movePossibilities[possibleMoves++] = &(*board)[pos.x][pos.y - 1];
        } else if (!squirrelFounded)
            movePossibilities[possibleMoves++] = &(*board)[pos.x][pos.y - 1];
    }

    return possibleMoves;
}

void processWolf(world ***board, int worldSize, position pos, int wBreedingPeriod, int wStarvationPeriod) {
    int possibleMoves;
    world **movePossibilities;
    world *currentCell, *newCell;

    movePossibilities = (world **) malloc(MOVES * sizeof(world *));
    currentCell = &(*board)[pos.x][pos.y];

    possibleMoves = calculateWolfMoves(board, worldSize, pos, movePossibilities);

    if (possibleMoves > 1) {
        int c = pos.x * worldSize + pos.y;
        newCell = movePossibilities[c % possibleMoves];
    } else if (possibleMoves == 1) {
        newCell = movePossibilities[0];
    } else {
        free(movePossibilities);
        return;
    }

    moveWolf(currentCell, newCell, wBreedingPeriod, wStarvationPeriod);

    free(movePossibilities);
}
/*********************************************Wolf Rules End*********************************************/
/********************************************************************************************************/
