#include "utils.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

static void (*_handler_exc)(void) = NULL; /* Exception handler. */

void init_utils_lib(void (*handler_exc)(void))
{
    _handler_exc = handler_exc;
}

/* Print the "out of memory" error and exit */
void out_of_memory()
{
    fprintf(stderr, "critical error: out of memory.\n");
    exit(1);
}

void sap_warn(char *msg)
{
    fprintf(stderr, "SAP error: %s\n", msg);
    if (_handler_exc != NULL)
        (*_handler_exc)();
}

/* Fetch the next token starting at *src until the first whitespace character or EOF */
char *fetch_token(char *src)
{
    if (*src == '\0' || isspace(*src))
        return NULL;
    char *p = NULL;
    sscanf(src, "%ms", &p);
    return p;
}

char *fetch_expr(FILE *src)
{
}