
#ifndef MY_MAPS_H
#define MY_MAPS_H

#include "zmap.h"

#define REGISTER_MAP_TYPES(X)   \
    X(char*, int,   StrInt)     \
    X(int,   float, IntFloat)

REGISTER_MAP_TYPES(DEFINE_MAP_TYPE)

#endif
