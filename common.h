#pragma once

#define EXPAND_PARAMS_1(i, Name, X, XL, t0, a0)       XL(i, Name, t0, a0)
#define EXPAND_PARAMS_2(i, Name, X, XL, t0, a0, ...)  X(i, Name, t0, a0) EXPAND_PARAMS_1(i+1, Name, X, XL, __VA_ARGS__)
#define EXPAND_PARAMS_3(i, Name, X, XL, t0, a0, ...)  X(i, Name, t0, a0) EXPAND_PARAMS_2(i+1, Name, X, XL, __VA_ARGS__)
#define EXPAND_PARAMS_4(i, Name, X, XL, t0, a0, ...)  X(i, Name, t0, a0) EXPAND_PARAMS_3(i+1, Name, X, XL, __VA_ARGS__)
#define EXPAND_PARAMS_5(i, Name, X, XL, t0, a0, ...)  X(i, Name, t0, a0) EXPAND_PARAMS_4(i+1, Name, X, XL, __VA_ARGS__)
#define EXPAND_PARAMS_6(i, Name, X, XL, t0, a0, ...)  X(i, Name, t0, a0) EXPAND_PARAMS_5(i+1, Name, X, XL, __VA_ARGS__)
#define EXPAND_PARAMS_7(i, Name, X, XL, t0, a0, ...)  X(i, Name, t0, a0) EXPAND_PARAMS_6(i+1, Name, X, XL, __VA_ARGS__)
#define EXPAND_PARAMS_8(i, Name, X, XL, t0, a0, ...)  X(i, Name, t0, a0) EXPAND_PARAMS_7(i+1, Name, X, XL, __VA_ARGS__)
#define EXPAND_PARAMS_9(i, Name, X, XL, t0, a0, ...)  X(i, Name, t0, a0) EXPAND_PARAMS_8(i+1, Name, X, XL, __VA_ARGS__)
#define EXPAND_PARAMS_10(i, Name, X, XL, t0, a0, ...) X(i, Name, t0, a0) EXPAND_PARAMS_9(i+1, Name, X, XL, __VA_ARGS__)
#define EXPAND_PARAMS_11(i, Name, X, XL, t0, a0, ...) X(i, Name, t0, a0) EXPAND_PARAMS_10(i+1, Name, X, XL, __VA_ARGS__)
#define EXPAND_PARAMS_12(i, Name, X, XL, t0, a0, ...) X(i, Name, t0, a0) EXPAND_PARAMS_11(i+1, Name, X, XL, __VA_ARGS__)
#define EXPAND_PARAMS_13(i, Name, X, XL, t0, a0, ...) X(i, Name, t0, a0) EXPAND_PARAMS_12(i+1, Name, X, XL, __VA_ARGS__)

#define EXPAND_PARAMS(X, XL, num, Name, ...) \
    EXPAND_PARAMS_##num(0, Name, X, XL, __VA_ARGS__)

#define EXPAND_PARAMS_TO_DECL_AUX(i, Name, t, a)   t a;

#define EXPAND_PARAMS_TO_DECL(num, Name, ...) \
    EXPAND_PARAMS( \
        EXPAND_PARAMS_TO_DECL_AUX, \
        EXPAND_PARAMS_TO_DECL_AUX, \
        num, Name, __VA_ARGS__)

#define EXPAND_PARAMS_TO_ARGS_AUX(i, Name, t, a)         t a,
#define EXPAND_PARAMS_TO_ARGS_AUX_LAST(i, Name, t, a)    t a

#define EXPAND_PARAMS_TO_ARGS(num, Name, ...) \
    EXPAND_PARAMS( \
        EXPAND_PARAMS_TO_ARGS_AUX, \
        EXPAND_PARAMS_TO_ARGS_AUX_LAST, \
        num, Name, __VA_ARGS__)

#define EXPAND_PARAMS_TO_CONTENTS_OF_COMMAND_QUEUE_AUX(i, Name, t, a) \
    value->args->MPI_##Name##_args-> a = a ;

#define EXPAND_PARAMS_TO_CONTENTS_OF_COMMAND_QUEUE(num, Name, ...) \
    EXPAND_PARAMS( \
        EXPAND_PARAMS_TO_CONTENTS_OF_COMMAND_QUEUE_AUX, \
        EXPAND_PARAMS_TO_CONTENTS_OF_COMMAND_QUEUE_AUX, \
        num, Name, __VA_ARGS__)

#define EXPAND_PARAMS_TO_ARGS_ARROW_PARAMS_AUX(i, Name, t, a) args-> a,

#define EXPAND_PARAMS_TO_ARGS_ARROW_PARAMS(num, Name, ...) \
    EXPAND_PARAMS( \
        EXPAND_PARAMS_TO_ARGS_ARROW_PARAMS_AUX, \
        EXPAND_PARAMS_TO_ARGS_ARROW_PARAMS_AUX, \
        num, Name, __VA_ARGS__)
