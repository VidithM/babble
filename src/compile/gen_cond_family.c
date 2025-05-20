#include "codegen.h"
#include "compile-utils.h"

static int nxt_wrep_id = 0, nxt_cond_id = 0;

int gen_cond_family (block blk, symstack stk, char *in_buf, size_t *frame_size,
    FILE *out_file, char *msg) {
    
    int is_wrep = (blk.label == WREP);
    size_t start = blk.start; int start_line = blk.start_line;
    size_t h1 = blk.hotspots[0], h2 = blk.hotspots[1];
    size_t sym_len = h2 - h1 + 1;
    int ret = BABBLE_OK;
    
    int curr_rep_id = -1; enum block_label ctrl_type;
    get_curr_frame_control_id (&ctrl_type, &curr_rep_id, stk);
    if (ctrl_type != REP) { curr_rep_id = -1; }

    if (curr_rep_id != -1) {
        (*frame_size) += 8;
        fprintf (out_file, "push rcx\n");
    }

    symbol sym_info;
    char *sym; size_t sym_val;
    sym = in_buf + h1;

    find_symbol (&sym_info, stk, sym, sym_len);

    if (sym_info.name == NULL) {
        INTEG_LIT_CHECK (sym, sym_len, &sym_val);
    } else {
        if (!INTEG_TYPE_CHECK (sym_info)) {
            TRUNCATE (sym, sym_info.name_len);
            BABBLE_MSG_COMPILE_ERR (start_line,
                " (cannot use symbol \"%s\" of non-integral/bool type in wrep)\n", sym);
            ret = BABBLE_COMPILE_ERR;
            goto done;
        }
    }
    
    size_t frame_bottom;
    get_curr_frame_bottom (&frame_bottom, stk);

    ret = push_symstack_entry (&stk, (is_wrep ? WREP : COND),
        (is_wrep ? nxt_wrep_id : nxt_cond_id), frame_bottom);
    if (ret) { goto done; }
    
    if (is_wrep) {
        fprintf (out_file, ".wrep_%d_body:\n", nxt_wrep_id);
    }

    if (sym_info.name == NULL) {
        fprintf (out_file, "mov r8, %ld\n", sym_val);
    } else {
        fprintf (out_file,
            "mov r9, rbp\n"
            "sub r9, 0x%lx\n"
            "mov r8, [r9]\n", sym_info.offset);
    }

    fprintf (out_file, "cmp r8, 0\n");
    if (is_wrep) {
        fprintf (out_file, "jz .wrep_%d_break\n", nxt_wrep_id);
    } else {
        fprintf (out_file, "jz .cond_%d_done\n", nxt_cond_id);
    }

    nxt_wrep_id += is_wrep;
    nxt_cond_id += !is_wrep;

done:
    return ret;
}