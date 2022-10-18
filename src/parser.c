/* Source file for SAP parser. */
#include "sapdefs.h"
#include "sap.h"
#include "parser.h"
#include "utils.h"

/* Global constants */
static sap_token sentinel = NULL;

/* Functions */

/* Useful debug routine. Convert an operator to a char string. */
static char *_sap_op_to_str(char s)
{
    char *p = (char *)malloc(2);
    if (p == NULL)
        out_of_memory();
    p[0] = s;
    p[1] = '\0';
    return p;
}

/* Functions for expression evaluations. */

/* Test if a token is operand */
int sap_is_operand(sap_token token)
{
    if (sap_is_func(token))
        return TRUE;
    switch (token->type)
    {
    case _SAP_VARIABLE:
    case _SAP_NUMBER:
        return TRUE;

    default:
        return FALSE;
    }
}

/* Test if a token is operator */
int sap_is_operator(sap_token token)
{
    switch (token->type)
    {
    case _SAP_END_OF_STMT:
    case _SAP_STACK_SENTINEL:
        return FALSE;

    case _SAP_PAREN_R:
        return FALSE;

    default:
        return !sap_is_operand(token);
    }
}

/* Test if a token is a function*/
int sap_is_func(sap_token token)
{
    switch (token->type)
    {
    case _SAP_SQRT:
    case _SAP_SIN:
    case _SAP_COS:
    case _SAP_ARCTAN:
    case _SAP_LN:
    case _SAP_EXP:
    case _SAP_FUNC_CALL:
        return TRUE;

    default:
        return FALSE;
    }
}

/* Get the IN precedence of an operator in the stack. */
int sap_get_in_prec(sap_token token)
{
    switch (token->type)
    {
    case _SAP_END_OF_STMT:
    case _SAP_STACK_SENTINEL:
        return -10;

    case _SAP_ASSIGN:
        return 1;

    case _SAP_EQ:
    case _SAP_NEQ:
        return 6;

    case _SAP_LESS:
    case _SAP_GREATER:
    case _SAP_LEQ:
    case _SAP_GEQ:
        return 11;

    case _SAP_ADD:
    case _SAP_MINUS:
        return 101;

    case _SAP_MULTIPLY:
    case _SAP_DIVIDE:
    case _SAP_MODULO:
        return 1001;

    case _SAP_POWER:
        return 10000;

    case _SAP_PAREN_L:
        return 0;

    default:
        return -1;
    }
}

/* Get the OUT precedence of an operator out of the stack */
int sap_get_out_prec(sap_token token)
{
    switch (token->type)
    {
    case _SAP_END_OF_STMT:
    case _SAP_STACK_SENTINEL:
        return -10;

    case _SAP_ASSIGN:
        return 2;

    case _SAP_EQ:
    case _SAP_NEQ:
        return 5;

    case _SAP_LESS:
    case _SAP_GREATER:
    case _SAP_LEQ:
    case _SAP_GEQ:
        return 10;

    case _SAP_ADD:
    case _SAP_MINUS:
        return 100;

    case _SAP_MULTIPLY:
    case _SAP_DIVIDE:
    case _SAP_MODULO:
        return 1000;

    case _SAP_POWER:
        return 10001;

    case _SAP_PAREN_L:
        return 1000000;

    default:
        return -1;
    }
}

/* Convert parameters to a new token node and ensure that parameters are safe (copied). Name and Val can be null. */
static sap_token _sap_new_token(sap_token_type type, char *name, sap_num val, sap_token *arg_tokens)
{
    sap_token tmp = (sap_token)malloc(sizeof(sap_token_struct));
    if (tmp == NULL)
        out_of_memory();

    /* Process type. */
    tmp->type = type;

    /* Process name. */
    if (name != NULL)
    {
        int size = strlen(name) + 1; /* Actual size of the name, including terminating '\0' */
        char *p = (char *)malloc(size);
        if (p == NULL)
        {
            out_of_memory();
        }
        memcpy(p, name, size);
        tmp->name = p;
    }
    else
        tmp->name = NULL;

    /* Process value. */
    if (val != NULL)
        tmp->val = sap_copy_num(val);
    else
        tmp->val = NULL;

    if (arg_tokens != NULL)
        tmp->arg_tokens = arg_tokens;
    else
        tmp->arg_tokens = NULL;

    /* Negation */
    tmp->negate = FALSE;

    return tmp;
}

static void _sap_free_token_array(sap_token **token);

/* Delete a token node and release resources. */
static void _sap_free_token(sap_token *token)
{
    if (*token == NULL)
        return;

    free((*token)->name);
    sap_free_num(&((*token)->val));
    if ((*token)->arg_tokens != NULL)
        _sap_free_token_array(&((*token)->arg_tokens));
    free(*token);
    *token = NULL;
}

/* Free a valid array of tokens, ended by _SAP_END_OF_STMT. */
static void _sap_free_token_array(sap_token **token_array)
{
    if (*token_array == NULL)
        return;

    sap_token *start = *token_array;
    while ((*start)->type != _SAP_END_OF_STMT)
        _sap_free_token(start++);
    _sap_free_token(start); /* Free _SAP_END_OF_STMT */
    free(*token_array);
    *token_array = NULL;
}

/* Get a stack sentinel from parser. */
sap_token sap_get_sentinel(void)
{
    if (sentinel == NULL)
        sentinel = _sap_new_token(_SAP_STACK_SENTINEL, NULL, NULL, NULL);
}

/* Get the length of token_array for convenience. The length contains _SAP_END_OF_STMT. */
int sap_get_token_arr_length(sap_token *array)
{
    int cnt = 1;
    while ((*array)->type != _SAP_END_OF_STMT)
    {
        array++;
        cnt++;
    }
    return cnt;
}

/* Parse the next token specified by lineptr. Lineptr will be updated. Leading whitespace characters are ignored. */
static sap_token _sap_parse_next_token(char **lineptr)
{
    sap_token result = NULL; /* Parse result. */
    char *ptr0 = *lineptr;   /* Mark the starting point for backup. */
    char *ptr = *lineptr;    /* Pointer used to iterate through the string. */

    while (isspace(*ptr)) /* Skip leading whitespace characters */
        ptr++;
    if (!isalnum(*ptr) && *ptr != '_') /* If isn't a function call, a variable or a number. It is possible to be '\0' */
    {
        sap_token_type type;
        if (*ptr == '\0')
            type = _SAP_END_OF_STMT;
        else
        {
            switch (*ptr)
            {
            case '+':
                type = _SAP_ADD;
                break;
            case '-':
                type = _SAP_MINUS;
                break;
            case '*':
                type = _SAP_MULTIPLY;
                break;
            case '/':
                type = _SAP_DIVIDE;
                break;
            case '%':
                type = _SAP_MODULO;
                break;
            case '^':
                type = _SAP_POWER;
                break;
            case '<':
                if (*++ptr == '=')
                    type = _SAP_LEQ;
                else
                {
                    type = _SAP_LESS;
                    --ptr;
                }
                break;
            case '>':
                if (*++ptr == '=')
                    type = _SAP_GEQ;
                else
                {
                    type = _SAP_GREATER;
                    --ptr;
                }
                break;
            case '=':
                if (*++ptr == '=')
                    type = _SAP_EQ;
                else
                {
                    type = _SAP_ASSIGN;
                    --ptr;
                }
                break;
            case '(':
                type = _SAP_PAREN_L;
                break;
            case ')':
                type = _SAP_PAREN_R;
                break;
            case '!':
                if (*++ptr == '=')
                    type = _SAP_NEQ;
                else
                {
                    --ptr;
                    sap_warn("Unknown operand: ", 1, _sap_op_to_str(*ptr), TRUE);
                }
                break;
            default:
                sap_warn("Unknown operand: ", 1, _sap_op_to_str(*ptr), TRUE);
                break;
            }
            ptr++;
        }

        result = _sap_new_token(type, NULL, NULL, NULL);
    }
    else if (isdigit(*ptr) || *ptr == '.') /* A number */
    {
        /* Validate number first. */
        char *ptr1 = ptr; /* Mark the start point. */
        int has_dp = FALSE;
        while (isdigit(*ptr) || *ptr == '.')
        {
            if (*ptr == '.')
                if (!has_dp)
                    has_dp = TRUE;
                else
                    sap_warn("syntax error: multiple decimal points.", 0);
            ptr++;
        }

        /* Allocate spaces and gather the result. */
        int len = ptr - ptr1;
        char *buf = (char *)malloc(len + 1);
        if (buf == NULL)
            out_of_memory();
        memcpy(buf, ptr1, len);
        *(buf + len) = '\0';

        /* New a number. */
        sap_num tmp = sap_str2num(buf);
        result = _sap_new_token(_SAP_NUMBER, NULL, tmp, NULL);

        /* Clean up. */
        sap_free_num(&tmp);
        free(buf);
    }
    else /* A variable or a function call */
    {
        /* Validate name first. */
        char *ptr1 = ptr; /* Mark the start point. */
        int cnt = 1;
        while (isalnum(*ptr) || *ptr == '_')
            ptr++;
        char *ptr2 = ptr; /* Mark the end of the name, one character after. */

        /* Allocate spaces and gather the result of the name. */
        int len = ptr2 - ptr1;
        char *buf = (char *)malloc(len + 1);
        if (buf == NULL)
            out_of_memory();
        memcpy(buf, ptr1, len);
        *(buf + len) = '\0';

        /* Skip function call blanks */
        while (isspace(*ptr))
            ptr++;
        /* Meet the first non-blank character. 
           If ptr is actually moved in the previous while loop, decrease. */
        if (*ptr != '(' && ptr > ptr2) 
            ptr--;

        /* Judge whether it is a function or a variable. */
        if (*ptr == '(') /* Calling functions */
        {
            sap_token_type type;
            if (strcmp(buf, _SAP_TEXT_FUNC_SIN) == 0)
                type = _SAP_SIN;
            else if (strcmp(buf, _SAP_TEXT_FUNC_COS) == 0)
                type = _SAP_COS;
            else if (strcmp(buf, _SAP_TEXT_FUNC_SQRT) == 0)
                type = _SAP_SQRT;
            else if (strcmp(buf, _SAP_TEXT_FUNC_ARCTAN) == 0)
                type = _SAP_ARCTAN;
            else if (strcmp(buf, _SAP_TEXT_FUNC_LN) == 0)
                type = _SAP_LN;
            else if (strcmp(buf, _SAP_TEXT_FUNC_EXP) == 0)
                type = _SAP_EXP;
            else
            {
                type = _SAP_FUNC_CALL;
                sap_warn("Unrecognized function: ", 1, buf, FALSE);
            }

            char *ptr3 = ptr + 1;
            while (*ptr != ')' && *ptr != '\0')
                ptr++;

            if (*ptr == ')')
                ptr++;
            else /* Unexpected unmatch of parentheses */
            {
                sap_warn("Unmatched parentheses. ", 0);
            }

            char *buf0;               /* Storing sub expression */
            int len = ptr - ptr3 - 1; /* Length of the sub expression */
            buf0 = (char *)malloc(len + 1);
            memcpy(buf0, ptr3, len);
            *(buf0 + len) = '\0';

            result = _sap_new_token(type, NULL, NULL, sap_parse_expr(buf0));

            /* Clean up. */
            free(buf0);
        }
        else
        {
            /* New a result. */
            result = _sap_new_token(_SAP_VARIABLE, buf, NULL, NULL);
        }

        /* Clean up. */
        free(buf);
    }

    *lineptr = ptr;
    return result;
}

#define _SAP_TOKEN_ARR_SIZE 30

/* Internal implementation for parsing an expression to array of tokens. */
static sap_token *sap_parse_expr_impl(char *src)
{
    /* Perform initial allocation for array. */
    int len = _SAP_TOKEN_ARR_SIZE;
    sap_token *arr = (sap_token *)malloc(len * sizeof(sap_token));
    sap_token *newarr;    /* In case we need to realloc more memory. */
    sap_token *ptr = arr; /* Next available position. */
    int negate = FALSE;   /* If TRUE in a loop, negate this operand. */

    if (arr == NULL)
        out_of_memory();

    sap_token next = NULL;
    do
    {
        /* If there is no available slot left */
        if (ptr - arr == len)
        {
            newarr = (sap_token *)realloc(arr, (len + _SAP_TOKEN_ARR_SIZE) * sizeof(sap_token));
            if (newarr == NULL)
                out_of_memory();
            len += _SAP_TOKEN_ARR_SIZE;
            arr = newarr;
            ptr = arr + len;
        }

        /* Fetch next token */
        next = _sap_parse_next_token(&src);

        /* If the operator is a '-', and (if there is no previous token, or the previous token is an operator) */
        if (next->type == _SAP_MINUS && (ptr <= arr || sap_is_operator(*(ptr - 1))))
        {
            negate = TRUE;
            _sap_free_token(&next);
            if (ptr > arr)
                next = *(ptr - 1);
            continue;
        }

        if (negate)
        {
            if (sap_is_operand(next))
                next->negate = TRUE;
            else
                sap_warn("Invalid unary minus. Token after: ", 1, _sap_debug_token2text(next), TRUE);
            negate = FALSE;
        }

        *ptr++ = next;
    } while (next == NULL || next->type != _SAP_END_OF_STMT); /* in case that leading tokens are being ignored. */

    return arr;
}

/* Parse an expression from src. This function guarantees that the returned array is ended with _SAP_END_OF_STMT.
   However, the array needs to be freed by the caller. src will only be read and no modifications will be made. */
sap_token *sap_parse_expr(char *src)
{
    return sap_parse_expr_impl(src);
}

/* Modify this token object to a number if possible. Negate the operand if required and set the flag to FALSE.
   All related resource will be freed. */
void sap_token_trans2num(sap_token token, sap_num val)
{
    sap_free_num(&(token->val));
    free(token->name);
    token->name = NULL;
    sap_free_tokens(&(token->arg_tokens));

    token->type = _SAP_NUMBER;
    token->val = sap_copy_num(val);
    if (token->negate) /* If the result should be negated */
    {
        sap_num tmp = sap_replicate_num(token->val);
        sap_free_num(&(token->val));
        token->val = tmp;
        sap_negate(token->val);
        token->negate = FALSE;
    }
}

/* Free a valid array of tokens that ends with _SAP_END_OF_STMT. */
void sap_free_tokens(sap_token **array)
{
    _sap_free_token_array(array);
}

/* Debug functions. They are forbidden to use in production environments. */

/* (Deprecated) Print token info for debug use. This function is not strictly written and lacks generosity.
   Forbidden to use in production.
   The upper bound length for subexpressions is capped at 10000, otherwise segmentation fault. */
char *_sap_debug_token2text(sap_token token)
{
    if (token == NULL)
    {
        printf("NULL token.\n");
        exit(1);
    }
    char *buf = (char *)malloc(10000); /* Buffer for this whole token. */
    if (buf == NULL)
    {
        printf("Out of memory on debug print.\n");
        exit(1);
    }

    char *buf_arg = NULL; /* Buffer for sub-tokens. */
    if (token->arg_tokens != NULL)
    {
        buf_arg = (char *)malloc(10000);
        if (buf_arg == NULL)
        {
            printf("Out of memory on debug print.\n");
            exit(1);
        }
        *buf_arg = '\0';
        sap_token *next = token->arg_tokens - 1;
        do
        {
            next++;
            char *sub_token = _sap_debug_token2text(*next);
            strcat(buf_arg, sub_token);
            strcat(buf_arg, ", ");
            free(sub_token);
        } while ((*next)->type != _SAP_END_OF_STMT);
    }
    sprintf(buf, "{Token type=%d, negate=%d, token name=%s, token val=%s, arguments=[%s]}",
            token->type,
            token->negate,
            token->name == NULL ? "NULL" : token->name,
            sap_num2str(token->val),
            buf_arg == NULL ? "NULL" : buf_arg);
    free(buf_arg);
    return buf;
}

/* Print a valid token array that ends with _SAP_END_OF_STMT */
void _sap_debug_print_token_arr(sap_token *token)
{
    sap_token *ptr64 = token;
    while ((*ptr64)->type != _SAP_END_OF_STMT)
    {
        printf("[Parser Debugger] Listing token: %s\n", _sap_debug_token2text(*ptr64));
        ptr64++;
    }
}