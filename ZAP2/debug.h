#ifndef ZAP_debug_h
#define ZAP_debug_h

#include "codeChunk.h"
#include "vm.h"

void disassembleChunk(Chunk* chunk, const char* name);
int disassembleInstruction(Chunk* chunk, int offset);

#endif