/* Copyright (c) 2026 NeverSight contributors. MIT license; see LICENSE. */
#include "mimalloc.h"
#include <stdlib.h>

extern int mi_test_dependency_initialized(void);
extern void* mi_test_dependency_malloc(size_t size);

// A dependency initializer can call an executable's allocator before the
// executable's own constructors run (for example through merged C++ symbols).
void* mi_test_early_malloc(size_t size) {
  void* p = malloc(size);
  if (p == NULL || !mi_is_in_heap_region(p)) abort();
  return p;
}

int main(void) {
  if (mi_test_dependency_initialized() != 1) return 1;
  void* p = mi_test_early_malloc(96);
  free(p);
  // Early registration must also retain the normal default-zone override.
  p = mi_test_dependency_malloc(96);
  if (p == NULL || !mi_is_in_heap_region(p)) return 2;
  free(p);
  return 0;
}
