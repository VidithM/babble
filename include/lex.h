#ifndef LEX_H
#define LEX_H

#include "babble-lang.h"

enum block_label {
    UNKNOWN,
    EMPTY,
    INC,
    EQ,
    EQ_STR_EXPR,
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
    int start_line;
    enum block_label label;
} block;

typedef struct lexer_blocklist {
    size_t nblocks;
    size_t cap;
    block *blocks;
} blocklist;

void dbg_blist (const char *name, blocklist blist);
void free_blist (blocklist *blist);
int lex (char *in_buf, size_t buf_size, blocklist *blist,
    char *msg);

#endif