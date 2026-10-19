/* Copyright (c) 2026 NeverSight contributors. MIT license; see LICENSE. */
#include "mimalloc.h"
#include <dlfcn.h>
#include <malloc/malloc.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(cond) do { if (!(cond)) { fprintf(stderr, "failed: %s at %d\n", #cond, __LINE__); return 1; } } while (0)

static int tiny_allocations(void) {
  void* blocks[128];
  for (size_t i = 0; i < 128; i++) {
    const size_t size = i % 17;
    blocks[i] = (i % 3 == 0 ? malloc(size) :
                 i % 3 == 1 ? calloc(1, size) : realloc(NULL, size));
    REQUIRE(blocks[i] != NULL);
    REQUIRE(mi_is_in_heap_region(blocks[i]));
    REQUIRE(((uintptr_t)blocks[i] % 16) == 0);
    REQUIRE(malloc_size(blocks[i]) >= 16);
    REQUIRE(malloc_good_size(size) >= 16);
    REQUIRE(malloc_good_size(size) == mi_good_size(size));
    // Check the platform minimum without overwriting another live block or
    // debug padding. Keep a batch live so alternating 8-byte slots cannot hide.
    memset(blocks[i], (int)(i + 1), 16);
  }
  for (size_t i = 0; i < 128; i++) {
    for (size_t j = 0; j < 16; j++) REQUIRE(((unsigned char*)blocks[i])[j] == i + 1);
    free(blocks[i]);
  }
  return 0;
}

static void* fresh_thread(void* ignored) {
  (void)ignored;
  return (void*)(uintptr_t)tiny_allocations();
}

int main(void) {
  REQUIRE(tiny_allocations() == 0);
  pthread_t thread;
  REQUIRE(pthread_create(&thread, NULL, fresh_thread, NULL) == 0);
  void* result = NULL;
  REQUIRE(pthread_join(thread, &result) == 0);
  REQUIRE(result == NULL);

  // Reproduce the ctypes initialization boundary with the system libffi.
  void* lib = dlopen("/usr/lib/libffi.dylib", RTLD_NOW | RTLD_LOCAL);
  REQUIRE(lib != NULL);
  void* (*closure_alloc)(size_t, void**) = (void* (*)(size_t, void**))dlsym(lib, "ffi_closure_alloc");
  void (*closure_free)(void*) = (void (*)(void*))dlsym(lib, "ffi_closure_free");
  REQUIRE(closure_alloc != NULL && closure_free != NULL);
  for (size_t i = 0; i < 32; i++) {
    void* code = NULL;
    void* closure = closure_alloc(1, &code);
    REQUIRE(closure != NULL && code != NULL);
    closure_free(closure);
    REQUIRE(tiny_allocations() == 0);
  }
  REQUIRE(dlclose(lib) == 0);
  return 0;
}
