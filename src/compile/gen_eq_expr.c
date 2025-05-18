#include "intrinsics.h"
#include "codegen.h"
#include "compile-utils.h"

int gen_eq_expr (block blk, const symstack stk,
    symbol copy_from, symbol copy_to, // if copy_to, pop from the stack at the end
    char *in_buf, size_t *expr_size, FILE *out_file, char *msg) {
    
    int ret = BABBLE_OK;
    enum sym_category expr_type;

    if (copy_from.name == NULL) {
        expr_type = EXPR_SYMCAT (blk.label);
    } else {
        expr_type = copy_from.category;
    }

    char *tmp;
    
    if (copy_to.name != NULL && copy_from.name != NULL) {
        if (expr_size != NULL) {
            (*expr_size) = copy_from.size;
        }
        if (copy_from.size > copy_to.cap) {
            char *copy_to_name = in_buf + blk.start;
            char *copy_from_name = in_buf + blk.hotspots[0];

            TRUNCATE (copy_to_name, copy_to.name_len);
            TRUNCATE (copy_from_name, copy_from.name_len);

            BABBLE_MSG_COMPILE_ERR (blk.start_line,
                " (cannot copy expr %s of size %ld into expr %s of size %ld)\n",
                copy_from_name, copy_from.size, copy_to_name, copy_to.size);
            
            ret = BABBLE_COMPILE_ERR;
            goto done;
        }
        
        fprintf (out_file,
            "mov rdi, rbp\n"
            "sub rdi, 0x%lx\n"
            "mov rsi, rbp\n"
            "sub rsi, 0x%lx\n"
            "mov rdx, 0x%lx\n", copy_from.offset, copy_to.offset, copy_from.size);
        GET_INTRINSIC (&tmp, "memcpy");
        fprintf (out_file, "%s", tmp);

        return ret;
    }

    int start_line = blk.start_line;
    switch (expr_type) {
        case STRING:
            {
                size_t start, end;
                if (copy_from.name == NULL) {
                    start = blk.hotspots[3], end = blk.hotspots[4];
                    BABBLE_ASSERT (end > start);
                    size_t len = end - start;
                    if (expr_size != NULL) {
                        (*expr_size) = len;
                    }
                    if (copy_to.name == NULL) {
                        fprintf (out_file, "; line %d\n", start_line);
                        size_t sub_amt = WORDSZ_CEIL (len);
                        // Keep stack pointer aligned with word size
                        fprintf (out_file,
                            "sub rsp, 0x%lx\n", sub_amt - len);
                        // Make space for newline
                        fprintf (out_file, "sub rsp, 0x1\n");
                        for (size_t i = end - 1; i > start; i--) {
                            fprintf (out_file,
                                "sub rsp, 0x1\n"
                                "mov rdx, %d\n"
                                "mov [rsp], dl\n", in_buf[i]);
                        }
                    } else {
                        if (len > copy_to.cap) {
                            char *copy_to_name = in_buf + blk.start;
                            char tmp = copy_to_name[copy_to.name_len];
                            copy_to_name[copy_to.name_len] = '\0';

                            BABBLE_MSG_COMPILE_ERR (start_line,
                                " (attempt to overflow string expr %s of size %ld)\n",
                                copy_to_name, copy_to.size);

                            copy_to_name[copy_to.name_len] = tmp;
                            ret = BABBLE_COMPILE_ERR;
                            goto done;
                        }
                        BABBLE_ASSERT (copy_to.category == STRING);
                        fprintf (out_file,
                            "mov r9, rbp\n"
                            "sub r9, 0x%lx\n", copy_to.offset - len + 1);
                        for (size_t i = end - 1; i > start; i--) {
                            fprintf (out_file,
                                "sub r9, 0x1\n"
                                "mov rdx, %d\n"
                                "mov [r9], dl\n", in_buf[i]);
                        }
                    }
                } else {
                    BABBLE_ASSERT (copy_to.name == NULL);
                    if (expr_size != NULL) {
                        (*expr_size) = copy_from.size;
                    }
                    size_t sub_amt = WORDSZ_CEIL (copy_from.size);
                    fprintf (out_file,
                        "mov rdi, rbp\n"
                        "sub rdi, 0x%lx\n"
                        "sub rsp, 0x%lx\n"
                        "mov rsi, rsp\n"
                        "mov rdx, 0x%lx\n", copy_from.offset,
                        sub_amt, copy_from.size);
                    GET_INTRINSIC (&tmp, "memcpy");
                    fprintf (out_file, "%s", tmp);
                }
            }
            break;
        case BOOL:
            {
                size_t ltok_start, ltok_end, rtok_start, rtok_end;
                size_t lsym_len, rsym_len;
                char *lsym, *rsym;
                symbol lsym_info, rsym_info;
                int64_t lval, rval;

                (*expr_size) = 8;

                if (copy_from.name == NULL) {
                    ltok_start = blk.hotspots[3]; ltok_end = blk.hotspots[4];
                    rtok_start = blk.hotspots[5]; rtok_end = blk.hotspots[6];
                    lsym_len = ltok_end - ltok_start + 1;
                    rsym_len = rtok_end - rtok_start + 1;

                    lsym = in_buf + ltok_start;
                    rsym = in_buf + rtok_start;
                    int op = blk.label;
                    size_t expr_id = blk.start;
                    
                    find_symbol (&lsym_info, stk, lsym, lsym_len);
                    find_symbol (&rsym_info, stk, rsym, rsym_len);

                    if (lsym_info.name == NULL) {
                        INTEG_LIT_CHECK (lsym, lsym_len, &lval);
                        fprintf (out_file,
                            "mov r8, %ld\n", lval);
                    } else {
                        if ((lsym_info.category != BOOL) && 
                            (lsym_info.category != INT64)) {
                            TRUNCATE (lsym, lsym_len);
                            BABBLE_MSG_COMPILE_ERR (start_line,
                                " (cannot use variable \"%s\" of non-integral/bool"
                                " type in bool expr)\n", lsym);
                            ret = BABBLE_COMPILE_ERR;
                            goto done;
                        }
                        fprintf (out_file,
                            "mov r10, rbp\n"
                            "sub r10, 0x%lx\n"
                            "mov r8, [r10]\n", lsym_info.offset);
                    }
                    if (rsym_info.name == NULL) {
                        INTEG_LIT_CHECK (rsym, rsym_len, &rval);
                        fprintf (out_file,
                            "mov r9, %ld\n", rval);
                    } else {
                        if ((rsym_info.category != BOOL) && 
                            (rsym_info.category != INT64)) {
                            TRUNCATE (rsym, rsym_len);
                            BABBLE_MSG_COMPILE_ERR (start_line,
                                " (cannot use variable \"%s\" of non-integral/bool"
                                " type in bool expr)\n", rsym);
                            ret = BABBLE_COMPILE_ERR;
                            goto done;
                        }
                        fprintf (out_file,
                            "mov r10, rbp\n"
                            "sub r10, 0x%lx\n"
                            "mov r9, [r10]\n", rsym_info.offset);
                    }

                    switch (op) {
                        case EQ_BOOL_EXPR_SAME:
                            {
                                fprintf (out_file,
                                    "cmp r8, r9\n"
                                    "jz .bool_expr_%ld_true\n", expr_id);
                            }
                            break;
                        case EQ_BOOL_EXPR_LE:
                            {
                                fprintf (out_file,
                                    "cmp r8, r9\n"
                                    "js .bool_expr_%ld_true\n", expr_id);
                            }
                            break;
                        case EQ_BOOL_EXPR_OR:
                            {
                                fprintf (out_file,
                                    "or r8, r9\n"
                                    "cmp r8, 0\n"
                                    "jnz .bool_expr_%ld_true\n", expr_id);
                            }
                            break;
                        case EQ_BOOL_EXPR_AND:
                            {
                                fprintf (out_file,
                                    "cmp r8, 0\n"
                                    "jz .bool_expr_%ld_false\n"
                                    "cmp r9, 0\n"
                                    "jnz .bool_expr_%ld_true\n",
                                    expr_id, expr_id);
                            }
                            break;
                        default:
                            BABBLE_ASSERT (0);
                    }

                    fprintf (out_file,
                        ".bool_expr_%ld_false:\n"
                        "mov r8, 0\n"
                        "jmp .bool_expr_%ld_done\n"
                        ".bool_expr_%ld_true:\n"
                        "mov r8, 1\n"
                        ".bool_expr_%ld_done:\n",
                        expr_id, expr_id, expr_id, expr_id);

                    if (copy_to.name == NULL) {
                        fprintf (out_file, "push r8\n");
                    } else {
                        fprintf (out_file,
                            "mov r9, rbp\n"
                            "sub r9, 0x%lx\n"
                            "mov [r9], r8\n", copy_to.offset);
                    }
                } else {
                    BABBLE_ASSERT (copy_to.name == NULL);
                    ret = gen_eq_int (blk, copy_from, copy_to, in_buf,
                        out_file, msg);
                }
            }
            break;
        default:
            // The parse step should already validate the expr type
            BABBLE_ASSERT (0);
    }
done:
    return ret;
}