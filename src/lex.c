#include "lex.h"
#include "parse.h"

static int init_blist (blocklist *blist, size_t cap) {
    assert (blist != NULL);
    blist->nblocks = 0;
    blist->cap = cap; 
    blist->blocks = (block*) malloc (cap * sizeof (block));
    if (blist->blocks == NULL) {
        return BABBLE_MISC_ERR;
    }
}

static int resize_blist (blocklist *blist) {
    assert (blist != NULL);
    blist->cap *= 2;
    blist->blocks = realloc (blist->blocks, blist->cap);
    if (blist->blocks == NULL) {
        return BABBLE_MISC_ERR;
    }
}

static int push_block (blocklist *blist,
    size_t start, size_t end,
    int start_line,
    enum block_label label) {

    int ret = BABBLE_OK;
    if (blist->nblocks == blist->cap) {
        ret = resize_blist (blist);
        if (!ret) {
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
void dbg_blist (const char *name, blocklist *blist) {
    printf ("==== DEBUG BLOCKLIST ====\n");
    printf ("name: %s\n", name);
    printf ("nblocks: %ld\n", blist->nblocks);
    printf ("cap: %ld\n\n", blist->cap);
    for (size_t i = 0; i < blist->nblocks; i++) {
        printf ("Block: %ld\n", i);
        char *label_name;
        switch (blist->blocks[i].label) {
            case UNKNOWN:
                label_name = "UNKNOWN";
                break;
            case EMPTY:
                label_name = "EMPTY";
                break;
            case INC:
                label_name = "INC";
                break;
            case EQ:
                label_name = "EQ";
                break;
            case REP:
                label_name = "REP";
                break;
            case PRINT:
                label_name = "PRINT";
                break;
            case SCOPE_OPEN:
                label_name = "SCOPE OPEN";
                break;
            default:
                label_name = "SCOPE CLOSE";
        }
        printf ("\tLabel: %d (%s)\n", blist->blocks[i].label, label_name);
        printf ("\tStart: %ld\n", blist->blocks[i].start);
        printf ("\tEnd: %ld\n", blist->blocks[i].end);
        printf ("\tStart line: %d\n", blist->blocks[i].start_line);
        printf ("\tHotspots: ");
        for (size_t j = 0; j < MAX_HOTSPOTS; j++) {
            printf ("%02ld ", blist->blocks[i].hotspots[j]);
        }
        printf ("\n");
    }
    printf ("========\n");
}
#else
void dbg_blist (const char *name, blocklist *blist) {}
#endif

void free_blist (blocklist *blist) {
    free (blist->blocks);
    memset (blist, 0x0, sizeof (blocklist));
}

int lex (char *in_buf, size_t buf_size, blocklist *blist, char *msg) {
    // init_blocks (blocks_handle);
    // Phase 1: Split by ';'
    // Phase 2: Handle REP and SCOPE
    // Phase 3: Label blocks
    init_blist (blist, DEFAULT_CAP);
    size_t start = 0;
    int start_line = 1;
    int line = 1;
    for (size_t i = 0; i < buf_size; i++) {
        if (in_buf[i] == ';') {
            push_block (blist, start, i, start_line,
                UNKNOWN);
            start = i + 1;
            start_line = line;
        }
        if (in_buf[i] == '\r' || in_buf[i] == '\n') {
            line++;
        }
    }

    if (start < buf_size) {
        // implicit ';' at the end
        push_block (blist, start, buf_size, start_line,
            UNKNOWN);
    }
    
    dbg_blist ("blist", blist);

    // Handle REP and SCOPE_OPEN, SCOPE_CLOSE; overwrite OG blist
    blocklist blist_phase2;
    init_blist (&blist_phase2, blist->cap);
    start = 0;
    line = 1;
    for (size_t i = 0; i < blist->nblocks; i++) {
        int changed = 0;
        int failed = 0;
        int line = blist->blocks[i].start_line;
        size_t start = blist->blocks[i].start;
        size_t end = blist->blocks[i].end;
        size_t curr = start;
        while (1) {
            // jump to next non-space
            size_t nxt = find_next (in_buf, start, blist->blocks[i].end);
            start = nxt;
            if (start >= end) {
                break;
            }

            if (curr < start) {
                line += (in_buf[curr] == '\r' || in_buf[curr] == '\n');
                curr++;
                continue;
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

                // Must find a '{', else fail
                match_res = find_next_pat (in_buf, match_res + 1, end, "{", 1);
                if (match_res == -1) { break; }

                push_block (&blist_phase2,
                    start,
                    match_res,
                    line,
                    REP);
                blist_phase2.blocks[blist_phase2.nblocks - 1].hotspots[0] = h1;
                blist_phase2.blocks[blist_phase2.nblocks - 1].hotspots[1] = h2;

                start = match_res + 1;
            }
            if (match (in_buf, start, end, "{", 1)) {
                push_block (&blist_phase2,
                    start,
                    start,
                    line,
                    SCOPE_OPEN);
                start++;
                continue;
            }
            if (match (in_buf, start, end, "}", 1)) {
                push_block (&blist_phase2,
                    start,
                    start,
                    line,
                    SCOPE_CLOSE);
                start++;
                continue;
            }
        terminal:
            if ((match_res = find_next_pat (in_buf, start, end, "=", 1)) != -1) {

            }
            if ((match_res = find_next_pat (in_buf, start, end, "+=", 2)) != -1) {

            }
            if ((match_res = find_next_pat (in_buf, start, end, "print", 5)) != -1) {
                // next should be a 
            }
            start = end;
        }
        if (start < end) {
            push_block (&blist_phase2,
                blist->blocks[i].start,
                blist->blocks[i].end,
                blist->blocks[i].start_line,
                UNKNOWN);
        }
        dbg_blist ("blist (phase 2)", &blist_phase2);
    }
    // ...
    free_blist (blist);
    blist->blocks = blist_phase2.blocks;
    blist->nblocks = blist_phase2.nblocks;
    blist->cap = blist_phase2.cap;

    // Label the blocks of blist

}