#include <stdio.h>
#include <stdlib.h>

#include "tst1.h"
#include "tst2.h"

#include "getglobals.h"

int main(int argc, char* argv[])
{
#define USAGE "%s elf_file"

    if( argc != 2 )
    {
        fprintf(stderr, USAGE "\n", argv[0]);
        return 1;
    }

    if(!get_addrs_from_ELF(argv[1], NULL))
        return 1;

    return 0;
}
