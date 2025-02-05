#ifndef SYMSTACK_H
#define SYMSTACK_H

#include "babble-lang.h"

#define NCHARS 36 // a-z, 0-9

typedef struct symtrie {
    int present;
    struct symtrie *kids [NCHARS];
    size_t offset;
} symtrie;

typedef struct stack_entry {
    size_t rep_id;
    size_t nsymbols, cap;
    const char **symbols;
    size_t *symbol_lens;
    size_t frame_bottom;
} stack_entry;

typedef struct symstack {
    size_t nscopes, cap;
    stack_entry *scopes;
    symtrie *trie;
} symstack;

int init_symstack (symstack *stk);
void free_symstack (symstack *stk);
int push_symstack_entry (symstack *stk, size_t rep_id, size_t curr_bottom);
int pop_symstack_entry (symstack *stk);
int insert_symbol (symstack *stk, const char *symbol, 
    size_t len, size_t offset);
void find_symbol (size_t *offset, symstack *stk, const char *symbol, size_t len);
void get_curr_frame_bottom (size_t *frame_size, symstack *stk);
void get_curr_frame_rep_id (size_t *rep_id, symstack *stk);

#endif