#include "parse.h"

int match (const char *start, const char *end,
    const char *pat, size_t len) {
    
    if ((start > end) || (end - start + 1 < len)) {
        return 0;
    }
    for (size_t i = 0; i < len; i++) {
        if (start[i] != pat[i]) {
            return 0;       
        }
    }
    return 1;
}

static size_t _find_next (const char *start, const char *end,
    int space) {

    for (const char *curr = start; curr <= end; curr++) {
        if (isspace (*curr) == space) {
            return (size_t) curr;
        }
    }
    return -1;
}

static size_t _find_prev (const char *start, const char *end,
    int space) {

    for (const char *curr = end; curr >= start; curr--) {
        if (isspace (*curr) == space) {
            return (size_t) curr;
        }
    }
    return -1;
}

size_t find_next (const char *start, const char *end) {
    return _find_next (start, end, 0);
}

size_t find_next_space (const char *start, const char *end) {
    return _find_next (start, end, 1);
}

size_t find_prev (const char *start, const char *end) {
    return _find_prev (start, end, 0);
}

size_t find_next_pat (const char *start, const char *end,
    const char *pat, size_t len) {

    if ((start > end) || (end - start + 1 < len)) {
        return 0;
    }

    for (const char *curr = start; curr + len - 1 <= end; curr++) {
        if (match (curr, end, pat, len)) {
            return (size_t) curr;
        }
    }
    return -1;
}