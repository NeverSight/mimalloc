/* Copyright (c) 2026 NeverSight contributors. MIT license; see LICENSE. */
#include <malloc/malloc.h>
#include <stdlib.h>
#include <string.h>

extern void* mi_test_early_malloc(size_t size);
static int initialized;

__attribute__((constructor)) static void initialize_dependency(void) {
  void* p = mi_test_early_malloc(96);
  // This image uses system malloc APIs. Its legitimate cross-image free must
  // find the replacement allocator even before the executable is initialized.
  malloc_zone_t* zone = malloc_zone_from_ptr(p);
  if (zone == NULL || malloc_size(p) < 96) abort();
  memset(p, 0x5a, 96);
  p = realloc(p, 192);
  if (p == NULL) abort();
  for (size_t i = 0; i < 96; i++) {
    if (((unsigned char*)p)[i] != 0x5a) abort();
  }
  free(p);
  initialized = 1;
}

int mi_test_dependency_initialized(void) {
  return initialized;
}

void* mi_test_dependency_malloc(size_t size) {
  return malloc(size);
}
