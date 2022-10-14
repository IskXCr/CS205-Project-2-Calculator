#include "utils.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* Print the "out of memory" error and exit */
void out_of_memory()
{
    fprintf(stderr, "critical error: out of memory.\n");
    exit(1);
}

void sap_warn(char *msg)
{
    fprintf(stderr, "SAP error: %s\n", msg);
    exit(1);
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