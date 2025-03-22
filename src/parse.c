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
        if ((isspace (buf[i]) != 0) == space) {
            return i;
        }
    }
    return -1;
}

static size_t _find_prev (const char *buf, size_t start,
    size_t end, int space) {

    for (size_t i = end; i >= start; i--) {
        if ((isspace (buf[i]) != 0) == space) {
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

size_t find_prev_space (const char *buf, size_t start, size_t end) {
    return _find_prev (buf, start, end, 1);
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

static int is_alpha (char c) {
    int ret = (c >= 'a' && c <= 'z');
    c ^= 0x20;
    ret = ret || (c >= 'a' && c <= 'z');
    return ret;
}

static int is_digit (char c) {
    return (c >= '0' && c <= '9');
}

int valid_symbol (const char *buf, size_t start, size_t end) {
    if (!is_alpha (buf[start])) {
        return 0;
    }
    for (size_t i = start + 1; i <= end; i++) {
        if (!(is_alpha (buf[i]) || is_digit (buf[i]))) {
            return 0;
        }
    }
    return 1;
}

int valid_integral (const char *buf, size_t start, size_t end) {
    for (size_t i = start; i <= end; i++) {
        if (!is_digit (buf[i])) {
            return 0;
        }
    }
    return 1;
}

int valid_expr (const char *buf, size_t start, size_t end) {
    const int EXPR_LEN = strlen("expr(");
    if (end - start + 1 < EXPR_LEN) {
        return 0;
    }
    return 1;
}