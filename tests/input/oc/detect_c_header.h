// Test file for C header that should NOT be detected as Objective-C
// This file has no OC keywords and should remain as C

#include <stdio.h>
#include <stdlib.h>

#define MAX_SIZE 100

typedef struct {
   int x;
   int y;
} Point;

int add(int a, int b);
void print_point(const Point *p);
