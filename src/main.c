#include "babble-lang.h"
#include "compile.h"

static void parse_args (int argc, char **argv,
    char **out_name, char **in_name, int *debug, int *quit) {

    // Format:
    // babble [options] file
    // Scan tokens until an unrecognized token is received. This must be the in_name.
    const char *help = "Usage: babble [options] file\n"
        "Options:\n"
        "-g\t\t\tgenerate debugging information\n"
        "-o [output file]\tname of executable\n"
        "-v\t\t\tversion\n"
        "--help\t\t\tdisplay this information\n";
    (*in_name) = NULL;
    (*out_name) = NULL;
    (*debug) = 0;
    (*quit) = 0;
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
            (*quit) = 1;
            break;
        } else if (!strcmp (argv[i], "--help")) {
            if (argc > 2) {
                ok = 0;
                break;
            }
            printf ("%s", help);
            (*quit) = 1;
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
        (*quit) = 1;
    }
}

int main (int argc, char **argv) {
    char *out_name, *in_name;
    int debug;
    int quit;
    parse_args (argc, argv, &out_name, &in_name, &debug, &quit);
    if (quit) {
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
    gen_asm (in_name, out_name);
    assemble (debug);
}
