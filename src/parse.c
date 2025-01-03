#include "parse.h"

int match (const char *buf, size_t start, size_t end,
    const char *pat, size_t len) {
    
    if ((start > end) || (end - start + 1 < len)) {
        return 0;
    }
    for (size_t i = 0; i < len; i++) {
        if (buf[start + i] != pat[i]) {
            return 0;       
        }
    }
    return 1;
}

static size_t _find_next (const char *buf, size_t start,
    size_t end, int space) {

    for (size_t i = start; i <= end; i++) {
        if (isspace (buf[i]) == space) {
            return i;
        }
    }
    return -1;
}

static size_t _find_prev (const char *buf, size_t start,
    size_t end, int space) {

    for (size_t i = end; i >= start; i--) {
        if (isspace (buf[i]) == space) {
            return i;
        }
    }
    return -1;
}

size_t find_next (const char *buf, size_t start, size_t end) {
    return _find_next (buf, start, end, 0);
}

size_t find_next_space (const char *buf, size_t start, size_t end) {
    return _find_next (buf, start, end, 1);
}

size_t find_prev (const char *buf, size_t start, size_t end) {
    return _find_prev (buf, start, end, 0);
}

size_t find_next_pat (const char *buf, size_t start, size_t end,
    const char *pat, size_t len) {
        
    for (size_t i = start; i + len - 1 <= end; i++) {
        if (match (buf, i, i + len - 1, pat, len)) {
            return i;
        }
    }
    return -1;
}