#ifndef ZAP_debug_h
#define ZAP_debug_h

#include "codeChunk.h"
#include "vm.h"

void disassembleChunk(VM* vm,Chunk* chunk, const char* name);
int disassembleInstruction(VM* vm,Chunk* chunk, int offset);

#endif