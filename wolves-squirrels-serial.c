/*
    wolves-squirrels-serial.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/*
    Utils
*/

#define DEBUG 1
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef struct {
    int x;
    int y;
} position;

void debug(const char *format, ...) {
    if (DEBUG) {
        va_list arg;

        va_start (arg, format);
        vfprintf (stdout, format, arg);
        va_end (arg);

        fflush(stdout);
    }
}

/*
    Stack
*/

typedef struct {
    position *data;
    int size;
    int maxSize;
} stack;

void init(stack *s, int maxSize) {
    s->data = (position *) malloc(maxSize * sizeof(position));
    s->size = 0;
    s->maxSize = maxSize;
}

void push(stack *s, position p) {
    if (s->size < s->maxSize) {
        s->data[s->size++] = p;
    } else {
        debug("stack is full\n");
    }
}

position pop(stack *s) {
    if (s->size == 0) {
        position p = { -1, -1 };
        return p;
    }

    return s->data[--s->size];
}

void destroy(stack *s) {
    free(s->data);
}

/*
    Wolves & Squirrels
*/

#define EMPTY ' '
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

void readFile(char *path, world ***readBoard, world ***writeBoard, int *worldSize);
void debugBoard(world **board, int worldSize);
void debug(const char *format, ...);
void processSquirrel(world **oldBoard, world ***newBoard, int worldSize, position pos, stack *s);
void processWolf(world **oldBoard, world ***newBoard, int worldSize, position pos, stack *s);
void processConflictSameType(world *currentCell, world *newCell);
void processConflict(world *currentCell, world *newCell);
void processCell(world **readBoard, world ***writeBoard, int worldSize, position pos, stack *s);


int wolfBreedingPeriod;
int squirrelBreedingPeriod;
int wolfStarvationPeriod;


int main(int argc, char *argv[]) {
    int numberOfGenerations;
    int worldSize;
    world **readBoard = NULL, **writeBoard = NULL, **tmp = NULL;
    int g;
    position pos;
    stack updatedCells;

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

    init(&updatedCells, worldSize * worldSize);

    debugBoard(readBoard, worldSize);

    /* process each generation */
    for (g = 0; g < numberOfGenerations; g++) {
        /* process first sub generation */
        for (pos.x = 0; pos.x < worldSize; pos.x++) {
            for (pos.y = pos.x % 2; pos.y < worldSize; pos.y += 2) {
                processCell(readBoard, &writeBoard, worldSize, pos, &updatedCells);
            }
        }

        /* copy updated cells to readBoard */
        while (1) {
            pos = pop(&updatedCells);
            if  (pos.x == -1) {
                break;
            } else {
                readBoard[pos.x][pos.y] = writeBoard[pos.x][pos.y];
            }
        }
        debug("Iteration %d Red\n", g + 1);
        debugBoard(readBoard, worldSize);

        /* process second sub generation */
        for (pos.x = 0; pos.x < worldSize; pos.x++) {
            for (pos.y = 1 - (pos.x % 2); pos.y < worldSize; pos.y += 2) {
                processCell(readBoard, &writeBoard, worldSize, pos, &updatedCells);
            }
        }

        /* copy updated cells to readBoard */
        while (1) {
            pos = pop(&updatedCells);
            if  (pos.x == -1) {
                break;
            } else {
                readBoard[pos.x][pos.y] = writeBoard[pos.x][pos.y];
            }
        }

        debug("Iteration %d Black\n", g + 1);
        debugBoard(readBoard, worldSize);
    }

    /*printBoardList(readBoard, worldSize);*/

    for (pos.x = 0; pos.x < worldSize; pos.x++) {
        free(readBoard[pos.x]);
        free(writeBoard[pos.x]);
    }
    free(readBoard);
    free(writeBoard);

    destroy(&updatedCells);

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
            (*writeBoard)[x][y].type = symbol;
        }
    }

    fclose(fr);
}

void debugBoard(world **board, int worldSize) {
    int i, j;
    debug("---------------------------------\n   ");
    for (i = 0; i < worldSize; i++) {
        debug("%02d|", i);
    }
    debug("\n");
    for (i = 0; i < worldSize; i++) {
        debug("%02d: ", i);
        for (j = 0; j < worldSize; j++) {
            debug("%1c| ", board[i][j].type);
        }
        debug("\n");
    }
    debug("---------------------------------\n\n");

}

void processCell(world **readBoard, world ***writeBoard, int worldSize, position pos, stack *s) {
    switch (readBoard[pos.x][pos.y].type) {
    case SQUIRRELONTREE:
    case SQUIRREL:
        processSquirrel(readBoard, writeBoard, worldSize, pos, s);
        break;
    case WOLF:
        processWolf(readBoard, writeBoard, worldSize, pos, s);
        break;
    }
}

/*
    Conflicts
*/

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
    destCell->starvation_period = - (destCell->starvation_period + wolfStarvationPeriod);
    destCell->breeding_period = oldCell->breeding_period;
}

/*
    Squirrel Rules
*/

int canSquirrelMove(world cell) {
    return cell.type == TREE || cell.type == EMPTY;
}

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

    if (oldCell->breeding_period >= squirrelBreedingPeriod) {
        if (oldCell->type == SQUIRRELONTREE) {
            newCell->type = SQUIRRELONTREE;
        } else if (oldCell->type == SQUIRREL) {
            newCell->type = SQUIRREL;
        }

        destCell->breeding_period = 0;
    } else {
         if (oldCell->type == SQUIRRELONTREE) {
            newCell->type = TREE;
        } else if (oldCell->type == SQUIRREL) {
            newCell->type = EMPTY;
        }
    }

    newCell->breeding_period = 0;
    newCell->starvation_period = 0;
}

int calculateSquirrelMoves(world **oldBoard, int worldSize, position pos, position *possiblePos) {
    int possibleMoves = 0;

    /* UP */
    if (pos.x - 1 > -1 && canSquirrelMove(oldBoard[pos.x - 1][pos.y])) {
        position p = pos;
        p.x -= 1;
        possiblePos[possibleMoves++] = p;
    }

    /* RIGHT */
    if (pos.y + 1 < worldSize && canSquirrelMove(oldBoard[pos.x][pos.y + 1])) {
        position p = pos;
        p.y += 1;
        possiblePos[possibleMoves++] = p;
    }

    /* DOWN */
    if (pos.x + 1 < worldSize && canSquirrelMove(oldBoard[pos.x + 1][pos.y])) {
        position p = pos;
        p.x += 1;
        possiblePos[possibleMoves++] = p;
    }

    /* LEFT */
    if (pos.y - 1 > -1 && canSquirrelMove(oldBoard[pos.x][pos.y - 1])) {
        position p = pos;
        p.y -= 1;
        possiblePos[possibleMoves++] = p;
    }

    return possibleMoves;
}

void processSquirrel(world **oldBoard, world ***newBoard, int worldSize, position pos, stack *s) {
    int possibleMoves;
    position destPos, possiblePos[MOVES];
    world *oldCell, *newCell, *destCell;

    oldCell = &oldBoard[pos.x][pos.y];
    newCell = &(*newBoard)[pos.x][pos.y];

    possibleMoves = calculateSquirrelMoves(oldBoard, worldSize, pos, possiblePos);

    push(s, pos);

    if (possibleMoves > 1) {
        int c = pos.x * worldSize + pos.y;
        destPos = possiblePos[c % possibleMoves];
        destCell = &(*newBoard)[destPos.x][destPos.y];
    } else if (possibleMoves == 1) {
        destPos = possiblePos[0];
        destCell = &(*newBoard)[destPos.x][destPos.y];
    } else {
        newCell->starvation_period++;
        return;
    }

    moveSquirrel(oldCell, newCell, destCell);
    push(s, destPos);
}
/*******************************************Squirrel Rules End*******************************************/
/********************************************************************************************************/


/***********************************************Wolf Rules***********************************************/
/********************************************************************************************************/

int canMove(world cell) {
    switch (cell.type) {
    case SQUIRREL:
        return 2;
    case EMPTY:
        return 1;;
    default:
        return 0;
    }
}

void moveWolf(world *oldCell, world *newCell, world *destCell) {
    if (destCell->type == WOLF) {
        processConflictSameType(oldCell, destCell);
    } else if (destCell->type == SQUIRREL) {
        processConflict(oldCell, destCell);
    } else if ((oldCell->starvation_period + 1) < wolfStarvationPeriod) {
        destCell->type = WOLF;
        destCell->starvation_period = oldCell->starvation_period + 1;
        destCell->breeding_period = oldCell->breeding_period + 1;
    }

    if (oldCell->breeding_period >= wolfBreedingPeriod) {
        newCell->type = WOLF;

        destCell->breeding_period = 0;
    } else {
        newCell->type = EMPTY;
    }

    newCell->breeding_period = 0;
    newCell->starvation_period = 0;
}

int calculateWolfMoves(world **oldBoard, world ***newBoard, int worldSize, position pos, position *possiblePos) {
    int possibleMoves = 0;
    int squirrelFound = 0;
    int canMoveRes;

    /* UP */
    if (pos.x - 1 > -1 && (canMoveRes = canMove(oldBoard[pos.x - 1][pos.y]))) {
        position p = pos;
        if (canMoveRes == 2) squirrelFound = 1;
        p.x -= 1;
        possiblePos[possibleMoves++] = p;
    }

    /* RIGHT */
    if (pos.y + 1 < worldSize && (canMoveRes = canMove(oldBoard[pos.x][pos.y + 1]))) {
        position p = pos;
        if (canMoveRes == 2) {
            if (!squirrelFound) {
                squirrelFound = 1;
                possibleMoves = 0;
            }
            p.y += 1;
            possiblePos[possibleMoves++] = p;
        } else if (!squirrelFound) {
            p.y += 1;
            possiblePos[possibleMoves++] = p;
        }
    }

    /* DOWN */
    if (pos.x + 1 < worldSize && (canMoveRes = canMove(oldBoard[pos.x + 1][pos.y]))) {
        position p = pos;
        if (canMoveRes == 2) {
            if (!squirrelFound) {
                squirrelFound = 1;
                possibleMoves = 0;
            }
            p.x += 1;
            possiblePos[possibleMoves++] = p;
        } else if (!squirrelFound) {
            p.x += 1;
            possiblePos[possibleMoves++] = p;
        }
    }

    /* LEFT */
    if (pos.y - 1 > -1 && (canMoveRes = canMove(oldBoard[pos.x][pos.y - 1]))) {
        position p = pos;
        if (canMoveRes == 2) {
            if (!squirrelFound) {
                squirrelFound = 1;
                possibleMoves = 0;
            }
            p.y -= 1;
            possiblePos[possibleMoves++] = p;
        } else if (!squirrelFound) {
            p.y -= 1;
            possiblePos[possibleMoves++] = p;
        }
    }

    return possibleMoves;
}

void processWolf(world **oldBoard, world ***newBoard, int worldSize, position pos, stack *s) {
    int possibleMoves;
    position destPos, possiblePos[MOVES];
    world *oldCell, *newCell, *destCell;

    oldCell = &oldBoard[pos.x][pos.y];
    newCell = &(*newBoard)[pos.x][pos.y];

    possibleMoves = calculateWolfMoves(oldBoard, newBoard, worldSize, pos, possiblePos);

    push(s, pos);

    if (possibleMoves > 1) {
        int c = pos.x * worldSize + pos.y;
        destPos = possiblePos[c % possibleMoves];
        destCell = &(*newBoard)[destPos.x][destPos.y];
    } else if (possibleMoves == 1) {
        destPos = possiblePos[0];
        destCell = &(*newBoard)[destPos.x][destPos.y];
    } else {
        newCell->starvation_period++;
        return;
    }

    moveWolf(oldCell, newCell, destCell);
    push(s, destPos);
}
/*********************************************Wolf Rules End*********************************************/
/********************************************************************************************************/
