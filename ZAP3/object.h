#ifndef ZAP_object_h
#define ZAP_object_h

#include "common.h"
#include "value.h"

struct KeyString{
    int length;
    char* chars;
    uint32_t hash;
};

#endif