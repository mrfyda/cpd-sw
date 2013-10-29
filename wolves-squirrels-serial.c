/*
    wolves-squirrels-serial.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <omp.h>

/*
    Utils
*/

#define DEBUG 0
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

struct node {
    position data;
    struct node *link;
};

typedef struct {
    struct node *head;
    struct node *list_node;
} stack;

void init(stack *s) {
    s->head = NULL;
    s->list_node = NULL;
}

void push(stack *s, position p) {
    s->list_node = malloc(sizeof(struct node));
    s->list_node->data = p;
    s->list_node->link = s->head;
    s->head = s->list_node;
}

position pop(stack *s) {
    position tmp;
    if (s->head == NULL) {
        position p = { -1, -1 };
        return p;
    }
    tmp = s->head->data;
    s->list_node = s->head;
    s->head = s->head->link;
    free(s->list_node);
    return tmp;
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

void readFile(const char *path, world ***readBoard, world ***writeBoard, int *worldSize);
void debugBoard(world **board, int worldSize);
void debug(const char *format, ...);
void printBoardList(world **board, int worldSize);
void processSquirrel(world **oldBoard, world ***newBoard, int worldSize, position pos, stack *s);
void processWolf(world **oldBoard, world ***newBoard, int worldSize, position pos, stack *s);
void processConflictSameType(world *currentCell, world *newCell);
void processConflict(world *currentCell, world *newCell);
void processCell(world **readBoard, world ***writeBoard, int worldSize, position pos, stack *s);


int wolfBreedingPeriod;
int squirrelBreedingPeriod;
int wolfStarvationPeriod;


int main(int argc, const char *argv[]) {
    int worldSize;
    world **readBoard = NULL, **writeBoard = NULL;
    position pos;
    stack updatedCells;
    int start, end;

    if (argc != 6)
        debug("Unexpected number of input: %d\n", argc);

    readFile(argv[1], &readBoard, &writeBoard, &worldSize);

    omp_set_num_threads(2);
    start = omp_get_wtime();
    #pragma omp parallel private(pos, updatedCells)
    {
        int g;
        int numberOfGenerations;

        wolfBreedingPeriod = atoi(argv[2]);
        squirrelBreedingPeriod = atoi(argv[3]);
        wolfStarvationPeriod = atoi(argv[4]);
        numberOfGenerations = atoi(argv[5]);
        init(&updatedCells);

        debugBoard(readBoard, worldSize);

        start = omp_get_wtime();
        /* process each generation */
        for (g = 0; g < numberOfGenerations; g++) {
            /* process first sub generation */
            int x, y;
            #pragma omp for schedule(static, 1) private(x, y)
            for (x = 0; x < worldSize; x++) {
                for (y = x % 2; y < worldSize; y += 2) {
                    pos.x = x;
                    pos.y = y;
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

            #pragma omp barrier
            #pragma omp single
            {
                debug("\n");
                debug("Iteration %d Red\n", g + 1);
                debugBoard(readBoard, worldSize);
                debug("\n");

            }

            /* process second sub generation */
            #pragma omp for schedule(static, 1) private(x, y)
            for (x = 0; x < worldSize; x++) {
                for (y = 1 - (x % 2); y < worldSize; y += 2) {
                    pos.x = x;
                    pos.y = y;
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

            #pragma omp barrier
            #pragma omp single
            {

                debug("Iteration %d Black\n", g + 1);
                debugBoard(readBoard, worldSize);
            }
            #pragma omp for private(x, y)
            for (x = 0; x < worldSize; x++) {
                for (y = 0; y < worldSize; y++) {
                    pos.x = x;
                    pos.y = y;
                    switch (readBoard[pos.x][pos.y].type) {
                    case SQUIRRELONTREE:
                    case SQUIRREL:
                        readBoard[pos.x][pos.y].breeding_period++;
                        readBoard[pos.x][pos.y].starvation_period++;
                        break;
                    case WOLF:
                        /* Matem-me agora! */
                        if (readBoard[pos.x][pos.y].starvation_period >= wolfStarvationPeriod) {
                            readBoard[pos.x][pos.y].type = EMPTY;
                            readBoard[pos.x][pos.y].breeding_period = 0;
                            readBoard[pos.x][pos.y].starvation_period = 0;
                        } else {
                            readBoard[pos.x][pos.y].breeding_period++;
                            readBoard[pos.x][pos.y].starvation_period++;
                        }
                        break;
                    }
                }
            }
        }
    }
    end = omp_get_wtime();

    printBoardList(readBoard, worldSize);

    for (pos.x = 0; pos.x < worldSize; pos.x++) {
        free(readBoard[pos.x]);
        free(writeBoard[pos.x]);
    }
    free(readBoard);
    free(writeBoard);

    printf("TIME: %d\n", end - start);
    fflush(stdout);

    return 0;
}

void readFile(const char *path, world ***readBoard, world ***writeBoard, int *worldSize) {
    char line[80];
    FILE *fr = fopen (path, "rt");

    if (fgets(line, 80, fr) != NULL) {
        int i, j;
        sscanf(line, "%d", worldSize);

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

            (*readBoard)[x][y].type = symbol;
            (*writeBoard)[x][y].type = symbol;
        }
    }

    fclose(fr);
}

void debugBoard(world **board, int worldSize) {
    if (DEBUG) {
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

void processSquirrel(world **oldBoard, world ***newBoard, int worldSize, position pos, stack *s) {
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
    push(s, pos);
    push(s, destPos);
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

void processWolf(world **oldBoard, world ***newBoard, int worldSize, position pos, stack *s) {
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
    push(s, pos);
    push(s, destPos);
}
/*********************************************Wolf Rules End*********************************************/
/********************************************************************************************************/
