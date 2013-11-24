/*
    wolves-squirrels-mpi.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <mpi.h>
#include <math.h>

/*
    Utils
*/

#define DEBUG 1
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
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
    Wolves & Squirrels
*/

#define EMPTY ' '
#define WOLF 'w'
#define SQUIRREL 's'
#define TREE 't'
#define ICE 'i'
#define SQUIRRELONTREE '$'

#define MOVES 4
#define PADDING 2

typedef struct {
    int type;
    int breeding_period;
    int starvation_period;
} world;

void readFile(const char *path, world ***readBoard, world ***writeBoard, int *worldSize, int id, int p, int *partitionSize);
void debugBoard(world **board, int partitionSize, int worldSize);
void debug(const char *format, ...);
void printBoardList(world **board, int worldSize);
void processSquirrel(world **oldBoard, world ***newBoard, int worldSize, position pos);
void processWolf(world **oldBoard, world ***newBoard, int worldSize, position pos);
void processConflictSameType(world *currentCell, world *newCell);
void processConflict(world *currentCell, world *newCell);
void processCell(world **readBoard, world ***writeBoard, int worldSize, position pos);


int wolfBreedingPeriod;
int squirrelBreedingPeriod;
int wolfStarvationPeriod;


int main(int argc, char *argv[]) {
    int worldSize;
    int partitionSize;
    world **readBoard = NULL, **writeBoard = NULL;
    int g;
    position pos;
    int numberOfGenerations;
    int id, p;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    debug("Process: %d of %d\n", id, p);

    if (argc != 6)
        debug("Unexpected number of input: %d\n", argc);

    readFile(argv[1], &readBoard, &writeBoard, &worldSize, id , p, &partitionSize);

    wolfBreedingPeriod = atoi(argv[2]);
    squirrelBreedingPeriod = atoi(argv[3]);
    wolfStarvationPeriod = atoi(argv[4]);
    numberOfGenerations = atoi(argv[5]);

    /*
    debugBoard(readBoard, worldSize);
    */

    /* process each generation */
    for (g = 0; g < numberOfGenerations; g++) {
        /* process first sub generation */
        int x, y;
        for (pos.x = 0; pos.x < partitionSize; pos.x++) {
            for (pos.y = pos.x % 2; pos.y < worldSize; pos.y += 2) {
                processCell(readBoard, &writeBoard, worldSize, pos);
            }
        }

        /* copy updated cells to readBoard */
        for (x = 0; x < partitionSize; x++) {
            for (y = 0; y < worldSize; y++) {
                readBoard[x][y] = writeBoard[x][y];
            }
        }

        /*
        debug("\n");
        debug("Iteration %d Red\n", g + 1);
        debugBoard(readBoard, worldSize);
        debug("\n");
        */

        /* process second sub generation */
        for (pos.x = 0; pos.x < partitionSize; pos.x++) {
            for (pos.y = 1 - (pos.x % 2); pos.y < worldSize; pos.y += 2) {
                processCell(readBoard, &writeBoard, worldSize, pos);
            }
        }

        for (pos.x = 0; pos.x < partitionSize; pos.x++) {
            for (pos.y = 0; pos.y < worldSize; pos.y++) {
                switch (writeBoard[pos.x][pos.y].type) {
                case SQUIRRELONTREE:
                case SQUIRREL:
                    writeBoard[pos.x][pos.y].breeding_period++;
                    break;
                case WOLF:
                    /* Matem-me agora! */
                    if (writeBoard[pos.x][pos.y].starvation_period >= wolfStarvationPeriod) {
                        writeBoard[pos.x][pos.y].type = EMPTY;
                        writeBoard[pos.x][pos.y].breeding_period = 0;
                        writeBoard[pos.x][pos.y].starvation_period = 0;
                    } else {
                        writeBoard[pos.x][pos.y].breeding_period++;
                        writeBoard[pos.x][pos.y].starvation_period++;
                    }
                    break;
                }
            }
        }

        /* copy updated cells to readBoard */
        for (x = 0; x < partitionSize; x++) {
            for (y = 0; y < worldSize; y++) {
                readBoard[x][y] = writeBoard[x][y];
            }
        }

        /*
        debug("Iteration %d Black\n", g + 1);
        debugBoard(readBoard, worldSize);
        */
    }

    printBoardList(readBoard, worldSize);

    free(*readBoard);
    free(*writeBoard);
    free(readBoard);
    free(writeBoard);

    MPI_Finalize();

    return 0;
}

void readFile(const char *path, world ***readBoard, world ***writeBoard, int *worldSize, int id , int p, int *partitionSize) {
    char line[80];
    FILE *fr = fopen (path, "rt");

    if (fgets(line, 80, fr) != NULL) {
        int i, j;
        int *requiredLines;
        int startX = 0;
        int sum = 0;
        int firstSize = 0;
        world *readSegment = NULL, *writeSegment = NULL;
        sscanf(line, "%d", worldSize);

        requiredLines = (int *) malloc(p * sizeof(int));
        for (i = 0; i < p; i++) {
            float division = (float)(*worldSize) / (float)p;

            if (i == p - 1) {
                requiredLines[i] = *worldSize - sum;
            } else if (division - abs(division) > 0.5) {
                requiredLines[i] = ceil(division);
            } else {
                requiredLines[i] = floor(division);
            }

            sum += requiredLines[i];
            if (i < id) {
                startX += requiredLines[i];
            }

            if (i == 0) {
                firstSize = MIN(requiredLines[0], PADDING);
            }

            if (i == 0) {
                requiredLines[0] += PADDING;
            } else if (i == 1) {
                requiredLines[1] += PADDING + firstSize;
            } else if (i == p - 1) {
                requiredLines[i] += PADDING;
            } else {
                requiredLines[i] += PADDING + firstSize;
            }
        }

        if (id == 1) {
            startX -= firstSize;
        } else if (id != 0) {
            startX -= PADDING;
        }

        readSegment = (world *) malloc(requiredLines[id] * (*worldSize * sizeof(world)));
        writeSegment = (world *) malloc(requiredLines[id] * (*worldSize * sizeof(world)));

        *readBoard = (world **) malloc(requiredLines[id] * sizeof(world *));
        *writeBoard = (world **) malloc(requiredLines[id] * sizeof(world *));

        for (i = 0; i < requiredLines[id]; i++) {
            (*readBoard)[i] = &readSegment[*worldSize * i];
            (*writeBoard)[i] = &writeSegment[*worldSize * i];

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

            if (x >= startX && x < startX + requiredLines[id]) {
                (*readBoard)[x - startX][y].type = symbol;
                (*writeBoard)[x - startX][y].type = symbol;
            }
        }

        *partitionSize = requiredLines[id];
        free(requiredLines);
    }

    fclose(fr);
}

void debugBoard(world **board, int partitionSize, int worldSize) {
    if (DEBUG) {
        int i, j;
        debug("---------------------------------\n   ");
        for (i = 0; i < worldSize; i++) {
            debug("%02d|", i);
        }
        debug("\n");
        for (i = 0; i < partitionSize; i++) {
            debug("%02d: ", i);
            for (j = 0; j < worldSize; j++) {
                debug("%1c| ", board[i][j].type);
            }
            debug("\n");
        }
        debug("---------------------------------\n");
    }
}

void printBoardList(world **board, int worldSize) {
    int i, j;
    for (i = 0; i < worldSize; i++) {
        for (j = 0; j < worldSize; j++) {
            if (board[i][j].type != EMPTY) {
                printf("%d %d %c\n", i, j, board[i][j].type);
                fflush(stdout);
            }
        }
    }
}

void processCell(world **readBoard, world ***writeBoard, int worldSize, position pos) {
    switch (readBoard[pos.x][pos.y].type) {
    case SQUIRRELONTREE:
    case SQUIRREL:
        processSquirrel(readBoard, writeBoard, worldSize, pos);
        break;
    case WOLF:
        processWolf(readBoard, writeBoard, worldSize, pos);
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
    destCell->starvation_period = 0;

    if (oldCell->type == WOLF) {
        destCell->breeding_period = oldCell->breeding_period;
    }
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
    } else if (destCell->type == WOLF) {
        processConflict(oldCell, destCell);
    } else if (destCell->type == TREE) {
        destCell->type = SQUIRRELONTREE;
        destCell->starvation_period = oldCell->starvation_period;
        destCell->breeding_period = oldCell->breeding_period;
    } else {
        destCell->type = SQUIRREL;
        destCell->starvation_period = oldCell->starvation_period;
        destCell->breeding_period = oldCell->breeding_period;
    }

    if (oldCell->breeding_period >= squirrelBreedingPeriod) {
        if (oldCell->type == SQUIRRELONTREE) {
            newCell->type = SQUIRRELONTREE;
        } else if (oldCell->type == SQUIRREL) {
            newCell->type = SQUIRREL;
        }

        destCell->breeding_period = -1;
        newCell->breeding_period = -1;
    } else {
        if (oldCell->type == SQUIRRELONTREE) {
            newCell->type = TREE;
        } else if (oldCell->type == SQUIRREL) {
            newCell->type = EMPTY;
        }

        newCell->breeding_period = 0;
    }

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

void processSquirrel(world **oldBoard, world ***newBoard, int worldSize, position pos) {
    int possibleMoves;
    position destPos, possiblePos[MOVES];
    world *oldCell, *newCell, *destCell;

    oldCell = &oldBoard[pos.x][pos.y];
    newCell = &(*newBoard)[pos.x][pos.y];

    possibleMoves = calculateSquirrelMoves(oldBoard, worldSize, pos, possiblePos);

    if (possibleMoves > 1) {
        int c = pos.x * worldSize + pos.y;
        destPos = possiblePos[c % possibleMoves];
        destCell = &(*newBoard)[destPos.x][destPos.y];
    } else if (possibleMoves == 1) {
        destPos = possiblePos[0];
        destCell = &(*newBoard)[destPos.x][destPos.y];
    } else {
        return;
    }

    moveSquirrel(oldCell, newCell, destCell);
}
/*******************************************Squirrel Rules End*******************************************/
/********************************************************************************************************/


/***********************************************Wolf Rules***********************************************/
/********************************************************************************************************/

int canWolfMove(world cell) {
    switch (cell.type) {
    case SQUIRREL:
        return 2;
    case EMPTY:
        return 1;
    default:
        return 0;
    }
}

void moveWolf(world *oldCell, world *newCell, world *destCell) {
    if (destCell->type == WOLF) {
        processConflictSameType(oldCell, destCell);
    } else if (destCell->type == SQUIRREL) {
        processConflict(oldCell, destCell);
    } else {
        destCell->type = WOLF;
        destCell->starvation_period = oldCell->starvation_period;
        destCell->breeding_period = oldCell->breeding_period;
    }

    if (oldCell->breeding_period >= wolfBreedingPeriod) {
        newCell->type = WOLF;

        destCell->breeding_period = -1;
        newCell->breeding_period = -1;
    } else {
        newCell->type = EMPTY;

        newCell->breeding_period = 0;
    }

    newCell->starvation_period = 0;
}

int calculateWolfMoves(world **oldBoard, int worldSize, position pos, position *possiblePos) {
    int possibleMoves = 0;
    int squirrelFound = 0;
    int canMoveRes;

    /* UP */
    if (pos.x - 1 > -1 && (canMoveRes = canWolfMove(oldBoard[pos.x - 1][pos.y]))) {
        position p = pos;
        if (canMoveRes == 2) squirrelFound = 1;
        p.x -= 1;
        possiblePos[possibleMoves++] = p;
    }

    /* RIGHT */
    if (pos.y + 1 < worldSize && (canMoveRes = canWolfMove(oldBoard[pos.x][pos.y + 1]))) {
        position p = pos;
        if (!squirrelFound) {
            if (canMoveRes == 2) {
                squirrelFound = 1;
                possibleMoves = 0;
            }
            p.y += 1;
            possiblePos[possibleMoves++] = p;
        }
    }

    /* DOWN */
    if (pos.x + 1 < worldSize && (canMoveRes = canWolfMove(oldBoard[pos.x + 1][pos.y]))) {
        position p = pos;
        if (!squirrelFound) {
            if (canMoveRes == 2) {
                squirrelFound = 1;
                possibleMoves = 0;
            }
            p.x += 1;
            possiblePos[possibleMoves++] = p;
        }
    }

    /* LEFT */
    if (pos.y - 1 > -1 && (canMoveRes = canWolfMove(oldBoard[pos.x][pos.y - 1]))) {
        position p = pos;
        if (!squirrelFound) {
            if (canMoveRes == 2) possibleMoves = 0;
            p.y -= 1;
            possiblePos[possibleMoves++] = p;
        }
    }

    return possibleMoves;
}

void processWolf(world **oldBoard, world ***newBoard, int worldSize, position pos) {
    int possibleMoves;
    position destPos, possiblePos[MOVES];
    world *oldCell, *newCell, *destCell;

    oldCell = &oldBoard[pos.x][pos.y];
    newCell = &(*newBoard)[pos.x][pos.y];

    possibleMoves = calculateWolfMoves(oldBoard, worldSize, pos, possiblePos);

    if (possibleMoves > 1) {
        int c = pos.x * worldSize + pos.y;
        destPos = possiblePos[c % possibleMoves];
        destCell = &(*newBoard)[destPos.x][destPos.y];
    } else if (possibleMoves == 1) {
        destPos = possiblePos[0];
        destCell = &(*newBoard)[destPos.x][destPos.y];
    } else {
        return;
    }

    moveWolf(oldCell, newCell, destCell);
}
/*********************************************Wolf Rules End*********************************************/
/********************************************************************************************************/

