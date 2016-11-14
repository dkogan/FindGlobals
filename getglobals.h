#pragma once

#include <stdbool.h>

bool get_addrs(const char* name,
               const char* filename,
               void (*func)(void),
               const char* source_pattern);


