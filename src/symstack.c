#include "symstack.h"

static void free_symtrie (symtrie *trie) {
    for (size_t i = 0; i < NCHARS; i++) {
        if (trie->kids[i] != NULL) {
            free_symtrie (trie->kids[i]);
        }
    }
    free (trie);
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

int push_symstack_entry (symstack *stk, size_t rep_id) {
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
            stk->scopes = (stack_entry *) realloc (stk->scopes, stk->cap);
            if (stk->scopes == NULL) {
                return BABBLE_MISC_ERR;
            }
        }
    }
    stk->scopes[stk->nscopes].rep_id = rep_id;
    stk->scopes[stk->nscopes].cap = 0;
    stk->scopes[stk->nscopes].nsymbols = 0;
    stk->scopes[stk->nscopes].symbols = NULL;
    stk->nscopes++;
    return BABBLE_OK;
}

void pop_symstack_entry (symstack *stk) {
    if (stk == NULL || stk->nscopes == 0) {
        return;
    }
    // TODO: Remove each symbol in scope from symtrie
    stack_entry *scope = &stk->scopes[stk->nscopes - 1];
    for (size_t i = 0; i < scope->nsymbols; i++) {

    }
    free (scope->symbols);
    stk->nscopes--;
}

int insert_symbol (symstack *stk, const char *symbol) {
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
            if (scope->symbols == NULL) {
                return BABBLE_MISC_ERR;
            }
            scope->cap = 1;
        } else {
            scope->cap *= 2;
            scope->symbols = (const char**) realloc (scope->symbols, scope->cap);
            if (scope->symbols == NULL) {
                return BABBLE_MISC_ERR;
            }
        }
    }
    // TODO: Add symbol to symtrie
    scope->symbols[scope->nsymbols] = symbol;
    scope->nsymbols++;
    return BABBLE_OK;
}
