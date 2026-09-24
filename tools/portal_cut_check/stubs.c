// Host stand-ins for what the cutter reaches outside itself.
#include <stdlib.h>
#include <string.h>

void* portalMalloc(unsigned int size) { return malloc(size); }
void portalFree(void* p) { free(p); }
void* portalRealloc(void* p, unsigned int size) { return realloc(p, size); }
void zeroMemory(void* m, int size) { memset(m, 0, size); }
void memCopy(void* t, const void* s, int size) { memmove(t, s, size); }
void* stackMalloc(int size) { return malloc(size); }
void stackMallocFree(void* p) { free(p); }
struct LevelDefinition* gCurrentLevel;
#include <assert.h>
void portalAssert(int a) { assert(a); }
