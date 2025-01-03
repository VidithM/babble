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
    put.start = start; put.end = end;
    put.start_line = start_line; put.label = label;
    blist->blocks[blist->nblocks] = put;
    blist->nblocks++;
    return ret;
}

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
    printf ("start line is: %d; start: %ld\n", start_line, start);
    if (start < buf_size) {
        // implicit ';' at the end
        push_block (blist, start, buf_size, start_line,
            UNKNOWN);
    }
    printf ("blist blocks: %ld\n", blist->nblocks);
    // Handle REP and SCOPE_OPEN, SCOPE_CLOSE; overwrite OG blist
    blocklist blist_phase2;
    init_blist (&blist_phase2, blist->cap);
    start = 0;
    line = 1;
    for (size_t i = 0; i < blist->nblocks; i++) {
        int changed = 0;
        int failed = 0;
        size_t start = blist->blocks[i].start;
        size_t end = blist->blocks[i].end;
        size_t skipto = 0;
        int line = blist->blocks[i].start_line;
        for (size_t j = start; j < end; j++) {
            if (j < skipto) {
                goto skip;
            }
            size_t match_res;
            // TODO: Change parse funcs to accept in_buf, start, end separately and return absolute
            // offset from in_buf
            if (match_res = match (in_buf + j, in_buf + end, "rep", 3)) {
                // 1. find_next should be a '('
                match_res = find_next (in_buf + j + 3, in_buf + end);
                printf ("stg 0: %p, %c\n", (char*) match_res, *((char*)match_res));
                if (match_res == -1) { continue; }
                if (*((char*) match_res) != '(') { continue; }
                blist->blocks[i].hotspots[0] = (match_res - (size_t) in_buf - start);

                // 2. find_next_pat on ')'
                match_res = find_next_pat ((char*) match_res + 1,
                    in_buf + end, ")", 1);
                printf ("stg 1\n");
                if (match_res == -1) { continue; }
                blist->blocks[i].hotspots[1] = (match_res - start);

                // 4. find_next should be a '{'
                match_res = find_next ((char*) match_res + 1,
                    in_buf + end);
                if (match_res == -1) { continue; }
                if (*((char*) match_res) != '{') { continue; }
                printf ("stg 2\n");
                blist->blocks[i].hotspots[2] = (match_res - (size_t) in_buf - start);

                // jump to after the idx of (3)
                skipto = j + 1;
                changed = 1;
                printf ("Matched a rep, start at %ld, end at %ld\n", j, match_res - (size_t) in_buf);
                push_block (&blist_phase2,
                    j,
                    match_res - start,
                    line,
                    REP);
            }
            if (match_res = match (in_buf + j, in_buf + end, "{", 1)) {

            }
            if (match_res = match (in_buf + j, in_buf + end, "}", 1)) {
            }
            skip:
                line += (in_buf[j] == '\r' || in_buf[j] == '\n');
        }
        if (failed) {
            free_blist (&blist_phase2);
            return BABBLE_COMPILE_ERR;
        }
        if (!changed) {
            push_block (&blist_phase2,
                blist->blocks[i].start,
                blist->blocks[i].end,
                blist->blocks[i].start_line,
                UNKNOWN);
        }
    }
    // ...
    free_blist (blist);
    blist->blocks = blist_phase2.blocks;
    blist->nblocks = blist_phase2.nblocks;
    blist->cap = blist_phase2.cap;

    // Label the blocks of blist

}