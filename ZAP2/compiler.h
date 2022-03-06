#ifndef ZAP_compiler_h
#define ZAP_compiler_h

#include "vm.h"

bool compile(const char* source, Chunk* chunk, VM* vm);

#endif