#ifndef COMPILE_H
#define COMPILE_H

#include "parse.h"
#include "intrinsics.h"

#define GET_INTRINSIC(_ret, _symbol)                        \
{                                                           \
    extern const size_t N_INTRINSICS;                       \
    extern intrinsic_info intrinsics [MAX_INTRINSICS];      \
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

#define SYM_NOT_FOUND(_sym, _len)                                           \
{                                                                           \
    _sym[_len] = '\0';                                                      \
    ret = BABBLE_COMPILE_ERR;                                               \
    BABBLE_MSG_COMPILE_ERR (start_line,                                     \
        " (variable \"%s\" is undefined)\n",  _sym);                        \
    goto done;                                                              \
}

#define TYPE_CHECK(a, b)                                                    \
{                                                                           \
    if (a != b) {                                                           \
        ret = BABBLE_COMPILE_ERR;                                           \
        BABBLE_MSG_COMPILE_ERR (start_line,                                 \
            " (type mismatch: expected %d, got %d)\n", a, b);               \
    }                                                                       \
}

#define INTEG_CHECK(_sym, _len, _val)                                       \
{                                                                           \
    if (!(valid_integral (_sym, 0, _len - 1))) {                            \
        SYM_NOT_FOUND (_sym, _len);                                         \
    }                                                                       \
    char tmp = _sym[_len];                                                  \
    _sym[_len] = '\0';                                                      \
    (*_val) = atoll(_sym);                                                  \
    _sym[_len] = tmp;                                                       \
}

#endif
