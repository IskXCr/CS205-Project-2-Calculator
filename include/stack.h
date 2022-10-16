/* Header file for the implementation of a token stack. */

#ifndef _STACK_H
#define _STACK_H


/* Included libraries */

#include "parser.h"


/* Definitions */

#ifdef TRUE
#undef TRUE
#endif
#define TRUE 1

#ifdef FALSE
#undef FALSE
#endif
#define FALSE 0

#define _STACK_DEFAULT_SIZE 20

/* Struct declarations */

typedef struct stack_struct *stack;

/* Structure used to store a token stack. */
typedef struct stack_struct
{
    int max_size;    /* Current max size */
    sap_token *base; /* Pointer to the base element */
    sap_token *ptr;  /* Pointer to the next free position in the array */
} stack_struct;

/* Function prototypes */

stack sap_new_stack(void);

void sap_free_stack(stack *s);

int sap_stack_has_element(stack s);

int sap_stack_empty(stack s);

void sap_stack_push(stack s, sap_token element);

sap_token sap_stack_pop(stack s);

sap_token sap_stack_top(stack s);

void sap_stack_reset(stack s);

#endif