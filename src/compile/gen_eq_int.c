#include "codegen.h"
#include "compile-utils.h"

int gen_eq_int (block blk, symbol copy_from, symbol copy_to, 
    char *in_buf, FILE *out_file, char *msg) {
    
    BABBLE_ASSERT ((blk.label == EQ) || (blk.label == INC));
    BABBLE_ASSERT_IMPLIES ((copy_from.name != NULL), copy_from.category == INT64);
    BABBLE_ASSERT_IMPLIES ((copy_to.name != NULL), copy_to.category == INT64);

    int ret = BABBLE_OK;

    size_t start; int start_line;
    char *lsym, *rsym;
    size_t l_len, r_len;
    int64_t rval;
    
    start = blk.start; start_line = blk.start_line;

    l_len = blk.hotspots[0] - start + 1;
    r_len = blk.hotspots[2] - blk.hotspots[1] + 1;
    lsym = in_buf + start;
    rsym = in_buf + blk.hotspots[1];

    if (copy_from.name == NULL) {
        INTEG_LIT_CHECK (rsym, r_len, &rval);
    }

    if ((copy_to.name == NULL) && (blk.label == INC)) {
        SYM_NOT_FOUND (lsym, l_len);
    }

    if (copy_to.name != NULL) {
        TYPE_CHECK (INT64, copy_to.category);
    }

    if (copy_to.name == NULL) {
        if (copy_from.name == NULL) {
            fprintf (out_file,
                "mov r8, %ld\n"
                "push r8\n", rval);
        } else {
            fprintf (out_file,
                "mov r9, rbp\n"
                "sub r9, 0x%lx\n"
                "mov r8, [r9]\n"
                "push r8\n", copy_from.offset);
        }
    } else {
        char *int_upd_str = ((blk.label == INC) ? "add" : "mov");
        if (copy_from.name == NULL) {
            fprintf (out_file,
                "mov r9, rbp\n"
                "sub r9, 0x%lx\n"
                "mov r8, %ld\n"
                "%s [r9], r8\n", copy_to.offset, rval, int_upd_str);
        } else {
            fprintf (out_file,
                "mov r9, rbp\n"
                "sub r9, 0x%lx\n"
                "mov r8, [r9]\n"
                "mov r9, rbp\n"
                "sub r9, 0x%lx\n"
                "%s [r9], r8\n", copy_from.offset, copy_to.offset,
                int_upd_str);
        }
    }

done:
    return ret;
}