#include "babble-lang.h"

int gen_asm (const char *in_name, const char *out_name) {
    FILE *in_file = fopen (in_name, "r");
    if (in_file == NULL) {
        return BABBLE_FILE_NOT_FOUND;
    }

    char cmd_buf [1024];
    snprintf (cmd_buf, 1024, "rm -f %s", out_name);
    system (cmd_buf);
    snprintf (cmd_buf, 1024, "touch %s", out_name);
    system (cmd_buf);

    FILE *out_file = fopen (out_name, "w");

    int64_t curr_val = 0;
    while (1) {
        char c = fgetc (in_file);
        if (feof (in_file)) {
            break;
        }
        switch (c) {
            case '+':
                break;
            case '-':
                break;
            case 'p':
                break;
        }
    }
}
void assemble (int debug) {

}