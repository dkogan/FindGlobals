#include "util.h"
#include "tst1.h"
#include "tst2.h"

int v2 = 5;

const char*       const_char_pointer       = NULL;
const char const* const_char_const_pointer = NULL;
char const*       char_const_pointer       = NULL;

const        char  const_string_array[]        = "456";
static const char  static_const_string_array[] = "abc";

struct S0 s0;
int       x0[0];
int       x1[30];



void print_tst2(void)
{
    static int    static_int_5[5];
    static double static_double;
    double dynamic_d;

    showvar(static_int_5);
    showvar(static_double);
    showvar(v2);
    showvar(const_char_pointer);
    showvar_readonly(const_char_const_pointer);
    showvar_readonly(char_const_pointer);
    showvar_readonly(const_string_array);
    showvar_readonly(static_const_string_array);
    showvar(s0);
    showvar(x0);
    showvar(x1);
}
