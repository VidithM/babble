#include "codegen.h"
#include "compile-utils.h"

int gen_eq_family (block blk, symstack stk, char *in_buf, size_t *frame_size, 
    FILE *out_file, char *msg) {
    
    int ret = BABBLE_OK;
    size_t start = blk.start;
    int start_line = blk.start_line;

    int is_expr = (blk.label > EQ && blk.label < REP); // Whether this is a literal expr
    int is_inc = (blk.label == INC);
    BABBLE_ASSERT_IMPLIES (is_expr, !is_inc);

    char *lsym, *rsym;
    size_t l_len, r_len;
    symbol lsym_info, rsym_info;
    symbol lsym_new_info;
    size_t rsym_size;

    lsym = in_buf + start;
    l_len = blk.hotspots[0] - start + 1;
    find_symbol (&lsym_info, stk, lsym, l_len);

    if (is_expr) {
        if (lsym_info.name != NULL) {
            TYPE_CHECK (lsym_info.category, blk.label - EQ);
        }
        rsym_info.name = NULL;
        ret = gen_eq_expr (blk, stk, rsym_info, lsym_info, in_buf, 
            &rsym_size, out_file, msg);
        
        if (ret) { goto done; }

        if (lsym_info.name == NULL) {
            lsym_info.name = lsym;
            lsym_info.name_len = l_len;
            lsym_info.size = rsym_size;
            lsym_info.cap = rsym_size;
            lsym_info.offset = (*frame_size) + WORDSZ_CEIL (rsym_size);
            lsym_info.category = blk.label - EQ;
            ret = insert_symbol (&stk, lsym_info);
            (*frame_size) += WORDSZ_CEIL (lsym_info.size);
        } else {
            lsym_new_info = lsym_info;
            lsym_new_info.size = rsym_size;
            set_symbol (&stk, lsym_info, lsym_new_info);
        }
    } else {
        rsym = in_buf + blk.hotspots[1];
        r_len = blk.hotspots[2] - blk.hotspots[1] + 1;
        find_symbol (&rsym_info, stk, rsym, r_len);

        int64_t rval;
        // Every valid symbol has a name. A NULL name means the symbol does not exist.
        if (rsym_info.name == NULL) {
            // rsym must be a integer literal
            INTEG_CHECK (rsym, r_len, &rval);
        }
        if (lsym_info.name == NULL) {
            // lsym is known to be a non-literal
            if (is_inc) {
                SYM_NOT_FOUND (lsym, l_len);
            }
            // assign
            if (rsym_info.name == NULL) {
                fprintf (out_file,
                    "mov r8, %ld\n"
                    "push r8\n", rval);
                rsym_info.size = 8;
                rsym_info.cap = 8;
                rsym_info.category = INT64;
            } else {
                if (rsym_info.category == INT64) {
                    fprintf (out_file,
                        "mov r9, rbp\n"
                        "sub r9, 0x%lx\n"
                        "mov r8, [r9]\n"
                        "push r8\n", rsym_info.offset);
                } else {
                    ret = gen_eq_expr (blk, stk, rsym_info, lsym_info, in_buf,
                        &rsym_size, out_file, msg);
                    if (ret) { goto done; }
                }
            }
            lsym_info.name = lsym;
            lsym_info.name_len = l_len;
            lsym_info.size = rsym_info.size;
            lsym_info.cap = rsym_info.cap;
            lsym_info.offset = (*frame_size) + WORDSZ_CEIL (rsym_info.size);
            lsym_info.category = rsym_info.category;

            ret = insert_symbol (&stk, lsym_info);
            (*frame_size) += WORDSZ_CEIL (lsym_info.size);
        } else {
            // set
            const char *int_upd_instr = (is_inc ? "add" : "mov");
            if (rsym_info.name == NULL) {
                TYPE_CHECK (lsym_info.category, INT64);
                fprintf (out_file,
                    "mov r8, %ld\n"
                    "mov r9, rbp\n"
                    "sub r9, 0x%lx\n"
                    "%s [r9], r8\n", rval, lsym_info.offset, int_upd_instr);
            } else {
                if (rsym_info.category == INT64) {
                    fprintf (out_file,
                        "mov r9, rbp\n"
                        "sub r9, 0x%lx\n"
                        "mov r8, [r9]\n"
                        "mov r9, rbp\n"
                        "sub r9, 0x%lx\n"
                        "%s [r9], r8\n", rsym_info.offset, lsym_info.offset, int_upd_instr);
                } else {
                    TYPE_CHECK (lsym_info.category, rsym_info.category);

                    ret = gen_eq_expr (blk, stk, rsym_info, lsym_info, in_buf,
                        NULL, out_file, msg);
                    if (ret) { goto done; }

                    lsym_new_info = lsym_info;
                    lsym_new_info.size = rsym_info.size;
                    set_symbol (&stk, lsym_info, lsym_new_info);
                }
            }
        }
    }
done:
    return ret;
}