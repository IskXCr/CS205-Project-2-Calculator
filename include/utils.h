/* Header file for utilities used. */
#include <stdio.h>

/* Constant definitions */
#ifndef _UTILS_H
#define _UTILS_H
#define MAXTOKEN 255


/* Function prototypes */

void out_of_memory(void);

void sap_warn(char *msg);

char *fetch_token(char *src);

char *fetch_expr(FILE* src);

#endif