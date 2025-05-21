#ifndef PARSE_H
#define PARSE_H

#include "babble-lang.h"

int match (const char *buf, size_t start, size_t end,
    const char *pat, size_t len);
size_t find_next (const char *buf, size_t start, size_t end);
size_t find_next_space (const char *buf, size_t start, size_t end);
size_t find_prev (const char *buf, size_t start, size_t end);
size_t find_prev_space (const char *buf, size_t start, size_t end);
size_t find_next_pat (const char *buf, size_t start, size_t end,
    const char *pat, size_t len);
int valid_symbol (const char *buf, size_t start, size_t end);
int valid_integral (const char *buf, size_t start, size_t end);
int valid_expr (char *buf, size_t start, size_t end, int terminal);
int valid_expr_full (char *buf, size_t start, size_t end,
    int terminal, size_t *hotspots, int *expr_type);

#endif