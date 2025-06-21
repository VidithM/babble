#ifndef SYMSTACK_H
#define SYMSTACK_H

#include "babble-lang.h"
#include "lex.h"

#define NCHARS 62 // a-z, A-Z, 0-9

enum sym_category {
    INT64,
    STRING,
    BOOL // Equivalent to INT64; this is a different category
         // due to EXPR_SYMCAT being used to distinguish
         // expr types in gen_eq_expr. Could instead make
         // EXPR_SYMCAT reflect the true type and have a separate
         // expr_category enum in compile-utils.h
    // FUTURE: FUNCTION?
};

enum sym_location {
    LOCAL, // default
    GLOBAL,
    EXTERN, 
};

typedef struct symbol {
    const char *name;
    size_t name_len;
    size_t size;
    size_t offset;
    enum sym_category category;
    enum sym_location location;
} symbol;

typedef struct symstack_opaque* symstack;

int init_symstack (symstack *stk);
void free_symstack (symstack *stk);
int push_symstack_entry (symstack *stk, enum block_label ctrl_type, int ctrl_id,
    size_t curr_bottom);
int pop_symstack_entry (symstack *stk);
int insert_symbol (symstack *stk, symbol sym);
void find_symbol (symbol *sym, const symstack stk, const char *sym_name, size_t len);
void set_symbol (symstack *stk, symbol targ, symbol to);
// TODO: Get rid of below 2?
void get_curr_frame_bottom (size_t *frame_size, const symstack stk);
void get_curr_frame_control_id (enum block_label *ctrl_type, int *id,
    const symstack stk);
void get_nscopes (size_t *nscopes, const symstack stk);

#endif