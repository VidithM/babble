#ifndef CODEGEN_H
#define CODEGEN_H

#include "lex.h"
#include "symstack.h"

int gen_eq_family (block blk, symstack stk, const char *in_buf);
// TODO: Add other lex block types

#endif