#include "codegen.h"
#include "compile-utils.h"

#include "intrinsics.h"

int gen_print (block blk, symstack stk, char *in_buf,
    FILE *out_file, char *msg) {
    
    int ret = BABBLE_OK;
    size_t start = blk.start;
    int start_line = blk.start_line;

    char *sym = in_buf + blk.hotspots[0];
    size_t len = blk.hotspots[1] - blk.hotspots[0] + 1;
    symbol sym_info;
    find_symbol (&sym_info, stk, sym, len);

    int64_t val;
    if (sym_info.name == NULL) {
        INTEG_CHECK (sym, len, &val);
        fprintf (out_file,
            "mov rdi, %ld\n", val);
    } else {
        if (sym_info.category == STRING) {
            size_t str_len = sym_info.size;
            printf ("Printing a string\n");
        } else {
            fprintf (out_file,
                "mov r9, rbp\n"
                "sub r9, 0x%lx\n"
                "mov rdi, [r9]\n", sym_info.offset);
        }
    }
    char *tmp;
    GET_INTRINSIC (&tmp, "print_i64");
    fprintf (out_file, "%s", tmp);
done:
    return ret;
}