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

int main(int argc, char *argv[]) {

  printf("http://vimeo.com/64611906\n");

  return 0;
}
