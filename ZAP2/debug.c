#include <stdio.h>

#include "debug.h"
#include "value.h"

/*
Prints a simple one byte instuctions name and increments the offset by 1
@param name The name of the instruction
@param offset the current offset of the instruction from the beginning of its chunk
*/
static int simpleInstruction(const char* name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

/*
Prints a simple one byte instuctions name and increments the offset by 1
@param name The name of the instruction
@param offset the current offset of the instruction from the beginning of its chunk
*/
static int byteInstruction(const char* name, Chunk* chunk,
                           int offset) {
  uint8_t slot = chunk->code[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2; 
}

/*
Prints the name of a jump instruction as well as the location it is jumping from and to
@param name The name of the instruction
@param sign the direction of the jump (1 is forward, -1 is backward)
@param chunk the current chunk the jump is being performed in
@param offset the current offset of the instruction from the beginning of its chunk
*/
static int jumpInstruction(const char* name, int sign,
                           Chunk* chunk, int offset) {
  uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
  jump |= chunk->code[offset + 2];
  printf("%-16s %4d -> %d\n", name, offset,
         offset + 3 + sign * jump);
  return offset + 3;
}

/*
prints out the name of some key that will be used for variable lookup
@param name The name of the instruction
@param vm The currently active VM
@param chunk The chunk the key location is stored in
@param offset the current offset of the instruction from the beginning of its chunk
*/
static int keyInstruction(VM* vm,const char* name, Chunk* chunk, 
                               int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  Key k = vm->globKeys.as.keys[constant];
    printf("%s",k.value);
    printf("'\n");
  return offset + 2;
}

/*
Prints out the value of the array stored in the VM at some offset
@param name The name of the instruction
@param vm The currently active vm
@param chunk The chunk the array offset is stored in
@param offset the current offset of the instruction from the beginning of its chunk
*/
static int arrayInstruction(VM* vm,const char* name, Chunk* chunk,
                               int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  printValue(*vm->constantArrays.values[constant]);
  printf("'\n");
  return offset + 2;
}

/*
Prints out the instruction at a given offset for a given vm and chunk
@param vm The currently active vm
@param chunk The chunk the instruction is stored in
@param offset the current offset of the instruction from the beginning of its chunk
@return Returns the current offset after executing an instruction
*/
int disassembleInstruction(VM* vm,Chunk* chunk, int offset) {
  printf("%04d ", offset);

  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
    printf("   | ");
  else
    printf("%4d ", chunk->lines[offset]);

  
  uint8_t instruction = chunk->code[offset];

  switch (instruction) {
    //Array controls
    case OP_ARRAY:
      return arrayInstruction(vm,"OP_ARRAY", chunk, offset);
    case OP_POP:
      return simpleInstruction("OP_POP", offset);
    //Math ops
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
    case OP_DOT_PROD:
      return simpleInstruction("OP_DOT_PROD", offset);
    //loop ops
    case OP_LOOP:
      return jumpInstruction("OP_LOOP", -1, chunk, offset);
    case OP_JUMP:
      return jumpInstruction("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE:
      return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    //global var ops
    case OP_GET_GLOBAL:
      return keyInstruction(vm,"OP_GET_GLOBAL", chunk, offset);
    case OP_DEFINE_GLOBAL:
      return keyInstruction(vm,"OP_DEFINE_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL:
      return keyInstruction(vm,"OP_SET_GLOBAL",chunk, offset);
    //local var ops
    case OP_GET_LOCAL:
      return byteInstruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL:
      return byteInstruction("OP_SET_LOCAL", chunk, offset);
    //function ops
    case OP_LOOKUP:
      return simpleInstruction("OP_LOOKUP", offset);
    case OP_PRINT:
      return simpleInstruction("OP_PRINT", offset);
    case OP_GET_DIMS:
      return simpleInstruction("OP_GET_DIMS", offset);
    case OP_COMPARE:
      return simpleInstruction("OP_COMPARE", offset);
    case OP_ALL:
      return simpleInstruction("OP_ALL", offset);
    case OP_RETURN:
      return simpleInstruction("OP_RETURN", offset);
    default:
      printf("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}

/*
Loops through a chunk and disassemble all op code and values within it
@param vm The currently active vm
@param chunk The currently active chunk
@param name the name of the chunk
*/
void disassembleChunk(VM* vm,Chunk* chunk, const char* name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->count;) {
    offset = disassembleInstruction(vm,chunk, offset);
  }
}