#ifndef ERROR_H
#define ERROR_H

#define BABBLE_MSG(...) snprintf (msg, MSG_LEN, __VA_ARGS__);
#define BABBLE_MSG_COMPILE_ERR(line, ...)                                           \
{                                                                                   \
    if (line == -1) {                                                               \
        snprintf (msg, MSG_LEN, "Babble error: Compile error on line %d ", line);   \
    } else {                                                                        \
        snprintf (msg, MSG_LEN, "Babble error: Compile error");                     \
    }                                                                               \
    size_t len = strlen (msg);                                                      \
    snprintf (msg + len, MSG_LEN - len, __VA_ARGS__);                               \
}

#define BABBLE_STATIC_ASSERT(_val) _Static_assert (_val, "\n");

#ifdef DEBUG
static void babble_assert_brkpt () {}   // For GDB
#define BABBLE_ASSERT(_val)                                              \
{                                                                        \
    if (!(_val)) {                                                       \
        fprintf (stderr, "[ERROR]: Assertion failed: %s, line %d\n",     \
            __FILE__, __LINE__);                                         \
        babble_assert_brkpt();                                           \
        exit (-1);                                                       \
    }                                                                    \
}
#define BABBLE_ASSERT_IMPLIES(a, b) BABBLE_ASSERT (!a || b);
#define BABBLE_BRKPT babble_assert_brkpt()
#else
#define BABBLE_ASSERT
#define BABBLE_ASSERT_IMPLIES
#define BABBLE_BRKPT
#endif

#endif