#ifndef BABBLE_LANG_H
#define BABBLE_LANG_H

#define BABBLE_VER_MAJOR 1
#define BABBLE_VER_MINOR 4

#define BABBLE_OK               0
#define BABBLE_EARLY_QUIT       1
// Errors
#define BABBLE_BAD_ARGS          -1
#define BABBLE_FILE_NOT_FOUND    -2
#define BABBLE_COMPILE_ERR       -3
#define BABBLE_MISSING_ASSEMBLER -4
#define BABBLE_ASSEMBLER_ERR     -5
#define BABBLE_LINKER_ERR        -6
#define BABBLE_MISC_ERR          -7

#if (__STDC_VERSION__ < 200000)
#error "Babble source requires at least C11"
#endif

// Constants
#define MSG_LEN 1024

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/time.h>

#include "error.h"

#endif
