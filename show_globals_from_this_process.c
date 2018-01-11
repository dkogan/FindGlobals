#include <stdio.h>
#include <stdlib.h>

#include "tst1.h"
#include "tst2.h"

#include "getglobals.h"

int main(int argc, char* argv[])
{
    printf("=================== Program sees: ===================\n");

    print_tst1();
    print_tst2();

    printf("=================== DWARF sees: ===================\n");

    get_addrs_from_this_process_from_DSO_with_function((void (*)())&print_tst1, "tst");
    return 0;
}
