/* Header file for utilities used. */

#ifndef _UTILS_H
#define _UTILS_H


/* Included libraries */

#include <stdio.h>

/* Constant definitions */

#define MAXTOKEN 255


/* Function prototypes */

void utils_init_lib(void (*handler_exc)(void));

void out_of_memory(void);

void sap_warn(char *msg, int cnt, ...);

#if !defined (__unix__) && !(defined (__APPLE__) && defined (__MACH__)) 
ssize_t getline0(char **lineptr, size_t *size, FILE *file);
#endif

char *fetch_token(char *src);

char **fetch_expr(char *src);

void free_expr_array(char ***src);

#endif