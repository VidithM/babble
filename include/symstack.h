#ifndef SYMSTACK_H
#define SYMSTACK_H

#define NCHARS 36 // a-z, 0-9

typedef struct symtrie {
    int present;
    symtrie *kids [NCHARS];
    size_t offset;
} symtrie;

typedef struct stack_entry {
    size_t rep_id;
    char **symbols;
} stack_entry;

typedef struct symstack {
    stack_entry *scopes;
    symtrie *trie;
} symstack;

void init_symstack ();
void push_symstack_entry ();
void pop_symstack_entry ();
void insert_symbol (const char *symbol);
int find_symbol (const char *symbol);


#endif