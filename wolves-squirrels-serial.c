/*
wolves-squirrels-serial.c

**************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3

#define WOLF 100
#define SQUIRREL 101

#define MAX 1000

struct world {
  int type;
  int breeding_period;
  int starvation_period;
} world[MAX][MAX];

int gridSize;
int wolfBreedingPeriod;
int squirrelBreedingPeriod;
int wolfStarvationPeriod;
int numberOfGenerations;

void readFile(char* path);

int main(int argc, char *argv[]) {

    if (argc != 6)
        printf("Unexpected number of input: %d\n", argc);

    printf("Reading from file: %s\n", argv[1]);
    readFile(argv[1]);

    wolfBreedingPeriod = atoi(argv[2]);
    printf("Wolf breeding period: %d\n", wolfBreedingPeriod);

    squirrelBreedingPeriod = atoi(argv[3]);
    printf("Squirrel breeding period: %d\n", squirrelBreedingPeriod);

    wolfStarvationPeriod = atoi(argv[4]);
    printf("Wolf starvation period: %d\n", wolfStarvationPeriod);

    numberOfGenerations = atoi(argv[5]);
    printf("Number of generations: %d\n", numberOfGenerations);


    printf("http://vimeo.com/64611906\n");

    return 0;
}

void readFile(char* path) {
    char line[80];
    FILE *fr = fopen (path, "rt");

    while (fgets(line, 80, fr) != NULL) {
        int x, y;
        char symbol;

        sscanf(line, "%d %d %c", &x, &y, &symbol);
	    printf("Read from file: %d %d %c\n", x, y, symbol);
    }

    fclose(fr);
}
