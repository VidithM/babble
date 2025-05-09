#ifndef CODEGEN_H
#define CODEGEN_H

#include "lex.h"
#include "symstack.h"

int gen_eq_expr (block blk, symstack stk, symbol copy_from, symbol copy_to,
    char *in_buf, size_t *expr_size, FILE *out_file, char *msg);
int gen_eq_family (block blk, symstack stk, char *in_buf, size_t *frame_size,
    FILE *out_file, char *msg);
int gen_print (block blk, symstack stk, char *in_buf,
    FILE *out_file, char *msg);
// TODO: Add other lex block types

#endif