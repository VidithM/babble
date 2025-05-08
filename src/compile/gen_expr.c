#include "intrinsics.h"

#include "codegen.h"
#include "compile-utils.h"

// Gen code to push a literal expr onto the stack
// In the future, when dealing with bool exprs, can use a separate static gen_bexpr, maybe with
// recursive evaluation
int gen_expr (block blk, symstack stk,
    symbol copy_from, symbol copy_to, // if copy_to, pop from the stack at the end
    char *in_buf, size_t *expr_size, FILE *out_file, char *msg) {
    
    int ret = BABBLE_OK;
    enum sym_category expr_type;

    if (copy_from.name == NULL) {
        expr_type = blk.label - EQ;
    } else {
        expr_type = copy_from.category;
    }

    char *tmp;
    
    if (copy_to.name != NULL && copy_from.name != NULL) {
        if (copy_from.size > copy_to.size) {
            char tmp1, tmp2;
            char *copy_to_name = in_buf + blk.start;
            char *copy_from_name = in_buf + blk.hotspots[1];
            tmp1 = copy_to_name[copy_to.name_len];
            tmp2 = copy_from_name[copy_from.name_len];
            copy_to_name[copy_to.name_len] = copy_from_name[copy_from.name_len] = '\0';

            BABBLE_MSG_COMPILE_ERR (blk.start_line,
                " (cannot copy expr %s of size %ld into expr %s of size %ld)\n",
                copy_from_name, copy_from.size, copy_to_name, copy_to.size);
            
            copy_to_name[copy_to.name_len] = tmp1;
            copy_from_name[copy_from.name_len] = tmp2;
            ret = BABBLE_COMPILE_ERR;
            goto done;
        }
        fprintf (out_file,
            "mov rdi, 0x%lx\n"
            "mov rsi, 0x%lx\n"
            "mov rdi, %ld\n", copy_to.offset, copy_from.offset, copy_from.size);
        GET_INTRINSIC (&tmp, "memcpy");
        fprintf (out_file, "%s", tmp);
    }

    size_t start, end;
    switch (expr_type) {
        case STRING:
            {
                if (copy_from.name == NULL) {
                    start = blk.hotspots[3], end = blk.hotspots[4];
                    BABBLE_ASSERT (end > start);
                    size_t len = end - start;
                    (*expr_size) = len;
                    if (copy_to.name == NULL) {
                        size_t sub_amt = WORDSZ_CEIL (len);
                        // Keep stack pointer aligned with word size
                        fprintf (out_file,
                            "sub rsp, %ld\n", sub_amt - len);
                        // Make space for newline
                        fprintf (out_file, "sub rsp, 0x1\n");
                        for (size_t i = end - 1; i > start; i--) {
                            fprintf (out_file,
                                "sub rsp, 0x1\n"
                                "mov rdx, %d\n"
                                "mov [rsp], dl\n", in_buf[i]);
                        }
                    } else {
                        if (len > copy_to.size) {
                            char *copy_to_name = in_buf + blk.start;
                            char tmp = copy_to_name[copy_to.name_len];
                            copy_to_name[copy_to.name_len] = '\0';

                            BABBLE_MSG_COMPILE_ERR (blk.start_line,
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
                        for (size_t i = end; i > start; i--) {
                            fprintf (out_file,
                                "sub r9, 0x1\n"
                                "mov rdx, %d\n"
                                "mov [r9], dl\n", in_buf[i]);
                        }
                    }
                } else {
                    BABBLE_ASSERT (copy_to.name == NULL);
                    (*expr_size) = copy_from.size;
                    size_t sub_amt = WORDSZ_CEIL (copy_from.size);
                    fprintf (out_file,
                        "mov rdi, rbp\n"
                        "sub rdi, 0x%lx\n"
                        "sub rsp, %ld\n"
                        "mov rsi, rsp\n"
                        "mov rdx, 0x%lx\n", copy_from.offset,
                        sub_amt, copy_from.size);
                    GET_INTRINSIC (&tmp, "memcpy");
                    fprintf (out_file, "%s", tmp);
                }
            }
            break;
        default:
            // The parse step should already validate the expr type
            BABBLE_BRKPT;
            BABBLE_ASSERT (0);
    }
done:
    return ret;
}