#include "symstack.h"

typedef struct symtrie {
    symbol sym; // sym.name == NULL if not present. A valid symbol always has a name,
                // so this is an acceptable way to encode no symbol.
    struct symtrie *kids [NCHARS];
} symtrie;

typedef struct stack_entry {
    size_t rep_id;
    size_t nsymbols, cap;
    symbol *symbols;
    size_t frame_bottom;
} stack_entry;

#define SYMSTACK_MAGIC_INIT 0x10716905

typedef struct symstack_opaque {
    size_t nscopes, cap;
    stack_entry *scopes;
    symtrie *trie;
    int32_t magic;
} symstack_opaque;

static size_t char_to_kid (char c) {
    if (c >= 'a' && c <= 'z') {
        return (c - 'a');
    }
    if (c >= 'A' && c <= 'z') {
        return (c - 'A') + 26;
    }
    BABBLE_ASSERT (c >= '0' && c <= '9');
    return (c - '0' + 52);
}

static void free_symtrie (symtrie *node) {
    BABBLE_ASSERT (node != NULL);

    for (size_t i = 0; i < NCHARS; i++) {
        if (node->kids[i] != NULL) {
            free_symtrie (node->kids[i]);
        }
    }
    free (node);
}

static int symtrie_insert (symtrie *node, symbol sym, size_t idx) {
    BABBLE_ASSERT ((node != NULL) && (sym.name != NULL));
    if (idx == sym.name_len) {
        node->sym = sym;
        return 1;
    }
    size_t nxt = char_to_kid (sym.name[idx]);
    if (node->kids[nxt] == NULL) {
        node->kids[nxt] = (symtrie *) malloc (sizeof (symtrie));
        if (node->kids[nxt] == NULL) {
            return BABBLE_MISC_ERR;
        }
        node->kids[nxt]->sym.name = NULL;
        memset (node->kids[nxt]->kids, 0x0, NCHARS * sizeof (symtrie *));
    }
    return symtrie_insert (node->kids[nxt], sym, idx + 1);
}

static int symtrie_remove (symtrie *node, symbol sym, size_t idx) {
    BABBLE_ASSERT ((node != NULL) && (sym.name != NULL));
    if (idx == sym.name_len) {
        node->sym.name = NULL;
    } else {
        size_t nxt = char_to_kid (sym.name[idx]);
        if (node->kids[nxt] != NULL) {
            int del = symtrie_remove (node->kids[nxt], sym, idx + 1);
            if (del) {
                free (node->kids[nxt]);
                node->kids[nxt] = NULL;
            }
        }
    }
    int keep = (node->sym.name != NULL);
    for (size_t i = 0; i < NCHARS; i++) {
        keep = keep || (node->kids[i] != NULL);
        if (keep) { break; }
    }
    if (!keep) {
        return 1; // delete this node
    }
    return 0;
}

static void symtrie_find (symbol *sym, symtrie *node, const char *sym_name,
    size_t len, size_t idx) {
    BABBLE_ASSERT (node != NULL);
    BABBLE_ASSERT (sym != NULL);

    if (idx == len) {
        if (!node->sym.name) {
            sym->name = NULL;
        } else {
            (*sym) = node->sym;
        }
        return;
    }
    size_t nxt = char_to_kid (sym_name[idx]);
    if (node->kids[nxt] == NULL) {
        sym->name = NULL;
        return;
    }
    symtrie_find (sym, node->kids[nxt], sym_name, len, idx + 1);
}

void free_symstack (symstack *stk_out) {
    if (stk_out == NULL) {
        return;
    }
    symstack_opaque *stk = (*stk_out);
    if (stk == NULL) {
        return;
    }

    BABBLE_ASSERT (stk->magic == SYMSTACK_MAGIC_INIT);
    for (size_t i = 0; i < stk->nscopes; i++) {
        free (stk->scopes[i].symbols);
    }
    free (stk->scopes);
    free_symtrie (stk->trie);
    free (stk);
    (*stk_out) = NULL;
}

int init_symstack (symstack *stk_out) {
    BABBLE_ASSERT (stk_out != NULL);

    (*stk_out) = (symstack_opaque *) malloc (sizeof (symstack_opaque));
    if ((*stk_out) == NULL) {
        return BABBLE_MISC_ERR;
    }
    symstack_opaque *stk = (*stk_out);

    stk->magic = SYMSTACK_MAGIC_INIT;
    stk->nscopes = 0;
    stk->cap = 0;
    push_symstack_entry (stk_out, -1, 0);

    stk->trie = (symtrie *) malloc (sizeof (symtrie));
    if (stk->scopes == NULL || stk->trie == NULL) {
        return BABBLE_MISC_ERR;
    }
    stk->trie->sym.name = NULL;
    memset (stk->trie->kids, 0x0, NCHARS * sizeof (symtrie *));
    return BABBLE_OK;
}

int push_symstack_entry (symstack *stk_out, size_t rep_id, size_t curr_bottom) {
    BABBLE_ASSERT (stk_out != NULL);

    // May segfault. OK, since that would be a bug in the caller (who owns
    // the symstack) and not this code.
    symstack_opaque *stk = (*stk_out);
    BABBLE_ASSERT ((stk != NULL) && (stk->magic == SYMSTACK_MAGIC_INIT));

    if (stk->cap == stk->nscopes) {
        if (stk->cap == 0) {
            stk->scopes = (stack_entry *) malloc (sizeof (stack_entry));
            if (stk->scopes == NULL) {
                return BABBLE_MISC_ERR;
            }
            stk->cap = 1;
        } else {
            stk->cap *= 2;
            stk->scopes = (stack_entry *) realloc (stk->scopes,
                stk->cap * sizeof (stack_entry));
            if (stk->scopes == NULL) {
                return BABBLE_MISC_ERR;
            }
        }
    }
    if (stk->nscopes > 0) {
        stk->scopes[stk->nscopes - 1].frame_bottom = curr_bottom;
    }
    stk->scopes[stk->nscopes].rep_id = rep_id;
    stk->scopes[stk->nscopes].cap = 0;
    stk->scopes[stk->nscopes].nsymbols = 0;
    stk->scopes[stk->nscopes].symbols = NULL;
    stk->scopes[stk->nscopes].frame_bottom = curr_bottom;
    stk->nscopes++;
    return BABBLE_OK;
}

int pop_symstack_entry (symstack *stk_out) {
    BABBLE_ASSERT (stk_out != NULL);

    symstack_opaque *stk = (*stk_out);
    BABBLE_ASSERT ((stk != NULL) && (stk->magic == SYMSTACK_MAGIC_INIT));

    if (stk->nscopes <= 1) {
        return BABBLE_BAD_ARGS;
    }
    stack_entry *scope = &stk->scopes[stk->nscopes - 1];
    for (size_t i = 0; i < scope->nsymbols; i++) {
        symtrie_remove (stk->trie, scope->symbols[i], 0);
    }
    free (scope->symbols);
    stk->nscopes--;
    return BABBLE_OK;
}

int insert_symbol (symstack *stk_out, symbol sym) {
    
    BABBLE_ASSERT (stk_out != NULL);

    symstack_opaque *stk = (*stk_out);
    BABBLE_ASSERT ((stk != NULL) && (stk->magic == SYMSTACK_MAGIC_INIT));
    
    BABBLE_ASSERT (stk->nscopes > 0);

    stack_entry *scope = &stk->scopes[stk->nscopes - 1];
    if (scope->cap == scope->nsymbols) {
        if (scope->nsymbols == 0) {
            scope->symbols = (symbol*) malloc (sizeof (symbol));
            if (scope->symbols == NULL) {
                return BABBLE_MISC_ERR;
            }
            scope->cap = 1;
        } else {
            scope->cap *= 2;
            scope->symbols = (symbol*) realloc (scope->symbols,
                scope->cap * sizeof (symbol));

            if (scope->symbols == NULL) {
                return BABBLE_MISC_ERR;
            }
        }
    }
    scope->symbols[scope->nsymbols] = sym;
    if (!symtrie_insert (stk->trie, scope->symbols[scope->nsymbols], 0)) {
        return BABBLE_MISC_ERR;
    }
    scope->nsymbols++;
    scope->frame_bottom += sym.size;
    return BABBLE_OK;
}

void find_symbol (symbol *sym, const symstack stk, const char *sym_name,
    size_t len) {

    BABBLE_ASSERT ((stk != NULL) && (stk->magic == SYMSTACK_MAGIC_INIT));
    BABBLE_ASSERT (sym != NULL);
    BABBLE_ASSERT (sym_name != NULL);
    BABBLE_ASSERT (len > 0);

    symbol sym_res;
    symtrie_find (&sym_res, stk->trie, sym_name, len, 0);
    
    if (sym_res.name == NULL) {
        sym->name = NULL;
    } else {
        (*sym) = sym_res;
    }
}

void get_curr_frame_bottom (size_t *frame_bottom, const symstack stk) {

    BABBLE_ASSERT ((stk != NULL) && (stk->magic == SYMSTACK_MAGIC_INIT));
    BABBLE_ASSERT (frame_bottom != NULL);
    BABBLE_ASSERT (stk->nscopes > 0);

    (*frame_bottom) = stk->scopes[stk->nscopes - 1].frame_bottom;
}

void get_curr_frame_rep_id (size_t *rep_id, const symstack stk) {

    BABBLE_ASSERT ((stk != NULL) && (stk->magic == SYMSTACK_MAGIC_INIT));
    BABBLE_ASSERT (rep_id != NULL);
    BABBLE_ASSERT (stk->nscopes > 0);

    (*rep_id) = stk->scopes[stk->nscopes - 1].rep_id;
}

void get_nscopes (size_t *nscopes, const symstack stk) {

    BABBLE_ASSERT ((stk != NULL) && (stk->magic == SYMSTACK_MAGIC_INIT));
    BABBLE_ASSERT (nscopes != NULL);

    (*nscopes) = stk->nscopes;
}