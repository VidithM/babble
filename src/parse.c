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

int valid_expr (char *buf, size_t start, size_t end, int terminal) {
    #include "lex.h"
    size_t dummy[MAX_HOTSPOTS];
    return valid_expr_full (buf, start, end, terminal, dummy, (int *) dummy);
}

int valid_expr_full (char *buf, size_t start, size_t end,
    int terminal, size_t *hotspots, int *expr_type) {
    
    char *expr_types[2] = {"str", "bool"}; 
    size_t at, expr_end;
    
    if (terminal) {
        // Ensure expr ends with ';'. This is not needed for integral/sym checks
        // because this is implied by them being a contiguous sequence of characters
        at = find_prev (buf, start, end);
        if (at == -1) { return 0; }
        if (buf[at] != ';') { return 0; }
    } else {
        at = end;
    }

    at--;
    // Find end of expr
    expr_end = find_prev (buf, start, at);
    if ((expr_end == -1) || (buf[expr_end] != ')')) { return 0; }

    at = start;
    if (!match (buf, at, end, "expr", 4)) {
        return 0;
    }
    at += 4;

    at = find_next (buf, at, end);
    if ((at == -1) || (buf[at] != '(')) { return 0; }
    at++;

    at = find_next (buf, at, end);
    if (at == -1) { return 0; }

    int type = -1;
    // TODO: Match against more expr types
    for (int i = 0; i < sizeof (expr_types) / sizeof (char *); i++) {
        if (match (buf, at, end, expr_types[i], strlen (expr_types[i]))) {
            type = i; break;
        }
    }
    
    if (type == -1) {
        return 0;
    }
    hotspots[0] = at;

    BABBLE_ASSERT (type >= 0 && type <= 1);
    at += strlen (expr_types[type]);

    at = find_next_pat (buf, at, end, ",", 1);
    if (at == -1) { return 0; }
    at++;
    
    // TODO ...
    if (type == 0) {
        size_t start, put;
        at = find_next (buf, at, end);
        if ((at == -1) || (buf[at] != '\"')) { return 0; }
        start = put = hotspots[1] = at;
        at++; put++; start++;
        at = find_next_pat (buf, at, end, "\"", 1);
        if (at == -1) { return 0; }
        // Replace escape chars
        for (size_t i = start; i < at; i++) {
            if (buf[i] == '\\') {
                if (i == (at - 1)) { return 0; }
                switch (buf[i + 1]) {
                    case 'n':
                        {
                            buf[put] = '\n';
                            i++;
                        }
                        break;
                    default:
                        return 0;
                }
            } else {
                buf[put] = buf[i];
            }
            put++;
        }
        // Mark actual end of string in HS; parsing continues from `at + 1`
        hotspots[2] = put;
        at++;
    } else {
        size_t match_res = at;
        char *ops[4] = {"==", "<", "::", "&&"};
        int op = -1;
        for (int i = 0; i < 4; i++) {
            match_res = find_next_pat (buf, at, end, ops[i], strlen (ops[i]));
            if (match_res != -1) {
                op = i; break;
            }
        }

        if (op == -1) { return 0; }
        // Make the type specific
        type += op;

        size_t ltok_end = find_prev (buf, at, match_res - 1);
        if (ltok_end == -1) { return 0; }
        hotspots[2] = ltok_end;

        size_t ltok_start = find_next (buf, at, match_res - 1);
        BABBLE_ASSERT (ltok_start != -1);
        hotspots[1] = ltok_start;

        size_t rtok_end = find_prev (buf, match_res + strlen (ops[op]), expr_end - 1);
        if (rtok_end == -1) { return 0; }
        hotspots[4] = rtok_end;

        size_t rtok_start = find_next (buf, match_res + strlen (ops[op]), expr_end - 1);
        BABBLE_ASSERT (rtok_start != -1);
        hotspots[3] = rtok_start;

        at = rtok_end + 1;
    }

    // Ensure no further characters until expr_end
    at = find_next (buf, at, end);
    if (at != expr_end) {
        return 0;
    }
    (*expr_type) = type;
    return 1;
}