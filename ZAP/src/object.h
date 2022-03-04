#ifndef zap_object_h
#define zap_object_h

#include "common.h"
#include "value.h"

#define IS_STRING(value)       isObjType(value, OBJ_STRING)

#define AS_STRING(value)       ((ObjString*)*AS_OBJ_ARR(value))
#define AS_CSTRING(value)      (((ObjString*)*AS_OBJ_ARR(value))->chars)

typedef enum {
  OBJ_STRING,
} ObjType;

struct Obj {
  ObjType type;
  struct Obj* next;
};

struct ObjString {
  Obj obj;
  int length;
  char* chars;
  uint32_t hash;
};

ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
void printObject(Obj* value);

static inline bool isObjType(Array value, ObjType type) {
  return IS_OBJ_ARR(value) && value.type == type;
}

#endif