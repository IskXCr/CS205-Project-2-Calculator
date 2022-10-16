#include "utils.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

static void (*_handler_exc)(void); /* Exception handler. */

/* Initialize exception handler through assigning a void function pointer. */
void utils_init_lib(void (*handler_exc)(void))
{
    _handler_exc = handler_exc;
}

/* Print the "out of memory" error and exit */
void out_of_memory()
{
    fprintf(stderr, "critical error: out of memory.\n");
    exit(1);
}

/* Output the warning message, following by a list of comments. Then let utils library handle the exception.
   The char* arguments will be **consumed**, meaning automatically freed after use. */
void sap_warn(char *msg, int cnt, ...)
{
    fprintf(stderr, "SAP error: %s", msg);

    /* Use variable argument list to fetch other messages. */
    va_list argp;
    va_start(argp, cnt);
    for (int i = 0; i < cnt; ++i)
    {
        char *p = va_arg(argp, char *);
        fprintf(stderr, "%s", p);
        free(p);
    }
    va_end(argp);

    fprintf(stderr, "\n");

    /* Call exception handler. */
    if (_handler_exc != NULL)
        (*_handler_exc)();
}

/* Fetch the next token starting at *src until the first whitespace character or EOF. String must be freed afterwards. */
char *fetch_token(char *src)
{
    if (*src == '\0' || isspace(*src))
        return NULL;
    char *p = NULL;
    sscanf(src, "%ms", &p);
    return p;
}

#define _UTILS_EXPR_ARR_SIZE 5

/* Fetch expressions. The expression terminate with a semicolon or newline character, or '\0' character.
   It is guaranteed that the last element of the array returned points to NULL. Any element in the array,
   along with the array itself, must be freed afterwards. You may call free_expr_array(&src) on the returned pointer. */
char **fetch_expr(char *src)
{
    char *src0; /* Store a copy of the source string */
    int srclen = strlen(src) + 1;
    int len = _UTILS_EXPR_ARR_SIZE; /* Store the size of the result array. */

    static const char delim[] = ";\n";
    char **result; /* Store the result array. */
    char **rw;     /* Next available position in the result array */
    char *token;


    /* Initialization */
    src0 = (char *)malloc(srclen);
    if (src0 == NULL)
        out_of_memory();
    memcpy(src0, src, srclen); /* We need a copy of strtok in order for this function to work. */

    token = strtok(src0, delim);
    rw = result = (char **)malloc(len * sizeof(char *));
    if (result == NULL)
        out_of_memory();

    while (token != NULL)
    {
        /* Reallocate memory if necessary. */
        if (rw - result == len - 1)
        {
            len += _UTILS_EXPR_ARR_SIZE;
            result = (char **)realloc(result, len * sizeof(char *));
            if (result == NULL)
                out_of_memory();
            rw = result + len;
        }

        /* Assign the token to a slot in the result array. */
        *rw++ = token;
        token = strtok(NULL, delim); /* Let strtok fetch the next token. */
    }
    *rw = NULL; /* Set the last element to NULL. */
    

    /* Clean up */
    free(src0);

    return result;
}

/* Free the array of strings for simplicity. */
void free_expr_array(char ***src)
{
    char **ptr = *src;
    while (*ptr != NULL)
        free(*ptr++);
    free(*src);
    *src = NULL;
}