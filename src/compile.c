#include "babble-lang.h"
#include "parse.h"
#include "lex.h"
#include "symstack.h"
#include "intrinsics.h"

extern intrinsic_info intrinsics [2];

#define GET_INTRINSIC(_ret, _symbol)                        \
{                                                           \
   (*_ret) = NULL;                                          \
    for (int i = 0;                                         \
        i < sizeof (intrinsics) / sizeof (intrinsic_info);  \
        i++) {                                              \
                                                            \
        if (!strcmp (intrinsics[i].symbol, _symbol)) {      \
            (*_ret) = intrinsics[i].source;                 \
            break;                                          \
        }                                                   \
    }                                                       \
}

static void init (FILE *out_file) {
    fprintf (out_file,
        "; ==========================================================\n"
        "; Linux x86-64 Netwide assembly generated by Babble v%d.%d\n"
        "; ==========================================================\n\n",
        BABBLE_VER_MAJOR, BABBLE_VER_MINOR);
    
    // Initialize sections
    fprintf (out_file,
        "section .text\n"
        "global _start\n");
    
    int n_intrinsics = sizeof (intrinsics) / sizeof (intrinsic_info);
    // TODO: Put these in a separate `intrinsics.asm` file and include them in
    for (int i = 0; i < n_intrinsics; i++) {
        if (intrinsics[i].impl) {
            fprintf (out_file, "%s", intrinsics[i].source);
        }
    }
    fprintf (out_file,
        "_quit:\n"
        "mov rdi, 0\n"
        "mov rax, 60\n"
        "syscall\n");
    
    fprintf (out_file, "_start:\n");
}

static int assemble (int debug, const char *asm_name,
    const char *out_name, char *msg) {

    char cmd_buf [MSG_LEN];
    struct timeval ts;
    gettimeofday (&ts, NULL);
    snprintf (cmd_buf, MSG_LEN, "nasm -O0 -o %ld.o -f elf64%s %s", ts.tv_usec,
        (debug ? " -gdwarf" : ""), asm_name);
    
    int ret = system (cmd_buf);
    if (ret) {
        snprintf (msg, MSG_LEN, "Babble error: Assembler error "
            "(shell exit code %d). Make sure nasm 2.15+ is installed\n",
            ret);
        return BABBLE_ASSEMBLER_ERR;
    }

    snprintf (cmd_buf, MSG_LEN, "ld%s -o %s %ld.o", (debug ? " -g" : ""),
        out_name, ts.tv_usec);
    ret = system (cmd_buf);
    if (ret) {
        snprintf (msg, MSG_LEN, "Babble error: Linker error (shell exit code %d)\n",
            ret);
        return BABBLE_LINKER_ERR;
    }

    snprintf (cmd_buf, MSG_LEN, "rm -f %ld.o", ts.tv_usec);
    system (cmd_buf);

    return BABBLE_OK;
}

int compile (int debug, const char *in_name,
    const char *out_name, char *msg) {

    FILE *in_file = fopen (in_name, "r");
    if (in_file == NULL) {
        snprintf (msg, MSG_LEN, "Babble error: Input file \"%s\" not found\n",
            in_name);
        return BABBLE_FILE_NOT_FOUND;
    }
    int ret;
    char *in_buf;
    size_t in_buf_size;

    fseek (in_file, 0L, SEEK_END);
    in_buf_size = ftell (in_file);
    rewind (in_file);
    in_buf = (char*) malloc (in_buf_size);
    if (in_buf == NULL) {
        return BABBLE_MISC_ERR;
    }

    size_t at = 0;
    while (1) {
        char c = fgetc (in_file);
        if (feof (in_file)) {
            break;
        }
        in_buf[at] = c;
        at++;
    }
    fclose (in_file);

    // Remove comments
    size_t put = 0;
    int comment = 0;
    size_t skipped = 0;
    for (size_t i = 0; i < in_buf_size; i++) {
        if ((in_buf[i] == '\n') || (in_buf[i] == '\r')) {
            comment = 0;
        }
        if (in_buf[i] == '%') {
            comment = 1;
        }
        if (!comment) {
            in_buf[put] = in_buf[i];
            put++;
        } else {
            skipped++;
        }
    }
    in_buf_size -= skipped;

    // Lexical and syntax processing
    blocklist blist;
    ret = lex (in_buf, in_buf_size, &blist, msg);

    // dbg_blist ("blist", &blist);

    if (ret) {
        free (in_buf);
        free_blist (&blist);
        return ret;
    }

    

    // Semantic processing and codegen
    #if 0
    Rep:
        (1). Check if sym exists in scope, OR is literal
        (2) Push symstack w/ rep_id
        2. push rbp
        3. mov rbp, rsp
        4. rcx = rep amt
        5. create label .loop_{rep_id}
        6. Before loop body:
            6a. cmp rcx, 0
            6b. jz .loop_{rep_id}_break
        7. Gen loop body
        8. dec rax
    Print:
        (1). Check if sym exists in scope
        2. Call intrinsic
    
    At end:
        If symstack not at level 0, fail
    Symstack:
        - arr of char**
        - trie of current symbols in scope
            - trie node contains offset from rsp, value
    #endif

    symstack stk;

    ret = init_symstack (&stk);
    if (ret) {
        free (in_buf);
        free_blist (&blist);
        return ret;
    }

    struct timeval ts;
    gettimeofday (&ts, NULL);
    char asm_name [MSG_LEN];
    memset (asm_name, 0x0, sizeof (asm_name));
    snprintf (asm_name, MSG_LEN, "%ld.asm", ts.tv_usec);

    char cmd_buf [MSG_LEN];
    snprintf (cmd_buf, MSG_LEN, "rm -f %ld.asm", ts.tv_usec);
    system (cmd_buf);
    snprintf (cmd_buf, MSG_LEN, "touch %ld.asm", ts.tv_usec);
    system (cmd_buf);

    FILE *out_file = fopen (asm_name, "w");
    if (out_file == NULL) {
        free (in_buf);
        free_blist (&blist);
        free_symstack (&stk);
        return BABBLE_MISC_ERR;
    }

    init (out_file);

    fprintf (out_file,
        "push rbp\n"
        "mov rbp, rsp\n");

    size_t nxt_rep_id = 0;
    size_t frame_size = 0;
    for (size_t i = 0; i < blist.nblocks; i++) {
        size_t start, end;
        size_t hot[MAX_HOTSPOTS];

        start = blist.blocks[i].start;
        end = blist.blocks[i].end;

        switch (blist.blocks[i].label) {
            #define SYM_NOT_FOUND(_sym, _len)                                           \
            {                                                                           \
                _sym[_len] = '\0';                                                      \
                ret = BABBLE_COMPILE_ERR;                                               \
                snprintf (msg, MSG_LEN, "Babble error: Compile error on line %d"        \
                    " (variable \"%s\" is undefined)\n", blist.blocks[i].start_line,    \
                    _sym);                                                              \
                goto done;                                                              \
            }
            #define LIT_CHECK(_sym, _len, _val)                                             \
            {                                                                               \
                if (!valid_literal (_sym, 0, _len - 1)) {                                   \
                    SYM_NOT_FOUND (_sym, _len);                                             \
                }                                                                           \
                char tmp = _sym[_len];                                                      \
                _sym[_len] = '\0';                                                          \
                (*_val) = atoll(_sym);                                                      \
                _sym[_len] = tmp;                                                           \
            }
            case SCOPE_OPEN:
                {
                    size_t curr_rep_id;
                    get_curr_frame_rep_id (&curr_rep_id, &stk);

                    if (curr_rep_id != -1) {
                        fprintf (out_file,
                            "push rcx\n");
                        frame_size += 8;
                    }

                    ret = push_symstack_entry (&stk, -1, frame_size);
                    if (ret) { goto done; }
                }
                break;
            case SCOPE_CLOSE:
                {
                    size_t curr_rep_id;
                    get_curr_frame_rep_id (&curr_rep_id, &stk);

                    ret = pop_symstack_entry (&stk);
                    if (ret) {
                        snprintf (msg, MSG_LEN, "Babble error: Compile error on line %d"
                            " (scope imbalance)\n", blist.blocks[i].start_line);
                        ret = BABBLE_COMPILE_ERR;
                        goto done;
                    }

                    size_t prev_rep_id;
                    get_curr_frame_rep_id (&prev_rep_id, &stk);

                    size_t frame_bottom;
                    get_curr_frame_bottom (&frame_bottom, &stk);

                    if (curr_rep_id != -1) {
                        fprintf (out_file,
                            "dec rcx\n");
                    }
                    // TODO: Maintain prev rsp, num scopes in regs?
                    fprintf (out_file,
                        "mov rsp, rbp\n"
                        "sub rsp, %ld\n", frame_bottom);
                    frame_size = frame_bottom;

                    if (curr_rep_id != -1) {
                        fprintf (out_file,
                            "jmp .loop_%ld_body\n"
                            ".loop_%ld_break:\n", curr_rep_id, curr_rep_id);
                    }

                    if (prev_rep_id != -1) {
                        fprintf (out_file,
                            "pop rcx\n");
                        frame_size -= 8;
                    }
                }
                break;
            case INC:
            case EQ:
                {
                    int is_inc = (blist.blocks[i].label == INC);
                    hot[0] = blist.blocks[i].hotspots[0];
                    hot[1] = blist.blocks[i].hotspots[1];
                    hot[2] = blist.blocks[i].hotspots[2];
                    
                    char *lsym = in_buf + start;
                    char *rsym = in_buf + hot[1];
                    size_t l_len = hot[0] - start + 1;
                    size_t r_len = hot[2] - hot[1] + 1;
                    size_t l_offset, r_offset;

                    find_symbol (&l_offset, &stk, lsym, l_len);
                    find_symbol (&r_offset, &stk, rsym, r_len);

                    int64_t rval;
                    if (r_offset == -1) {
                        // rsym must be a literal
                        LIT_CHECK (rsym, r_len, &rval);
                    }
                    if (l_offset == -1) {
                        // lsym is known to be a non-literal
                        if (is_inc) {
                            SYM_NOT_FOUND (lsym, l_len);
                        }
                        // assign
                        if (r_offset == -1) {
                            fprintf (out_file,
                                "mov r8, %ld\n"
                                "push r8\n", rval);
                        } else {
                            fprintf (out_file,
                                "mov r9, rbp\n"
                                "sub r9, %ld\n"
                                "mov r8, [r9]\n"
                                "push r8\n", r_offset);    
                        }
                        frame_size += 8;
                        ret = insert_symbol (&stk, lsym, l_len, frame_size);
                        if (ret) { goto done; }
                    } else {
                        // set
                        const char *upd_instr = (is_inc ? "add" : "mov");
                        if (r_offset == -1) {
                            fprintf (out_file,
                                "mov r8, %ld\n"
                                "mov r9, rbp\n"
                                "sub r9, %ld\n"
                                "%s [r9], r8\n", rval, l_offset, upd_instr);
                        } else {
                            fprintf (out_file,
                                "mov r9, rbp\n"
                                "sub r9, %ld\n"
                                "mov r8, [r9]\n"
                                "mov r9, rbp\n"
                                "sub r9, %ld\n"
                                "%s [r9], r8\n", r_offset, l_offset, upd_instr);
                        }
                    }
                }
                break;
            case REP:
                {
                    size_t curr_rep_id;
                    get_curr_frame_rep_id (&curr_rep_id, &stk);
                    if (curr_rep_id != -1) {
                        fprintf (out_file,
                            "push rcx\n");
                        frame_size += 8;
                    }
                    
                    ret = push_symstack_entry (&stk, nxt_rep_id, frame_size);
                    if (ret) { goto done; }

                    hot[0] = blist.blocks[i].hotspots[0];
                    hot[1] = blist.blocks[i].hotspots[1];

                    char *sym = in_buf + hot[0];
                    size_t len = hot[1] - hot[0] + 1;
                    size_t offset;

                    find_symbol (&offset, &stk, sym, len);

                    int64_t val;
                    if (offset == -1) {
                        LIT_CHECK (sym, len, &val);
                        fprintf (out_file,
                            "mov rcx, %ld\n", val);
                    } else {
                        fprintf (out_file,
                            "mov r9, rbp\n"
                            "sub r9, %ld\n"
                            "mov rcx, [r9]\n", offset);   
                    }

                    fprintf (out_file,
                        ".loop_%ld_body:\n"
                        "cmp rcx, 0\n"
                        "jz .loop_%ld_break\n", nxt_rep_id, nxt_rep_id);
                    
                    nxt_rep_id++;
                }
                break;
            case PRINT:
                {
                    hot[0] = blist.blocks[i].hotspots[0];
                    hot[1] = blist.blocks[i].hotspots[1];
                    char *sym = in_buf + hot[0];
                    size_t len = hot[1] - hot[0] + 1;
                    size_t offset;
                    find_symbol (&offset, &stk, sym, len);

                    int64_t val;
                    if (offset == -1) {
                        LIT_CHECK (sym, len, &val);
                        fprintf (out_file,
                            "mov rdi, %ld\n", val);
                    } else {
                        fprintf (out_file,
                            "mov r9, rbp\n"
                            "sub r9, %ld\n"
                            "mov rdi, [r9]\n", offset);
                    }
                    char *tmp;
                    GET_INTRINSIC (&tmp, "print_i64");
                    fprintf (out_file, "%s", tmp);
                }
                break;
        }
    }
done:
    if ((!ret) && (stk.nscopes > 1)) {
        snprintf (msg, MSG_LEN, "Babble error: Compile error (scope imbalance)\n");
        ret = BABBLE_COMPILE_ERR;
    }
    fprintf (out_file,
        "mov rsp, rbp\n"
        "pop rbp\n"
        "call _quit\n");

    free_symstack (&stk);
    free_blist (&blist);
    free (in_buf);
    fclose (out_file);

    if (ret) {
        return ret;
    }

    // #if 0
    ret = assemble (debug, asm_name, out_name, msg);
    // #endif
    #ifndef DEBUG
    snprintf (cmd_buf, MSG_LEN, "rm -f %ld.asm", ts.tv_usec);
    system (cmd_buf);
    #endif
    return ret;
}