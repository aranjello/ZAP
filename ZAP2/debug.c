#include <stdio.h>

#include "debug.h"
#include "value.h"

static int simpleInstruction(const char* name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

static int byteInstruction(const char* name, Chunk* chunk,
                           int offset) {
  uint8_t slot = chunk->code[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2; 
}

static int jumpInstruction(const char* name, int sign,
                           Chunk* chunk, int offset) {
  uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
  jump |= chunk->code[offset + 2];
  printf("%-16s %4d -> %d\n", name, offset,
         offset + 3 + sign * jump);
  return offset + 3;
}

static int keyInstruction(const char* name,VM* vm, Chunk* chunk, 
                               int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  Key k = vm->globKeys.as.keys[constant];
    printf("%s",k.value);
    printf("'\n");
  return offset + 2;
}

static int constantInstruction(const char* name,VM* vm, Chunk* chunk,
                               int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  printValue(vm->activeArrays.values[constant]);
  printf("'\n");
  return offset + 2;
}

int disassembleInstruction(VM* vm,Chunk* chunk, int offset) {
  printf("%04d ", offset);
 if (offset > 0 &&
      chunk->lines[offset] == chunk->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[offset]);
  }
  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
    case OP_LOOKUP:
      return simpleInstruction("OP_LOOKUP", offset);
    case OP_LOOP:
      return jumpInstruction("OP_LOOP", -1, chunk, offset);
    case OP_JUMP:
      return jumpInstruction("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE:
      return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_ARRAY:
      return constantInstruction("OP_ARRAY",vm, chunk, offset);
    case OP_POP:
      return simpleInstruction("OP_POP", offset);
    case OP_ADD:
      return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT:
      return simpleInstruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
      return simpleInstruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
      return simpleInstruction("OP_DIVIDE", offset);
    case OP_NEGATE:
      return simpleInstruction("OP_NEGATE", offset);
    case OP_PRINT:
      return simpleInstruction("OP_PRINT", offset);
    case OP_GET_GLOBAL:
      return keyInstruction("OP_GET_GLOBAL", vm, chunk, offset);
    case OP_DEFINE_GLOBAL:
      return keyInstruction("OP_DEFINE_GLOBAL", vm, chunk,
                                 offset);
    case OP_SET_GLOBAL:
      return keyInstruction("OP_SET_GLOBAL", vm,chunk, offset);
    case OP_RETURN:
      return simpleInstruction("OP_RETURN", offset);
      case OP_GET_LOCAL:
      return byteInstruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL:
      return byteInstruction("OP_SET_LOCAL", chunk, offset);
    default:
      printf("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}

void disassembleChunk(VM* vm,Chunk* chunk, const char* name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->count;) {
    offset = disassembleInstruction(vm,chunk, offset);
  }
}