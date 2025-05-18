#include "codegen.h"
#include "compile-utils.h"

#include "intrinsics.h"

int gen_print (block blk, const symstack stk, char *in_buf,
    FILE *out_file, char *msg) {
    
    int ret = BABBLE_OK;
    size_t start = blk.start;
    int start_line = blk.start_line;

    char *sym = in_buf + blk.hotspots[0];
    size_t len = blk.hotspots[1] - blk.hotspots[0] + 1;
    symbol sym_info;
    find_symbol (&sym_info, stk, sym, len);

    int64_t val;
    char *tmp;
    if (sym_info.name == NULL) {
        INTEG_LIT_CHECK (sym, len, &val);
        fprintf (out_file,
            "mov rdi, %ld\n", val);
        GET_INTRINSIC (&tmp, "print_i64");
        fprintf (out_file, "%s", tmp);
    } else {
        if (sym_info.category == STRING) {
            fprintf (out_file,
                "mov r9, rbp\n"
                "sub r9, 0x%lx\n"
                "mov rdx, %d\n"
                "mov [r9], dl\n"
                "sub r9, 0x%lx\n", sym_info.offset - sym_info.size + 1,
                '\n', sym_info.size);
            
            fprintf (out_file,
                "push rax\n"
                "push rdi\n"
                "push rcx\n"
                "mov rdi, 1\n"
                "mov rsi, r9\n"
                "mov rdx, 0x%lx\n"
                "mov rax, 1\n"
                "syscall\n"
                "pop rcx\n"
                "pop rdi\n"
                "pop rax\n", sym_info.size + 1);
        } else {
            BABBLE_ASSERT (INTEG_TYPE_CHECK (sym_info));
            fprintf (out_file,
                "mov r9, rbp\n"
                "sub r9, 0x%lx\n"
                "mov rdi, [r9]\n", sym_info.offset);

            GET_INTRINSIC (&tmp, "print_i64");
            fprintf (out_file, "%s", tmp);
        }
    }
done:
    return ret;
}