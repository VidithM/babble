#include "babble-lang.h"
#include "compile.h"

static int parse_args (int argc, char **argv,
    char **out_name, char **in_name, int *debug) {
    // Format:
    // babble [options] file
    const char *help = "Usage: babble [options] file\n"
        "Options:\n"
        "-g\t\t\tgenerate debugging information\n"
        "-o [output file]\tname of executable\n"
        "-v\t\t\tversion\n"
        "--help\t\t\tdisplay this information\n";
    (*in_name) = NULL;
    (*out_name) = NULL;
    (*debug) = 0;
    int ret = BABBLE_OK;
    int ok = 1;

    for (int i = 1; i < argc; i++) {
        if (!strcmp (argv[i], "-g")) {
            (*debug) = 1;
        } else if (!strcmp (argv[i], "-o")) {
            if (i == argc - 1) {
                ok = 0;
                break;
            }
            (*out_name) = argv[i + 1];
            i++;
        } else if (!strcmp (argv[i], "-v")) {
            if (argc > 2) {
                ok = 0;
                break;
            }
            printf ("Babble v%d.%d\n", BABBLE_VER_MAJOR, BABBLE_VER_MINOR);
            ret = BABBLE_EARLY_QUIT;
            break;
        } else if (!strcmp (argv[i], "--help")) {
            if (argc > 2) {
                ok = 0;
                break;
            }
            printf ("%s", help);
            ret = BABBLE_EARLY_QUIT;
        } else {
            if (i < argc - 1) {
                ok = 0;
                break;
            }
            (*in_name) = argv[i];
        }
    }
    if (!ok) {
        printf ("Invalid arguments\n%s", help);
        (*in_name) = NULL;
        (*out_name) = NULL;
        (*debug) = 0;
        ret = BABBLE_BAD_ARGS;
    }
    return ret;
}

int main (int argc, char **argv) {
    char *out_name, *in_name;
    int debug, ret;
    if (parse_args (argc, argv, &out_name, &in_name, &debug)) {
        // Nothing more to do
        return 0;
    }
    if (in_name == NULL) {
        printf ("Babble error: Missing input file\n");
        return 0;
    }
    if (out_name == NULL) {
        out_name = "a.out";
    }
    ret = compile (debug, in_name, out_name);
}
