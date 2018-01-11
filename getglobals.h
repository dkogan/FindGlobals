#pragma once

#include <stdbool.h>

bool get_addrs_from_this_process_from_DSO_with_function(
  // Pointer to any function in the ELF file we're instrumenting
  void (*func)(void),

  // Only source files that contain this string will report their
  // data. NULL if all source files should report their data
  const char* source_pattern);

bool get_addrs_from_ELF(
  // File that we're looking at
  const char* file,

  // Only source files that contain this string will report their
  // data. NULL if all source files should report their data
  const char* source_pattern);


