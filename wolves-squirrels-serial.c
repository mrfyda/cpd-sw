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
void processSquirrel(world **oldBoard, world ***newBoard, int worldSize, position pos);
void processWolf(world **oldBoard, world ***newBoard, int worldSize, position pos);
void processConflictSameType(world *currentCell, world *newCell);
void processConflict(world *currentCell, world *newCell);
void processCell(world **readBoard, world ***writeBoard, int worldSize, position pos);


int wolfBreedingPeriod;
int squirrelBreedingPeriod;
int wolfStarvationPeriod;


int main(int argc, char *argv[]) {
    int numberOfGenerations;
    int worldSize;
    world **readBoard = NULL, **writeBoard = NULL, **tmp = NULL;
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
                processCell(readBoard, &writeBoard, worldSize, currentPos);
            }
        }

        printBoard(readBoard, worldSize);

        for (currentPos.x = 0; currentPos.x < worldSize; currentPos.x++) {
            for (currentPos.y = 1 - (currentPos.x % 2); currentPos.y < worldSize; currentPos.y += 2) {
                processCell(readBoard, &writeBoard, worldSize, currentPos);
            }
        }

        tmp = readBoard;
        readBoard = writeBoard;
        writeBoard = tmp;
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

void processCell(world **readBoard, world ***writeBoard, int worldSize, position pos) {
    switch (readBoard[pos.x][pos.y].type) {
    case SQUIRRELONTREE:
    case SQUIRREL:
        debug("Processing Squirrel @[%d, %d]...\n", pos.x, pos.y);
        processSquirrel(readBoard, writeBoard, worldSize, pos);
        break;
    case WOLF:
        debug("Processing Wolf @[%d, %d]...\n", pos.x, pos.y);
        processWolf(readBoard, writeBoard, worldSize, pos);
        break;
    }
}

void processConflictSameType(world *oldCell, world *destCell) {
    if (oldCell->starvation_period < destCell->starvation_period) {
        destCell->starvation_period = oldCell->starvation_period;
        destCell->breeding_period = oldCell->breeding_period;
    } else if (oldCell->starvation_period > destCell->starvation_period) {
        /* destCell is already up to date */
    } else {
        destCell->breeding_period = MAX(oldCell->breeding_period, destCell->breeding_period);
    }
}

void processConflict(world *oldCell, world *destCell) {
    destCell->type = WOLF;
    destCell->starvation_period = oldCell->starvation_period - destCell->starvation_period;
    destCell->breeding_period = oldCell->breeding_period;
}

/* This function returns a troolean:
 *     if cantMove -> 0
 *     if canMove  -> 1
 *     if canEat   -> 2
 */
int canMove(int type, world cell) {
    if (type == SQUIRREL && (cell.type == TREE || cell.type == EMPTY)) return 1;
    else if (type == WOLF && (cell.type == SQUIRREL || cell.type == EMPTY)) {
        if (cell.type == SQUIRREL) return 2;
        else return 1;
    } else return 0;
}

/*********************************************Squirrel Rules*********************************************/
/********************************************************************************************************/
void moveSquirrel(world *oldCell, world *newCell, world *destCell) {
    if (destCell->type == SQUIRREL || destCell->type == SQUIRRELONTREE) {
        processConflictSameType(oldCell, destCell);
    } else if (destCell->type == TREE) {
        destCell->type = SQUIRRELONTREE;
        destCell->starvation_period = oldCell->starvation_period + 1;
        destCell->breeding_period = oldCell->breeding_period + 1;
    } else {
        destCell->type = SQUIRREL;
        destCell->starvation_period = oldCell->starvation_period + 1;
        destCell->breeding_period = oldCell->breeding_period + 1;
    }

    if (!oldCell->breeding_period < squirrelBreedingPeriod) {
        if (oldCell->type == SQUIRRELONTREE) {
            newCell->type = SQUIRRELONTREE;
        } else if (oldCell->type == SQUIRREL) {
            newCell->type = SQUIRREL;
        }
        newCell->breeding_period = 0;
        newCell->starvation_period = 0;

        destCell->breeding_period = 0;
    }
}

int calculateSquirrelMoves(world ***oldBoard, world ***newBoard, int worldSize, position pos, world **movePossibilities) {
    int possibleMoves = 0;

    /* UP */
    if (pos.x - 1 > -1 && canMove(SQUIRREL, (*oldBoard)[pos.x - 1][pos.y])) {
        movePossibilities[possibleMoves++] = &(*newBoard)[pos.x - 1][pos.y];
    }

    /* RIGHT */
    if (pos.y + 1 < worldSize && canMove(SQUIRREL, (*oldBoard)[pos.x][pos.y + 1])) {
        movePossibilities[possibleMoves++] = &(*newBoard)[pos.x][pos.y + 1];
    }

    /* DOWN */
    if (pos.x + 1 < worldSize && canMove(SQUIRREL, (*oldBoard)[pos.x + 1][pos.y])) {
        movePossibilities[possibleMoves++] = &(*newBoard)[pos.x + 1][pos.y];
    }

    /* LEFT */
    if (pos.y - 1 > -1 && canMove(SQUIRREL, (*oldBoard)[pos.x][pos.y - 1])) {
        movePossibilities[possibleMoves++] = &(*newBoard)[pos.x][pos.y - 1];
    }

    return possibleMoves;
}

void processSquirrel(world **oldBoard, world ***newBoard, int worldSize, position pos) {
    int possibleMoves;
    world **movePossibilities;
    world *oldCell, *newCell, *destCell;

    movePossibilities = (world **) malloc(MOVES * sizeof(world *));
    oldCell = &oldBoard[pos.x][pos.y];
    newCell = &(*newBoard)[pos.x][pos.y];

    possibleMoves = calculateSquirrelMoves(&oldBoard, newBoard, worldSize, pos, movePossibilities);

    if (possibleMoves > 1) {
        int c = pos.x * worldSize + pos.y;
        destCell = movePossibilities[c % possibleMoves];
    } else if (possibleMoves == 1) {
        destCell = movePossibilities[0];
    } else {
        free(movePossibilities);
        return;
    }

    moveSquirrel(oldCell, newCell, destCell);

    free(movePossibilities);
}
/*******************************************Squirrel Rules End*******************************************/
/********************************************************************************************************/


/***********************************************Wolf Rules***********************************************/
/********************************************************************************************************/
void moveWolf(world *oldCell, world *newCell, world *destCell) {
    if (destCell->type == WOLF) {
        processConflictSameType(oldCell, destCell);
    } else if (destCell->type == SQUIRREL) {
        destCell->type = WOLF;
        destCell->starvation_period = 0;
        destCell->breeding_period = oldCell->breeding_period + 1;
    } else if ((oldCell->starvation_period + 1) < wolfStarvationPeriod) {
        destCell->type = WOLF;
        destCell->starvation_period = oldCell->starvation_period + 1;
        destCell->breeding_period = oldCell->breeding_period + 1;
    }

    if (!oldCell->breeding_period < wolfBreedingPeriod) {
        newCell->type = WOLF;
        newCell->breeding_period = 0;
        newCell->starvation_period = 0;

        destCell->breeding_period = 0;
    }
}

int calculateWolfMoves(world ***oldBoard, world ***newBoard, int worldSize, position pos, world **movePossibilities) {
    int possibleMoves = 0;
    int squirrelFounded = 0;
    int canMoveRes;

    /* UP */
    if (pos.x - 1 > -1 && (canMoveRes = canMove(WOLF, (*oldBoard)[pos.x - 1][pos.y]))) {
        if (canMoveRes == 2) squirrelFounded = 1;
        movePossibilities[possibleMoves++] = &(*newBoard)[pos.x - 1][pos.y];
    }

    /* RIGHT */
    if (pos.y + 1 < worldSize && (canMoveRes = canMove(WOLF, (*oldBoard)[pos.x - 1][pos.y]))) {
        if (canMoveRes == 2) {
            if (!squirrelFounded) {
                squirrelFounded = 1;
                possibleMoves = 0;
            }
            movePossibilities[possibleMoves++] = &(*newBoard)[pos.x][pos.y + 1];
        } else if (!squirrelFounded)
            movePossibilities[possibleMoves++] = &(*newBoard)[pos.x][pos.y + 1];
    }

    /* DOWN */
    if (pos.x + 1 < worldSize && (canMoveRes = canMove(WOLF, (*oldBoard)[pos.x + 1][pos.y]))) {
        if (canMoveRes == 2) {
            if (!squirrelFounded) {
                squirrelFounded = 1;
                possibleMoves = 0;
            }
            movePossibilities[possibleMoves++] = &(*newBoard)[pos.x + 1][pos.y];
        } else if (!squirrelFounded)
            movePossibilities[possibleMoves++] = &(*newBoard)[pos.x + 1][pos.y];
    }

    /* LEFT */
    if (pos.y - 1 > -1 && (canMoveRes = canMove(WOLF, (*oldBoard)[pos.x][pos.y - 1]))) {
        if (canMoveRes == 2) {
            if (!squirrelFounded) {
                squirrelFounded = 1;
                possibleMoves = 0;
            }
            movePossibilities[possibleMoves++] = &(*newBoard)[pos.x][pos.y - 1];
        } else if (!squirrelFounded)
            movePossibilities[possibleMoves++] = &(*newBoard)[pos.x][pos.y - 1];
    }

    return possibleMoves;
}

void processWolf(world **oldBoard, world ***newBoard, int worldSize, position pos) {
    int possibleMoves;
    world **movePossibilities;
    world *oldCell, *newCell, *destCell;

    movePossibilities = (world **) malloc(MOVES * sizeof(world *));
    oldCell = &oldBoard[pos.x][pos.y];
    newCell = &(*newBoard)[pos.x][pos.y];

    possibleMoves = calculateWolfMoves(&oldBoard, newBoard, worldSize, pos, movePossibilities);

    if (possibleMoves > 1) {
        int c = pos.x * worldSize + pos.y;
        destCell = movePossibilities[c % possibleMoves];
    } else if (possibleMoves == 1) {
        destCell = movePossibilities[0];
    } else {
        free(movePossibilities);
        return;
    }

    moveWolf(oldCell, newCell, destCell);

    free(movePossibilities);
}
/*********************************************Wolf Rules End*********************************************/
/********************************************************************************************************/
