/* Header file for utilities used. */
#include <stdio.h>

/* Constant definitions */
#ifndef _UTILS_H
#define _UTILS_H
#define MAXTOKEN 255


/* Function prototypes */

void utils_init_lib(void (*handler_exc)(void));

void out_of_memory(void);

void sap_warn(char *msg, int cnt, ...);

char *fetch_token(char *src);

char **fetch_expr(char *src);

void free_expr_array(char ***src);

#endif