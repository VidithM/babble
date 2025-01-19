#include "symstack.h"

static size_t char_to_kid (char c) {
    if (c <= 'z') {
        return (c - 'a');
    }
    return (c - '0' + 26);
}

static void free_symtrie (symtrie *trie) {
    for (size_t i = 0; i < NCHARS; i++) {
        if (trie->kids[i] != NULL) {
            free_symtrie (trie->kids[i]);
        }
    }
    free (trie);
}

static int symtrie_insert (symtrie *node, const char *symbol,
    size_t len, size_t offset, size_t idx) {

    if (node == NULL) {
        return 0;
    }
    if (idx == len - 1) {
        node->present = 1;
        node->offset = offset;
        return 1;
    }
    size_t nxt = char_to_kid (symbol[idx]);
    if (node->kids[nxt] == NULL) {
        node->kids[nxt] = (symtrie *) malloc (sizeof (symtrie));
        if (node->kids[nxt] == NULL) {
            return 0;
        }
        node->kids[nxt]->present = 0;
        memset (node->kids[nxt]->kids, 0x0, NCHARS * sizeof (symtrie *));
    }
    symtrie_insert (node->kids[nxt], symbol, len, offset, idx + 1);
}

static int symtrie_remove (symtrie *node, const char *symbol,
    size_t len, size_t idx) {
    
    if (node == NULL) {
        return 0;
    }
    if (idx == len - 1) {
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

void free_symstack (symstack *stk) {
    if (stk == NULL) {
        return;
    }
    for (size_t i = 0; i < stk->nscopes; i++) {
        free (stk->scopes[i].symbols);
    }
    free (stk->scopes);
    free_symtrie (stk->trie);
}

int init_symstack (symstack *stk) {
    if (stk == NULL) {
        return BABBLE_BAD_ARGS;
    }
    stk->nscopes = 0;
    stk->cap = 0;
    push_symstack_entry (stk, -1);

    stk->trie = (symtrie *) malloc (sizeof (symtrie));
    if (stk->scopes == NULL || stk->trie == NULL) {
        return BABBLE_MISC_ERR;
    }
    stk->trie->present = 0;
    memset (stk->trie->kids, 0x0, NCHARS * sizeof (symtrie *));
    return BABBLE_OK;
}

int push_symstack_entry (symstack *stk, int rep_id) {
    if (stk == NULL) {
        return BABBLE_BAD_ARGS;
    }
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
    stk->scopes[stk->nscopes].rep_id = rep_id;
    stk->scopes[stk->nscopes].cap = 0;
    stk->scopes[stk->nscopes].nsymbols = 0;
    stk->scopes[stk->nscopes].symbols = NULL;
    stk->scopes[stk->nscopes].symbol_lens = NULL;
    stk->nscopes++;
    return BABBLE_OK;
}

int pop_symstack_entry (symstack *stk) {
    if (stk == NULL || stk->nscopes == 1) {
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

    if (stk == NULL) {
        return BABBLE_BAD_ARGS;
    }
    if (stk->nscopes == 0) {
        return BABBLE_BAD_ARGS;
    }
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
    return BABBLE_OK;
}
