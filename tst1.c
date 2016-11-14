#include <stdio.h>
#include <stdlib.h>

#include "tst1.h"
#include "getglobals.h"

struct S v1[10];

volatile const char vcchar;
const volatile char cvchar;




extern int         v2;
extern int         v21;
extern const char* c22;

extern struct S0   s0;
extern int         x0[0];
extern const char globalconst;



#define printsize(n) printf(#n" at %p, size %zd\n", &n, sizeof(n))

int main(int argc, char* argv[])
{
    printsize(v1);
    printsize(v2);
    printsize(v21);
    printsize(c22);
    printsize(s0);
    printsize(x0);
    printsize(globalconst);
    printsize(vcchar);
    printsize(cvchar);

    printf("\n===================\n\n");

    get_addrs(argv[0], argv[0],
              (void (*)())&main,
              "getglobals");

    return 0;
}
