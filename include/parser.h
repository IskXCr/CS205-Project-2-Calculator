/* Header file for declarations of parser functions */

#ifndef _PARSER_H
#define _PARSER_H


/* Include libraries */

#include "number.h"


/* Definitions */

#ifdef TRUE
#undef TRUE
#endif
#define TRUE 1

#ifdef FALSE
#undef FALSE
#endif
#define FALSE 0

/* Function token definitions */

#define _SAP_TEXT_FUNC_SIN    "sin"
#define _SAP_TEXT_FUNC_COS    "cos"
#define _SAP_TEXT_FUNC_SQRT   "sqrt"
#define _SAP_TEXT_FUNC_ARCTAN "atan"
#define _SAP_TEXT_FUNC_LN     "ln"
#define _SAP_TEXT_FUNC_EXP    "exp"


/* Enum declarations */

/* Defines token types for the parser. */
typedef enum sap_token_type
{
    _SAP_STACK_SENTINEL = -1, /* Sentinel of the tokens */
    _SAP_END_OF_STMT = 1,     /* End of statement */
    
    _SAP_LESS,    /* less */
    _SAP_GREATER, /* greater */
    _SAP_EQ,      /* Equal */
    _SAP_LEQ,     /* less or equal */
    _SAP_GEQ,     /* greater or equal */
    _SAP_NEQ,     /* Not equal */
    _SAP_ASSIGN,  /* Assignment */

    _SAP_ADD,      /* Add */
    _SAP_MINUS,    /* Minus */
    _SAP_MULTIPLY, /* Multiply */
    _SAP_DIVIDE,   /* Divide */
    _SAP_MODULO,  /* Modulus */
    _SAP_POWER,    /* Power */

    _SAP_SQRT,   /* Sqrt */
    _SAP_SIN,    /* Sin */
    _SAP_COS,    /* Cos */
    _SAP_ARCTAN, /* Arctangent */
    _SAP_LN,     /* Natural Logarithm */
    _SAP_EXP,    /* exp */

    _SAP_PAREN_L, /* Left parentheses */
    _SAP_PAREN_R, /* Right parentheses */

    _SAP_VARIABLE, /* Variable name */
    _SAP_NUMBER,   /* Number */
    _SAP_SUB_EXPR, /* Sub expression: Expression enclosed in a pair of parentheses. */
    
    _SAP_FUNC_CALL /* Function calls */
} sap_token_type;


/* Struct declarations */

typedef struct sap_token_struct *sap_token;

/* Structure used to store the actual token. */
typedef struct sap_token_struct
{
    sap_token_type type; /* Type of this token. */
    char *name;          /* If it is a variable or function call, stores its name in a copy. Else it is NULL. Must be copied when using. */
    sap_num val;         /* If it is a number, stores the value, and when use this value please ensure that it is copied (referenced). Else it is NULL. */

    /* If it is a function, stores the array of tokens of arguments. 
       Else, it is NULL. Array must end with _SAP_END_OF_STMT.
       The struct has complete control over the array, and _sap_free_token will free the array when necessary. */
    struct sap_token_struct **arg_tokens; 

    int negate; /* TRUE if the evaluation result of this token is to be negated. */
} sap_token_struct;


/* Function prototypes */

sap_token sap_get_sentinel(void);

int sap_is_operand(sap_token token);

int sap_is_operator(sap_token token);

int sap_is_func(sap_token token);

int sap_get_in_prec(sap_token token);

int sap_get_out_prec(sap_token token);

int sap_get_token_arr_length(sap_token *array);

sap_token *sap_parse_expr(char *src);

void sap_token_trans2num(sap_token token, sap_num val);

void sap_free_tokens(sap_token **array);


/* Debug function prototypes. They are forbidden to use in production environments. */

char *_sap_debug_token2text(sap_token token);

void _sap_debug_print_token_arr(sap_token *token);

#endif
