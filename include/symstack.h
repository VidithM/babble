#ifndef SYMSTACK_H
#define SYMSTACK_H

#include "babble-lang.h"

#define NCHARS 62 // a-z, A-Z, 0-9

enum sym_category {
    INT64,
    STRING
    // FUTURE: BOOL, FUNCTION?
};

typedef struct symbol {
    const char *name;
    size_t name_len;
    size_t size;
    size_t offset;
    enum sym_category category;
} symbol;

typedef struct symstack_opaque* symstack;

int init_symstack (symstack *stk);
void free_symstack (symstack *stk);
int push_symstack_entry (symstack *stk, size_t rep_id, size_t curr_bottom);
int pop_symstack_entry (symstack *stk);
int insert_symbol (symstack *stk, symbol sym);
void find_symbol (symbol *sym, const symstack stk, const char *symbol, size_t len);
// TODO: Get rid of below 2?
void get_curr_frame_bottom (size_t *frame_size, const symstack stk);
void get_curr_frame_rep_id (size_t *rep_id, const symstack stk);
void get_nscopes (size_t *nscopes, const symstack stk);

#endif