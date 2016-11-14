#include "util.h"
#include "tst1.h"
#include "tst2.h"

struct S1 v1[10];

volatile const char volatile_const_char;
const volatile char const_volatile_char;
const char          const_char = 1;


void print_tst1(void)
{
    showvar(v1);
    showvar(volatile_const_char);
    showvar(const_volatile_char);
    showvar_readonly(const_char);
}
