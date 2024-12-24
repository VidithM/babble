#ifndef BABBLE_LANG_H
#define BABBLE_LANG_H

#define BABBLE_VER_MAJOR 1
#define BABBLE_VER_MINOR 0

#define BABBLE_OK               0
#define BABBLE_EARLY_QUIT       1
// Errors
#define BABBLE_BAD_ARGS          -1
#define BABBLE_FILE_NOT_FOUND    -2
#define BABBLE_COMPILE_ERR       -3
#define BABBLE_MISSING_ASSEMBLER -4
#define BABBLE_ASSEMBLER_ERR     -5
#define BABBLE_MISC_ERR          -6


#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#endif
