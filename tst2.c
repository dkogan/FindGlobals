#include <stdio.h>
#include <stdlib.h>

#include "tst1.h"


int v2;
int v21 = 5;
const char* c22;

struct S0 s0;
int       x0[0];

const char globalconst;

int tst2func(void)
{
    static int staticf[5];
    static double staticd;
    double dynamicd;

    return 3;
}
