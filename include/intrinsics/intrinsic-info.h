#ifndef INTRINSIC_INFO_H
#define INTRINSIC_INFO_H

typedef struct intrinsic_info {
    char *symbol;
    char *source;
    int impl;       // 1 if an implementation. All impls are populated in the init stage.
} intrinsic_info;

#endif