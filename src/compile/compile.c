#include "babble-lang.h"
#include "intrinsics.h"
#include "parse.h"
#include "lex.h"
#include "symstack.h"

#include "compile-utils.h"
#include "codegen.h"

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

    // TODO: Put these in a separate intrinsics.asm file and include them in
    BABBLE_ASSERT (N_INTRINSICS <= MAX_INTRINSICS);
    for (int i = 0; i < N_INTRINSICS; i++) {
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

    char _obj_name [MSG_LEN]; char *obj_name = _obj_name; // Silence -Wformat-truncation
    snprintf (obj_name, MSG_LEN, "/tmp/babble_%d.%d_%ld.o", BABBLE_VER_MAJOR,
        BABBLE_VER_MINOR, ts.tv_usec);

    snprintf (cmd_buf, MSG_LEN, "nasm -O0 -o %s -f elf64%s %s", obj_name,
        (debug ? " -gdwarf" : ""), asm_name);
    
    int ret = system (cmd_buf);
    if (ret) {
        snprintf (msg, MSG_LEN, "Babble error: Assembler error "
            "(shell exit code %d). Make sure nasm 2.15+ is installed\n",
            ret);
        return BABBLE_ASSEMBLER_ERR;
    }

    snprintf (cmd_buf, MSG_LEN, "ld%s -o %s %s", (debug ? " -g" : ""),
        out_name, obj_name);
    ret = system (cmd_buf);
    if (ret) {
        snprintf (msg, MSG_LEN, "Babble error: Linker error (shell exit code %d)\n",
            ret);
        return BABBLE_LINKER_ERR;
    }

    #ifndef DEBUG
    snprintf (cmd_buf, MSG_LEN, "rm -f %s", obj_name);
    system (cmd_buf);
    #endif

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

    if (ret) {
        free (in_buf);
        return ret;
    }

    symstack stk;

    ret = init_symstack (&stk);
    if (ret) {
        free (in_buf);
        free_blist (&blist);
        return ret;
    }

    struct timeval ts;
    gettimeofday (&ts, NULL);
    char _asm_name [MSG_LEN]; char *asm_name = _asm_name; // Silence -Wformat-truncation
    memset (asm_name, 0x0, sizeof (asm_name));
    snprintf (asm_name, MSG_LEN, "/tmp/babble_%d.%d_%ld.asm", BABBLE_VER_MAJOR, 
        BABBLE_VER_MINOR, ts.tv_usec);

    char cmd_buf [MSG_LEN];
    snprintf (cmd_buf, MSG_LEN, "rm -f %s", asm_name);
    system (cmd_buf);
    snprintf (cmd_buf, MSG_LEN, "touch %s", asm_name);
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
    
    enum block_label ctrl_type;
    int curr_rep_id = -1, curr_cond_id = -1, curr_wrep_id = -1, nxt_rep_id = 0;
    size_t frame_size = 0;
    size_t nscopes;
    for (size_t i = 0; i < blist.nblocks; i++) {
        size_t start, end;
        int start_line;
        size_t hot[MAX_HOTSPOTS];

        start_line = blist.blocks[i].start_line;
        start = blist.blocks[i].start;
        end = blist.blocks[i].end;

        switch (blist.blocks[i].label) {
            case SCOPE_OPEN:
                {
                    get_curr_frame_control_id (&ctrl_type, &curr_rep_id, stk);
                    if (ctrl_type != REP) { curr_rep_id = -1; }

                    if (curr_rep_id != -1) {
                        fprintf (out_file, "push rcx\n");
                        frame_size += 8;
                    }

                    ret = push_symstack_entry (&stk, -1, -1, frame_size);
                    if (ret) { goto done; }
                }
                break;
            case SCOPE_CLOSE:
                {
                    curr_rep_id = curr_wrep_id = curr_cond_id = -1;
                    get_curr_frame_control_id (&ctrl_type, NULL, stk);
                    
                    switch (ctrl_type) {
                        case REP:
                            get_curr_frame_control_id (NULL, &curr_rep_id, stk);
                            break;
                        case WREP:
                            get_curr_frame_control_id (NULL, &curr_wrep_id, stk);
                            break;
                        case COND:
                            get_curr_frame_control_id (NULL, &curr_cond_id, stk);
                    }

                    ret = pop_symstack_entry (&stk);
                    if (ret) {
                        BABBLE_MSG_COMPILE_ERR (start_line,
                            " (scope imbalance)\n");
                        ret = BABBLE_COMPILE_ERR;
                        goto done;
                    }

                    int prev_rep_id = -1;
                    get_curr_frame_control_id (&ctrl_type, &prev_rep_id, stk);
                    if (ctrl_type != REP) { prev_rep_id = -1; }

                    size_t frame_bottom;
                    get_curr_frame_bottom (&frame_bottom, stk);

                    if (curr_rep_id != -1) {
                        fprintf (out_file,
                            "dec rcx\n");
                    }
                    // TODO: Maintain prev rsp, num scopes in regs?
                    fprintf (out_file,
                        "mov rsp, rbp\n"
                        "sub rsp, 0x%lx\n", frame_bottom);
                    frame_size = frame_bottom;
                    
                    if (curr_rep_id != -1) {
                        fprintf (out_file,
                            "jmp .rep_%d_body\n"
                            ".rep_%d_break:\n", curr_rep_id, curr_rep_id);
                    }
                    if (curr_wrep_id != -1) {
                        fprintf (out_file,
                            "jmp .wrep_%d_body\n"
                            ".wrep_%d_break:\n", curr_wrep_id, curr_wrep_id);
                    }
                    if (curr_cond_id != -1) {
                        fprintf (out_file,
                            ".cond_%d_done:\n", curr_cond_id);
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
            case EQ_STR_EXPR:
            case EQ_BOOL_EXPR_SAME:
            case EQ_BOOL_EXPR_LE:
            case EQ_BOOL_EXPR_OR:
            case EQ_BOOL_EXPR_AND:
                {
                    ret = gen_eq_family (blist.blocks[i], stk, in_buf, &frame_size,
                        /* should this be &out_file? */ out_file, msg);
                    if (ret) { goto done; }
                }
                break;
            case REP:
                {
                    curr_rep_id = -1;
                    get_curr_frame_control_id (&ctrl_type, &curr_rep_id, stk);
                    if (ctrl_type != REP) { curr_rep_id = -1; }

                    if (curr_rep_id != -1) {
                        fprintf (out_file, "push rcx\n");
                        frame_size += 8;
                    }

                    ret = push_symstack_entry (&stk, REP, nxt_rep_id, frame_size);
                    if (ret) { goto done; }

                    hot[0] = blist.blocks[i].hotspots[0];
                    hot[1] = blist.blocks[i].hotspots[1];

                    char *sym = in_buf + hot[0];
                    size_t len = hot[1] - hot[0] + 1;
                    symbol sym_info;

                    find_symbol (&sym_info, stk, sym, len);

                    int64_t val;
                    if (sym_info.name == NULL) {
                        INTEG_LIT_CHECK (sym, len, &val);
                        fprintf (out_file,
                            "mov rcx, %ld\n", val);
                    } else {
                        if (sym_info.category != INT64) {
                            BABBLE_MSG_COMPILE_ERR (start_line, " (\"rep\" cannot accept a "
                                "non-integer argument)\n");
                            ret = BABBLE_COMPILE_ERR;
                            goto done;
                        }
                        fprintf (out_file,
                            "mov r9, rbp\n"
                            "sub r9, 0x%lx\n"
                            "mov rcx, [r9]\n", sym_info.offset);   
                    }

                    fprintf (out_file,
                        ".rep_%d_body:\n"
                        "cmp rcx, 0\n"
                        "jz .rep_%d_break\n", nxt_rep_id, nxt_rep_id);
                    
                    nxt_rep_id++;
                }
                break;
            case COND:
            case WREP:
                {
                    ret = gen_cond_family (blist.blocks[i], stk, in_buf, &frame_size,
                        out_file, msg);
                    if (ret) { goto done; }
                }
                break;
            case PRINT:
                {
                    ret = gen_print (blist.blocks[i], stk, in_buf, out_file, msg);
                    if (ret) { goto done; }
                }
                break;
            default:
                BABBLE_ASSERT (0);
        }
    }
done:
    get_nscopes (&nscopes, stk);
    if ((!ret) && (nscopes > 1)) {
        BABBLE_MSG_COMPILE_ERR (-1, " (scope imbalance)\n");
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
    snprintf (cmd_buf, MSG_LEN, "rm -f %s", asm_name);
    system (cmd_buf);
    #endif
    return ret;
}