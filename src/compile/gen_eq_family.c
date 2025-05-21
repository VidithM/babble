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
            TYPE_CHECK (lsym_info.category, EXPR_SYMCAT (blk.label));
        }
        rsym_info.name = NULL;
        ret = gen_eq_expr (blk, stk, rsym_info, lsym_info, in_buf, 
            &rsym_size, out_file, msg);
        
        if (ret) { goto done; }

        if (lsym_info.name == NULL) {
            lsym_info.name = lsym;
            lsym_info.name_len = l_len;
            lsym_info.size = rsym_size;
            lsym_info.offset = (*frame_size) + WORDSZ_CEIL (rsym_size);
            lsym_info.category = EXPR_SYMCAT (blk.label);
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

        if (lsym_info.name == NULL) {
            // lsym is known to be a non-literal
            if (is_inc) {
                SYM_NOT_FOUND (lsym, l_len);
            }
            // assign
            if (rsym_info.name == NULL) {
                ret = gen_eq_int (blk, rsym_info, lsym_info, in_buf,
                    out_file, msg);
                if (ret) { goto done; }
                rsym_info.size = 8;
                rsym_info.category = INT64;
            } else {
                if (rsym_info.category == INT64) {
                    ret = gen_eq_int (blk, rsym_info, lsym_info, in_buf,
                        out_file, msg);
                    if (ret) { goto done; }
                } else {
                    ret = gen_eq_expr (blk, stk, rsym_info, lsym_info, in_buf,
                        &rsym_size, out_file, msg);
                    if (ret) { goto done; }
                }
            }
            lsym_info.name = lsym;
            lsym_info.name_len = l_len;
            lsym_info.size = rsym_info.size;
            lsym_info.offset = (*frame_size) + WORDSZ_CEIL (rsym_info.size);
            lsym_info.category = rsym_info.category;

            ret = insert_symbol (&stk, lsym_info);
            (*frame_size) += WORDSZ_CEIL (lsym_info.size);
        } else {
            // set
            if (rsym_info.name == NULL) {
                ret = gen_eq_int (blk, rsym_info, lsym_info, in_buf,
                    out_file, msg);
                if (ret) { goto done; }
            } else {
                TYPE_CHECK (lsym_info.category, rsym_info.category);
                if (rsym_info.category == INT64) {
                    ret = gen_eq_int (blk, rsym_info, lsym_info, in_buf,
                        out_file, msg);
                } else {
                    ret = gen_eq_expr (blk, stk, rsym_info, lsym_info, in_buf,
                        NULL, out_file, msg);
                    if (ret) { goto done; }
                }
            }
        }
    }
done:
    return ret;
}