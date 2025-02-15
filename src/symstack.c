#include "symstack.h"

static size_t char_to_kid (char c) {
    if (c >= 'a' && c <= 'z') {
        return (c - 'a');
    }
    return (c - '0' + 26);
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

static int symtrie_insert (symtrie *node, const char *symbol,
    size_t len, size_t offset, size_t idx) {
    BABBLE_ASSERT ((node != NULL) && (symbol != NULL));

    if (idx == len) {
        node->present = 1;
        node->offset = offset;
        return 1;
    }
    size_t nxt = char_to_kid (symbol[idx]);
    if (node->kids[nxt] == NULL) {
        node->kids[nxt] = (symtrie *) malloc (sizeof (symtrie));
        if (node->kids[nxt] == NULL) {
            return BABBLE_MISC_ERR;
        }
        node->kids[nxt]->present = 0;
        memset (node->kids[nxt]->kids, 0x0, NCHARS * sizeof (symtrie *));
    }
    return symtrie_insert (node->kids[nxt], symbol, len, offset, idx + 1);
}

static int symtrie_remove (symtrie *node, const char *symbol,
    size_t len, size_t idx) {
    BABBLE_ASSERT ((node != NULL) && (symbol != NULL));

    if (idx == len) {
        node->present = 0;
    } else {
        size_t nxt = char_to_kid (symbol[idx]);
        if (node->kids[nxt] != NULL) {
            int del = symtrie_remove (node->kids[nxt], symbol,len, idx + 1);
            if (del) {
                free (node->kids[nxt]);
                node->kids[nxt] = NULL;
            }
        }
    }
    int keep = node->present;
    for (size_t i = 0; i < NCHARS; i++) {
        keep = keep || (node->kids[i] != NULL);
        if (keep) { break; }
    }
    if (!keep) {
        return 1; // delete this node
    }
    return 0;
}

static void symtrie_find (size_t *offset, symtrie *node, const char *symbol,
    size_t len, size_t idx) {
    BABBLE_ASSERT (node != NULL);
    BABBLE_ASSERT (symbol != NULL);

    if (idx == len) {
        if (!node->present) {
            (*offset) = (size_t) -1;
        } else {
            (*offset) = node->offset;
        }
        return;
    }
    size_t nxt = char_to_kid (symbol[idx]);
    if (node->kids[nxt] == NULL) {
        (*offset) = -1;
        return;
    }
    symtrie_find (offset, node->kids[nxt], symbol, len, idx + 1);
}

void free_symstack (symstack *stk) {
    BABBLE_ASSERT (stk != NULL);

    for (size_t i = 0; i < stk->nscopes; i++) {
        free (stk->scopes[i].symbols);
    }
    free (stk->scopes);
    free_symtrie (stk->trie);
}

int init_symstack (symstack *stk) {
    BABBLE_ASSERT (stk != NULL);

    stk->nscopes = 0;
    stk->cap = 0;
    push_symstack_entry (stk, -1, 0);

    stk->trie = (symtrie *) malloc (sizeof (symtrie));
    if (stk->scopes == NULL || stk->trie == NULL) {
        return BABBLE_MISC_ERR;
    }
    stk->trie->present = 0;
    memset (stk->trie->kids, 0x0, NCHARS * sizeof (symtrie *));
    return BABBLE_OK;
}

int push_symstack_entry (symstack *stk, size_t rep_id, size_t curr_bottom) {
    BABBLE_ASSERT (stk != NULL);
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
    stk->scopes[stk->nscopes].symbol_lens = NULL;
    stk->scopes[stk->nscopes].frame_bottom = curr_bottom;
    stk->nscopes++;
    return BABBLE_OK;
}

int pop_symstack_entry (symstack *stk) {
    BABBLE_ASSERT (stk != NULL);
    if (stk->nscopes <= 1) {
        return BABBLE_BAD_ARGS;
    }
    stack_entry *scope = &stk->scopes[stk->nscopes - 1];
    for (size_t i = 0; i < scope->nsymbols; i++) {
        symtrie_remove (stk->trie, scope->symbols[i],
            scope->symbol_lens[i], 0);
    }
    free (scope->symbols);
    stk->nscopes--;
    return BABBLE_OK;
}

int insert_symbol (symstack *stk, const char *symbol,
    size_t len, size_t offset) {
    
    BABBLE_ASSERT (stk != NULL);
    BABBLE_ASSERT (stk->nscopes > 0);

    stack_entry *scope = &stk->scopes[stk->nscopes - 1];
    if (scope->cap == scope->nsymbols) {
        if (scope->nsymbols == 0) {
            scope->symbols = (const char**) malloc (sizeof (char*));
            scope->symbol_lens = (size_t *) malloc (sizeof (size_t));
            if (scope->symbols == NULL || scope->symbol_lens == NULL) {
                return BABBLE_MISC_ERR;
            }
            scope->cap = 1;
        } else {
            scope->cap *= 2;
            scope->symbols = (const char**) realloc (scope->symbols,
                scope->cap * sizeof (char*));
            scope->symbol_lens = (size_t *) realloc (scope->symbol_lens,
                scope->cap * sizeof (size_t));

            if (scope->symbols == NULL || scope->symbol_lens == NULL) {
                return BABBLE_MISC_ERR;
            }
        }
    }
    // TODO: Add symbol to symtrie
    scope->symbols[scope->nsymbols] = symbol;
    scope->symbol_lens[scope->nsymbols] = len;
    if (!symtrie_insert (stk->trie, symbol, len, offset, 0)) {
        return BABBLE_MISC_ERR;
    }
    scope->nsymbols++;
    scope->frame_bottom += 8;
    return BABBLE_OK;
}

void find_symbol (size_t *offset, symstack *stk, const char *symbol,
    size_t len) {

    BABBLE_ASSERT (stk != NULL);
    BABBLE_ASSERT (offset != NULL);
    BABBLE_ASSERT (symbol != NULL);
    BABBLE_ASSERT (len > 0);

    symtrie_find (offset, stk->trie, symbol, len, 0);
}

void get_curr_frame_bottom (size_t *frame_bottom, symstack *stk) {

    BABBLE_ASSERT (stk != NULL);
    BABBLE_ASSERT (frame_bottom != NULL);
    BABBLE_ASSERT (stk->nscopes > 0);

    (*frame_bottom) = stk->scopes[stk->nscopes - 1].frame_bottom;
}

void get_curr_frame_rep_id (size_t *rep_id, symstack *stk) {

    BABBLE_ASSERT (stk != NULL);
    BABBLE_ASSERT (rep_id != NULL);
    BABBLE_ASSERT (stk->nscopes > 0);

    (*rep_id) = stk->scopes[stk->nscopes - 1].rep_id;
}