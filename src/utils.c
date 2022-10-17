#include "utils.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
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

/* Output the warning message, following by first count of comments, and then 
   a list of comments and int FLAG to indicate whether to free the corresponding pointer after use.
   Then let utils library handle the exception.
   The char* arguments will be **consumed** if the flag following the char* is TRUE, meaning automatically freed after use. */
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
        int do_free = va_arg(argp, int);
        if (do_free)
            free(p);
    }
    va_end(argp);

    fprintf(stderr, "\n");

    /* Call exception handler. */
    if (_handler_exc != NULL)
        (*_handler_exc)();
}

#if !defined (__unix__) && !(defined (__APPLE__) && defined (__MACH__)) 
#define _UTILS_BUF_DEFAULT_SIZE 100

/* Like getline() in a standard POSIX library. However, it does not set error flags, and simply exits if there is no enough space. */
ssize_t getline0(char **lineptr, size_t *size, FILE *file)
{
    size_t len;   /* Length of the current buf. Dynamically allocated. */
    char *buf; /* Buf used to store the result. */
    char *ptr; /* Next free position in the buffer */
    int c;     /* Store the character */

    /* Initialization */
    len = _UTILS_BUF_DEFAULT_SIZE;
    ptr = buf = (char *)malloc(len);
    if (buf == NULL)
        out_of_memory();

    /* Assignments */
    *lineptr = buf;
    *size = len;

    while ((c = fgetc(file)) != '\0' && c != EOF && c != '\n' )
    {
        if (ptr - buf == len)
        {
            char *newbuf = (char *)realloc(buf, len + _UTILS_BUF_DEFAULT_SIZE);
            if (newbuf == NULL)
                out_of_memory();
            *lineptr = newbuf;
            buf = newbuf;
            ptr = buf + len;
            len += _UTILS_BUF_DEFAULT_SIZE;
            *size = len;
        }
        *ptr++ = c;
    }
    if (c == '\n')
        *ptr++ = c;
    *ptr = '\0';
    
    return (ssize_t)(ptr - buf);
}
#endif

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
    char *tmp;     /* Store memory allocation result */
    int len0;      /* Store token length */
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
            result = (char **)realloc(result, (len + _UTILS_EXPR_ARR_SIZE) * sizeof(char *));
            if (result == NULL)
                out_of_memory();
            rw = result + len - 1;
            len += _UTILS_EXPR_ARR_SIZE;
        }

        /* Assign the token to a slot in the result array. */
        len0 = strlen(token) + 1;
        tmp = (char *)malloc(len0);
        if (tmp == NULL)
            out_of_memory();
        memcpy(tmp, token, len0);

        *rw++ = tmp;
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