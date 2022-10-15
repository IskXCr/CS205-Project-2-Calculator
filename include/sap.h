/* Header file for defining tokens for the parser */
#ifndef _SAP_H
#define _SAP_H

typedef enum {
    ENDOFSTATEMENT = 1,
    NUMBER,
    NAME,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVISION,
    MODULUS,
    SQRT,
    SIN,
    COS,
    ARCTAN,
    EXP,
    POW,
    LN,
    ASSIGN,
    LEQ,
    EQ,
    GER
} token_type;

#endif