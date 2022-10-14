/* Header file for defining tokens for the parser */
#ifndef _SAP_H
#define _SAP_H

typedef enum {
    ENDOFLINE,
    NUMBER,
    NAME,
    SQRT,
    SIN,
    COS,
    EXP,
    ARCT,
    LN
} token_type;

#endif