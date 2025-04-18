#ifndef INTRINSICS_H
#define INTRINSICS_H

#include "intrinsic-info.h"

#define MAX_INTRINSICS 128

#define GET_INTRINSIC(_ret, _symbol)                        \
{                                                           \
    (*_ret) = NULL;                                         \
    for (int i = 0;                                         \
        i < sizeof (intrinsics) / sizeof (intrinsic_info);  \
        i++) {                                              \
                                                            \
        if (!strcmp (intrinsics[i].symbol, _symbol)) {      \
            (*_ret) = intrinsics[i].source;                 \
            break;                                          \
        }                                                   \
    }                                                       \
}

extern const size_t N_INTRINSICS;
extern intrinsic_info intrinsics [MAX_INTRINSICS];

#endif