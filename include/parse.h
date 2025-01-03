#ifndef PARSE_H
#define PARSE_H

#include "babble-lang.h"

int match (const char *start, const char *end,
    const char *pat, size_t len);
size_t find_next (const char *start, const char *end);
size_t find_next_space (const char *start, const char *end);
size_t find_prev (const char *start, const char *end);
size_t find_next_pat (const char *start, const char *end,
    const char *pat, size_t len);

#endif