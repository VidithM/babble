#ifndef COMPILE_H
#define COMPILE_H

#include "parse.h"

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
    char* TYPE_NAMES[] = {"INT64", "STRING"};                               \
    if (a != b) {                                                           \
        ret = BABBLE_COMPILE_ERR;                                           \
        BABBLE_MSG_COMPILE_ERR (start_line,                                 \
            " (type mismatch: expected %s, got %s)\n",                      \
            TYPE_NAMES[a], TYPE_NAMES[b]);                                  \
        goto done;                                                          \
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

#define WORDSZ 8
#define WORDSZ_CEIL(n) (WORDSZ * ((n + WORDSZ) / WORDSZ))

#endif
