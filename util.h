#pragma once

#include <stdio.h>

#define showvar(n)          printf(#n" at %p, size %zd\n", &n, sizeof(n))
#define showvar_readonly(n) printf("readonly "#n" at %p, size %zd\n", &n, sizeof(n))
