#ifndef LEX_H
#define LEX_H

#include "babble-lang.h"

enum block_label {
    UNKNOWN,
    EMPTY,
    INC,
    EQ,
    REP,
    PRINT,
    SCOPE_OPEN,
    SCOPE_CLOSE
};

#define MAX_HOTSPOTS 32
#define DEFAULT_CAP 16

typedef struct block {
    size_t hotspots [MAX_HOTSPOTS];
    size_t start, end;
    size_t start_line;
    enum block_label label;
} block;

typedef struct lexer_blocklist {
    size_t nblocks;
    size_t cap;
    block *blocks;
} blocklist;

void free_blist (blocklist *blist);
int lex (char *in_buf, size_t buf_size, blocklist *blist,
    char *msg);

#endif