#include "lex.h"
#include "parse.h"

static int init_blist (blocklist *blist, size_t cap) {

    BABBLE_ASSERT (blist != NULL);

    blist->nblocks = 0;
    blist->cap = cap; 
    blist->blocks = (block*) malloc (cap * sizeof (block));
    if (blist->blocks == NULL) {
        return BABBLE_MISC_ERR;
    }
    return BABBLE_OK;
}

static int resize_blist (blocklist *blist) {

    BABBLE_ASSERT (blist != NULL);

    blist->cap *= 2;
    blist->blocks = (block *) realloc (blist->blocks,
        blist->cap * sizeof (block));
    if (blist->blocks == NULL) {
        return BABBLE_MISC_ERR;
    }
    return BABBLE_OK;
}

static int push_block (blocklist *blist,
    size_t start, size_t end,
    int start_line,
    enum block_label label) {

    BABBLE_ASSERT (blist != NULL);
    
    int ret = BABBLE_OK;
    if (blist->nblocks == blist->cap) {
        ret = resize_blist (blist);
        if (ret) {
            return ret;
        }
    }
    block put;
    memset (&put, 0x0, sizeof (block));
    put.start = start; put.end = end;
    put.start_line = start_line; put.label = label;
    blist->blocks[blist->nblocks] = put;
    blist->nblocks++;
    return ret;
}

#ifdef DEBUG
static char* LABEL_NAMES[] = {"UNKNOWN", "EMPTY", "INC",
    "EQ", "EQ_STR_EXPR", "REP", "PRINT", "SCOPE_OPEN", "SCOPE_CLOSE"};

void dbg_blist (const char *name, blocklist blist) {
    printf ("==== DEBUG BLOCKLIST ====\n");
    printf ("name: %s\n", name);
    printf ("nblocks: %ld\n", blist.nblocks);
    printf ("cap: %ld\n\n", blist.cap);
    for (size_t i = 0; i < blist.nblocks; i++) {
        printf ("Block: %ld\n", i);
        char *label_name = LABEL_NAMES[blist.blocks[i].label];
        printf ("\tLabel: %d (%s)\n", blist.blocks[i].label, label_name);
        printf ("\tStart: %ld\n", blist.blocks[i].start);
        printf ("\tEnd: %ld\n", blist.blocks[i].end);
        printf ("\tStart line: %d\n", blist.blocks[i].start_line);
        printf ("\tHotspots: ");
        for (size_t j = 0; j < MAX_HOTSPOTS; j++) {
            printf ("%02ld ", blist.blocks[i].hotspots[j]);
        }
        printf ("\n");
    }
    printf ("========\n");
}
#else
void dbg_blist (const char *name, blocklist blist) {}
#endif

void free_blist (blocklist *blist) {
    free (blist->blocks);
    memset (blist, 0x0, sizeof (blocklist));
}

int lex (char *in_buf, size_t buf_size, blocklist *blist, char *msg) {
    int ret = BABBLE_OK;
    blocklist blist_phase2;
    memset (&blist_phase2, 0x0, sizeof (blocklist));

    if (init_blist (blist, DEFAULT_CAP)) {
        return BABBLE_MISC_ERR;
    }

    size_t start = 0, end;
    int start_line = 1;
    int line = 1;
    for (size_t i = 0; i < buf_size; i++) {
        if (in_buf[i] == ';') {
            if (push_block (blist, start, i, start_line, UNKNOWN)) {
                ret = BABBLE_MISC_ERR;
                goto done;
            }
            start = i + 1;
            start_line = line;
        }
        if (in_buf[i] == '\r' || in_buf[i] == '\n') {
            line++;
        }
    }

    if (start < buf_size) {
        // implicit ';' at the end
        if (push_block (blist, start, buf_size, start_line, UNKNOWN)) {
            ret = BABBLE_MISC_ERR;
            goto done;
        }
    }

    if (init_blist (&blist_phase2, blist->cap)) {
        ret = BABBLE_MISC_ERR;
        goto done;
    }

    for (size_t i = 0; i < blist->nblocks; i++) {
        line = blist->blocks[i].start_line;
        start = blist->blocks[i].start;
        end = blist->blocks[i].end;
        size_t curr = start;
        while (1) {
            // jump to next non-space
            start = find_next (in_buf, start, blist->blocks[i].end);
            if (start >= end) {
                break;
            }

            while (curr < start) {
                line += (in_buf[curr] == '\r' || in_buf[curr] == '\n');
                curr++;
            }
            // try to consume a block
            // =, +=, print are terminal blocks (must end with ;)
            // REP, {, } are non-terminal

            // Try to consume REP
            size_t match_res;
            if (match (in_buf, start, end, "rep", 3)) {
                // next should be a '('
                size_t h1, h2;
                match_res = find_next (in_buf, start + 3, end);
                // If fails, must be a terminal
                if (match_res == -1) { goto terminal; }
                if (in_buf[match_res] != '(') { goto terminal; }
                h1 = match_res;

                // Must find a ')', else fail
                match_res = find_next_pat (in_buf, match_res + 1, end, ")", 1);
                if (match_res == -1) { break; }
                h2 = match_res;

                // Next should be a '{', else fail
                match_res = find_next (in_buf, match_res + 1, end);;
                if (match_res == -1) { break; }
                if (in_buf[match_res] != '{') { break; }

                push_block (&blist_phase2,
                    start,
                    match_res,
                    line,
                    REP);
                blist_phase2.blocks[blist_phase2.nblocks - 1].hotspots[0] = h1;
                blist_phase2.blocks[blist_phase2.nblocks - 1].hotspots[1] = h2;

                start = match_res + 1;
                continue;
            } else if (match (in_buf, start, end, "{", 1)) {
                push_block (&blist_phase2,
                    start,
                    start,
                    line,
                    SCOPE_OPEN);
                start++;
                continue;
            } else if (match (in_buf, start, end, "}", 1)) {
                push_block (&blist_phase2,
                    start,
                    start,
                    line,
                    SCOPE_CLOSE);
                start++;
                continue;
            }
        terminal:
            if (in_buf[end] != ';') {
                break;
            }
            if ((match_res = find_next_pat (in_buf, start, end, "+=", 2)) != -1) {
                push_block (&blist_phase2,
                    start,
                    end,
                    line,
                    INC);
                blist_phase2.blocks[blist_phase2.nblocks - 1].hotspots[0] = match_res;
                start = end;
            } else if ((match_res = find_next_pat (in_buf, start, end, "=", 1)) != -1) {
                push_block (&blist_phase2,
                    start,
                    end,
                    line,
                    EQ);
                blist_phase2.blocks[blist_phase2.nblocks - 1].hotspots[0] = match_res;
                start = end;
            } else if (match (in_buf, start, end, "print", 5)) {
                BABBLE_BRKPT;
                // next should be a '('
                size_t h1, h2;
                match_res = find_next (in_buf, start + 5, end);
                if (match_res == -1) { break; }
                if (in_buf[match_res] != '(') { break; }
                h1 = match_res;
                // must find a ')'
                match_res = find_next_pat (in_buf, match_res + 1, end, ")", 1);
                if (match_res == -1) { break; }
                h2 = match_res;
                // next must be ';'
                match_res = find_next (in_buf, match_res + 1, end);
                if (match_res == -1) { break; }
                if (in_buf[match_res] != ';') { break; }

                push_block (&blist_phase2,
                    start,
                    end,
                    line,
                    PRINT);
                blist_phase2.blocks[blist_phase2.nblocks - 1].hotspots[0] = h1;
                blist_phase2.blocks[blist_phase2.nblocks - 1].hotspots[1] = h2;
                start = end;
            } else {
                break;
            }
        }
        if (start < end) {
            BABBLE_MSG_COMPILE_ERR (line, "\n");
            ret = BABBLE_COMPILE_ERR;
            goto done;
        }
    }

    free_blist (blist);
    blist->blocks = blist_phase2.blocks;
    blist->nblocks = blist_phase2.nblocks;
    blist->cap = blist_phase2.cap;

    blist_phase2.blocks = NULL;

    // Fine validation of blist:
    // ensure arg formats are right (alphanumeric, cannot start with num)
    for (size_t i = 0; i < blist->nblocks; i++) {
        int failed = 0;
        int generic_msg = 1;
        int is_expr, block_label_offset;
        start_line = blist->blocks[i].start_line;
        start = blist->blocks[i].start;
        end = blist->blocks[i].end;

        if (blist->blocks[i].label == INC || blist->blocks[i].label == EQ) {
            size_t h1 = blist->blocks[i].hotspots[0];
            if (h1 == 0) {
                failed = 1;
                goto fail;
            }

            size_t ltok_end = find_prev (in_buf, start, h1 - 1);
            if (ltok_end == -1) {
                failed = 1;
                goto fail;
            }

            blist->blocks[i].hotspots[0] = ltok_end;

            if (!valid_symbol (in_buf, start, ltok_end)) {
                failed = 1;
                goto fail;
            }

            size_t rtok_start = find_next (in_buf,
                h1 + (blist->blocks[i].label == INC) + 1, end);

            // cannot include ';'
            if (rtok_start >= end) {
                failed = 1;
                goto fail;
            }
            blist->blocks[i].hotspots[1] = rtok_start;

            size_t rtok_end = end;
            rtok_end = find_prev (in_buf, rtok_start, end - 1);
            if (rtok_end == -1) {
                failed = 1;
                goto fail;
            }

            block_label_offset = 0;
            is_expr = valid_expr_full (in_buf, rtok_start, end, 1, blist->blocks[i].hotspots + 2,
                &block_label_offset);

            if (!(valid_symbol (in_buf, rtok_start, rtok_end) ||
                valid_integral (in_buf, rtok_start, rtok_end) ||
                is_expr)) {

                failed = 1;
                goto fail;
            }

            // Refine the block label
            if (is_expr) {
                // TODO: Update to include more expr types
                BABBLE_ASSERT (block_label_offset == 0);
                if (blist->blocks[i].label == INC) {
                    BABBLE_MSG_COMPILE_ERR (start_line,
                        " (+= not supported for string exprs)\n");
                    failed = 1;
                    generic_msg = 0;
                    goto fail;
                }
                blist->blocks[i].label += (block_label_offset + 1);
            } else {
                blist->blocks[i].hotspots[2] = rtok_end;
            }

        }
        if (blist->blocks[i].label == REP || blist->blocks[i].label == PRINT) {
            size_t h1 = blist->blocks[i].hotspots[0];
            size_t h2 = blist->blocks[i].hotspots[1];
            size_t tok_start = find_next (in_buf, h1 + 1, h2 - 1);
            size_t tok_end = find_prev (in_buf, h1 + 1, h2 - 1);
            if ((tok_start == -1) || (tok_start > tok_end)) {
                BABBLE_MSG_COMPILE_ERR (start_line, " (missing argument)\n");
                generic_msg = 0;
                failed = 1;
                goto fail;
            }

            blist->blocks[i].hotspots[0] = tok_start;
            blist->blocks[i].hotspots[1] = tok_end;
            if (!(valid_symbol (in_buf, tok_start, tok_end) ||
                valid_integral (in_buf, tok_start, tok_end))) {

                failed = 1;
            }
        }
    fail:
        if (failed) {
            if (generic_msg) {
                snprintf (msg, MSG_LEN, "Babble error: Compile error on line %d\n",
                    blist->blocks[i].start_line);
            }
            ret = BABBLE_COMPILE_ERR;
            goto done;
        }
    }
done:
    dbg_blist ("blist", *blist);
    if (ret != BABBLE_OK) {
        free_blist (blist);
    }
    free_blist (&blist_phase2);
    return ret;
}