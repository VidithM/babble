#ifndef INTRINSICS_H
#define INTRINSICS_H

#define MAX_INTRINSICS 128

typedef struct intrinsic_info {
    char *symbol;
    char *source;
    int impl;       // 1 if an implementation. All impls are populated in the init stage.
} intrinsic_info;

#endif