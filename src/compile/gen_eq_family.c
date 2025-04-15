#include "codegen.h"

int gen_eq_family (block blk, symstack *stk, const char *in_buf, FILE *out_file, char *msg) {

    size_t start = blk.start;
    size_t start_line = blk.start_line;

    int is_expr = (blk.label % 2); // Whether this is a literal expr
    int is_inc = (blk.label == INC);
    BABBLE_ASSERT_IMPLIES (is_expr, !is_inc);
    
    #if 0
        - Handling exprs:
            - If is_expr, we know its a literal
            - Else, run the symbol check. If null, must be integral
    #endif

    char *lsym, *rsym;
    size_t l_len, r_len;
    symbol lsym_info, rsym_info;

    lsym = in_buf + start;
    l_len = blk.hotspots[0] - start + 1;
    find_symbol (&lsym_info, &stk, lsym, l_len);

    if (!is_expr) {
        rsym = in_buf + blk.hotspots[1];
        r_len = blk.hotspots[2] - blk.hotspots[1] + 1;
        find_symbol (&rsym_info, &stk, rsym, r_len);
    }

    int64_t rval;
    // Every valid symbol has a name. A NULL name means the symbol does not exist.
    if (!is_expr && (rsym_info.name == NULL)) {
        // rsym must be a integer literal
        INTEG_CHECK (rsym, r_len, &rval);
    }
    if (lsym_info.name == NULL) {
        // lsym is known to be a non-literal
        if (is_inc) {
            SYM_NOT_FOUND (lsym, l_len);
        }
        size_t rsym_size;
        // assign
        if (is_expr || (rsym_info.name == NULL)) {
            if (is_expr) {
                rsym_size = blk.hotspots[4] - blk.hotspots[3];
            } else {
                rsym_size = 8;
                fprintf (out_file,
                    "mov r8, %ld\n"
                    "push r8\n", rval);
            }
        } else {
            rsym_size = rsym_info.size;
            if (rsym_info.category == STRING) {                                
            } else {
                fprintf (out_file,
                    "mov r9, rbp\n"
                    "sub r9, %ld\n"
                    "mov r8, [r9]\n"
                    "push r8\n", rsym_info.offset);
            }    
        }
        lsym_info.name = lsym;
        lsym_info.name_len = l_len;
        lsym_info.size = rsym_size;
        lsym_info.offset = frame_size + rsym_size;
        lsym_info.category = INT64;

        ret = insert_symbol (&stk, lsym_info);
        frame_size += lsym_info.size;

        if (ret) { goto done; }
    } else {
        // set
        const char *int_upd_instr = (is_inc ? "add" : "mov");
        if (is_expr || (rsym_info.name == NULL)) {

            fprintf (out_file,
                "mov r8, %ld\n"
                "mov r9, rbp\n"
                "sub r9, %ld\n"
                "%s [r9], r8\n", rval, lsym_info.offset, int_upd_instr);
        } else {
            fprintf (out_file,
                "mov r9, rbp\n"
                "sub r9, %ld\n"
                "mov r8, [r9]\n"
                "mov r9, rbp\n"
                "sub r9, %ld\n"
                "%s [r9], r8\n", rsym_info.offset, lsym_info.offset, int_upd_instr);
        }
    }
}