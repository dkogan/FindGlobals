#pragma once

#include <stdbool.h>

bool get_addrs( // Pointer to any function in the ELF file we're instrumenting
                void (*func)(void),

                // Only source files that contain this string will report their
                // data. NULL if all source files should report their data
                const char* source_pattern);


