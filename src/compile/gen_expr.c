#include "codegen.h"
#include "compile-utils.h"

// Gen code to push a literal expr onto the stack
// In the future, when dealing with bool exprs, can use a separate static gen_bexpr, maybe with
// recursive evaluation
int gen_expr (block blk, symstack stk,
    symbol copy_from, symbol copy_to, // if copy_to, pop from the stack at the end
    const char *in_buf, size_t *expr_size, FILE *out, char *msg) {
    
    int ret = BABBLE_OK;
    enum sym_category expr_type = blk.label - EQ;

    switch (expr_type) {
        case STRING:
            {
                
            }
            break;
        default:
            // The parse step should already validate the expr type
            BABBLE_ASSERT (0);
    }
    return ret;
}