/* The source file for the virtual machine of the SAP program */
#include "sapdefs.h"
#include "sap.h"

/* Common macros */

#ifdef MIN
#undef MIN
#endif
#define MIN(a, b) (((a) <= (b)) ? (a) : (b))

#ifdef MAX
#undef MAX
#endif
#define MAX(a, b) (((a) >= (b)) ? (a) : (b))

#define _TRANS_FUNC_MIN_SCALE 3

/* Global constants */
static lut_table symbols;

/* Functions */

/* Handle common exceptions occurred.
   The function that have called this will by themselves free the resource if required. */
static void handle(void)
{
    // do nothing.
    if (debug)
        printf("Internal exception.\n");
    return;
}

/* Initialize the whole sap library. This function can be called only once. */
void sap_init_lib(void)
{
    utils_init_lib(&handle);
    sap_init_number_lib();

    symbols = lut_new_table();
}

/* Convert an infix expression to postfix.
   The resultant array contains sap_token in parameter=tokens, or is NULL if the expression is invalid.
   The array must be freed manually.
   Reference: https://www.geeksforgeeks.org/infix-to-postfix-using-different-precedence-values-for-in-stack-and-out-stack */
static sap_token *_sap_to_postfix(sap_token *tokens)
{
    sap_token *result;  /* Store the final result. */
    sap_token *resultr; /* Result, but reallocated */
    sap_token *rptr;    /* Points to the next free position in the result array. */
    sap_token *tptr;    /* Points to the current position in the tokens array. */
    int len;            /* The length of result array. */
    int rlen = 0;       /* The actual length without parentheses. For future realloc(). */
    stack stk;          /* Token stack. */

    /* Initializations */
    len = sap_get_token_arr_length(tokens);
    rptr = result = (sap_token *)malloc(len * sizeof(sap_token));
    if (result == NULL)
        out_of_memory();
    tptr = tokens;
    stk = sap_new_stack();
    sap_stack_push(stk, sap_get_sentinel());

    /* While the tokens array is not empty */
    while ((*tptr)->type != _SAP_END_OF_STMT)
    {
        // debug
        // printf("[Postfix] Processing token: %s\n", _sap_debug_token2text(*tptr));
        rlen++;
        if (sap_is_operand(*tptr)) /* If the input is an operand */
            *rptr++ = *tptr;
        else if (sap_is_operator(*tptr)) /* If the input is an operator */
        {
            if (sap_stack_empty(stk) || sap_get_out_prec(*tptr) > sap_get_in_prec(sap_stack_top(stk)))
                sap_stack_push(stk, *tptr);
            else
            {
                while (sap_stack_has_element(stk) && sap_get_out_prec(*tptr) < sap_get_in_prec(sap_stack_top(stk)))
                    *rptr++ = sap_stack_pop(stk);
                sap_stack_push(stk, *tptr);
            }
        }
        else if ((*tptr)->type == _SAP_PAREN_R) /* If the input is right parenthese */
        {
            /* Pop the elements while the top of the stack is not _SAP_STACK_SENTINEL */
            while (sap_stack_top(stk)->type != _SAP_PAREN_L && sap_stack_top(stk)->type != _SAP_STACK_SENTINEL)
                *rptr++ = sap_stack_pop(stk);

            if (sap_stack_top(stk)->type == _SAP_STACK_SENTINEL)
            {
                sap_warn("Invalid postfix expression with missing parenthese.. No result is returned. ", 0);

                if (debug)
                {
                    printf("[Debug-Postfix] Showing tokens: \n");
                    _sap_debug_print_token_arr(tokens);
                }
                free(result);
                sap_free_stack(&stk);
                return NULL;
            }
            sap_stack_pop(stk);
        }
        tptr++;
    }

    /* Pop the remaining operators */
    while (sap_stack_has_element(stk) && sap_stack_top(stk)->type != _SAP_STACK_SENTINEL)
        *rptr++ = sap_stack_pop(stk);

    sap_free_stack(&stk);

    /* Clean up */
    *rptr = *tptr; /* IMPORTANT: _SAP_END_OF_STMT placed. */
    resultr = (sap_token *)realloc(result, (rlen + 1) * sizeof(sap_token));
    if (resultr != NULL) /* If successfully reallocated */
        result = resultr;

    return result;
}

static sap_num _sap_evaluate(sap_token **tokens);

/* A simple routine for evaluating a **variable** or a **number** node to an actual value, performing the possible negate.
   This routine is used as an evaluation of the operand. If the operand cannot be evaluated, return NULL.
   The returned pointer is the token itself. */
static sap_token _sap_evaluate_operand(sap_token token)
{
    if (!sap_is_operand(token))
    {
        sap_warn("SAP error: operand cannot be evaluated.", 0);
        return NULL;
    }

    if (token->type == _SAP_NUMBER)
    {

    }
    else if (token->type == _SAP_VARIABLE)
    {
        sap_num res = lut_find(symbols, token->name);
        if (res == NULL)
            sap_token_trans2num(token, _zero_);
        else
        {
            sap_token_trans2num(token, res);
            sap_free_num(&res);
        }
    }
    else if (token->type == _SAP_SUB_EXPR)
    {
        sap_num res = _sap_evaluate(&(token->arg_tokens));
        if (res == NULL)
        {
            sap_warn("Invalid sub expression.", 0);
            sap_token_trans2num(token, _zero_);
        }
        else
        {
            sap_token_trans2num(token, res);
            sap_free_num(&res);
        }
    }
    else if (sap_is_func(token))
    {
        sap_num tmp0, tmp1; /* Temporary results */
        
        tmp0 = _sap_evaluate(&(token->arg_tokens));
        if (tmp0 == NULL)
        {
            sap_warn("Invalid arguments.", 0);
            tmp0 = sap_copy_num(_zero_);
        }
        switch (token->type)
        {
        case _SAP_SQRT:
            tmp1 = sap_sqrt(tmp0, tmp0->n_scale);
            break;
        case _SAP_SIN:
            tmp1 = sap_sin(tmp0, MAX(tmp0->n_scale, _TRANS_FUNC_MIN_SCALE));
            break;
        case _SAP_COS:
            tmp1 = sap_cos(tmp0, MAX(tmp0->n_scale, _TRANS_FUNC_MIN_SCALE));
            break;
        case _SAP_ARCTAN:
            tmp1 = sap_arctan(tmp0, MAX(tmp0->n_scale, _TRANS_FUNC_MIN_SCALE));
            break;
        case _SAP_LN:
            tmp1 = sap_ln(tmp0, MAX(tmp0->n_scale, _TRANS_FUNC_MIN_SCALE));
            break;
        case _SAP_EXP:
            tmp1 = sap_exp(tmp0, MAX(tmp0->n_scale, _TRANS_FUNC_MIN_SCALE));
            break;
        case _SAP_FUNC_CALL:
            /* Reserved for future function table invoke. */
        default:
            sap_warn("Unsupported operation", 0);
            tmp1 = sap_copy_num(tmp0);
        }
        /* Clean up*/
        sap_token_trans2num(token, tmp1);
        sap_free_num(&tmp0);
        sap_free_num(&tmp1);
    }

    if (token->negate) /* If the result should be negated */
    {
        sap_num tmp = sap_replicate_num(token->val);
        sap_free_num(&(token->val));
        token->val = tmp;
        sap_negate(token->val);
        token->negate = FALSE;
    }
    return token;
}

/* Evaluate an expression in tokens. Resource will be freed thereafter. (Intermediate resources are freed)
   Return NULL if an array of empty statement is passed as the argument, or when failed to parse the expression.
   Partial reference: https://www.engr.mun.ca/~theo/Misc/exp_parsing.htm */
static sap_num _sap_evaluate(sap_token **tokens)
{
    /* Skip some simple situations. */
    if ((**tokens)->type == _SAP_END_OF_STMT) /* If the end of statement is the only element, return. */
        return NULL;
    // if ((*tokens)->type == _SAP_VARIABLE && (*(tokens + 1))->type == _SAP_ASSIGN) /* If it is assignment */
    // {
    //     sap_token var = (*tokens);
    //     sap_num res = _sap_evaluate(tokens + 2);
    //     if (res == NULL)
    //     {
    //         sap_warn("Invalid assignment.", 0);
    //         res = sap_copy_num(_zero_);
    //     }
    //     lut_insert(symbols, var->name, res);
    //     return res;
    // }

    sap_token *ptr0;     /* Store the postfix equivalent of tokens. */
    sap_token *ptr;      /* Current position in the postfix tokens. */
    sap_num tmp0 = NULL; /* Used to store intermediate results. Usually this is the result. Should be freed in place. */
    sap_num tmp1 = NULL; /* Used to store intermediate results. Usually this is the left operand. Should be freed in place. */
    sap_num tmp2 = NULL; /* Used to store intermediate results. Usually this is the right operand. Should be freed in place. */
    stack stk;           /* Store operands and operators */

    /* Initialization */
    ptr = ptr0 = _sap_to_postfix(*tokens);
    if (ptr0 == NULL)
    {
        sap_warn("Evaluator: Failed the parse the expression to postfix.", 0);
        return NULL;
    }

    stk = sap_new_stack();
    sap_stack_push(stk, sap_get_sentinel());

    // debug
    if (debug)
    {
        printf("[Evaluator Debugger] Showing tokens: \n");
        _sap_debug_print_token_arr(*tokens);
    }

    while ((*ptr)->type != _SAP_END_OF_STMT)
    {
        if (sap_is_operand(*ptr))
            sap_stack_push(stk, *ptr);
        else /* An operator */
        {
            /* In the evaluation process, only one of the node is evaluated and updated. */
            /* ASSIGN is ignored. */
            if ((*ptr)->type == _SAP_ASSIGN)
            {
                sap_token tok_r = sap_stack_pop(stk);
                sap_token tok_l = sap_stack_pop(stk);
                if (tok_l->type != _SAP_VARIABLE)
                {
                    sap_warn("Assignment can only be made to a lvalue.", 0);
                    tmp2 = sap_copy_num(_zero_);
                    tmp0 = sap_copy_num(_zero_);
                }
                else if (tok_r == NULL)
                {
                    sap_warn("Right operand on assignment cannot be evaluated. Variable name: ", 1, tok_l->name, FALSE);
                    tmp2 = sap_copy_num(_zero_);
                    tmp0 = sap_copy_num(_zero_);
                }
                else
                {
                    _sap_evaluate_operand(tok_r);
                    tmp2 = sap_copy_num(tok_r->val);
                    tmp0 = sap_copy_num(tmp2);
                    lut_insert(symbols, tok_l->name, tmp2);
                }
            }
            else
            {
                sap_token tok2 = sap_stack_pop(stk);
                sap_token tok1 = sap_stack_pop(stk);
                if (tok1 != NULL && tok1->type != _SAP_STACK_SENTINEL && tok2 != NULL && tok2->type != _SAP_STACK_SENTINEL)
                {
                    tmp2 = sap_copy_num(_sap_evaluate_operand(tok2)->val);
                    tmp1 = sap_copy_num(_sap_evaluate_operand(tok1)->val);
                    switch ((*ptr)->type)
                    {
                    case _SAP_LESS:
                        tmp0 = sap_compare(tmp1, tmp2) == -1 ? sap_copy_num(_one_) : sap_copy_num(_zero_);
                        break;
                    case _SAP_GREATER:
                        tmp0 = sap_compare(tmp1, tmp2) == 1 ? sap_copy_num(_one_) : sap_copy_num(_zero_);
                        break;
                    case _SAP_EQ:
                        tmp0 = sap_compare(tmp1, tmp2) == 0 ? sap_copy_num(_one_) : sap_copy_num(_zero_);
                        break;
                    case _SAP_LEQ:
                        tmp0 = sap_compare(tmp1, tmp2) <= 0 ? sap_copy_num(_one_) : sap_copy_num(_zero_);
                        break;
                    case _SAP_GEQ:
                        tmp0 = sap_compare(tmp1, tmp2) >= 0 ? sap_copy_num(_one_) : sap_copy_num(_zero_);
                        break;
                    case _SAP_NEQ:
                        tmp0 = sap_compare(tmp1, tmp2) != 0 ? sap_copy_num(_one_) : sap_copy_num(_zero_);
                        break;

                    case _SAP_ADD:
                        tmp0 = sap_add(tmp1, tmp2, MAX(tmp1->n_scale, tmp2->n_scale));
                        break;
                    case _SAP_MINUS:
                        tmp0 = sap_sub(tmp1, tmp2, MAX(tmp1->n_scale, tmp2->n_scale));
                        break;
                    case _SAP_MULTIPLY:
                        tmp0 = sap_mul(tmp1, tmp2, MAX(tmp1->n_scale, tmp2->n_scale));
                        break;
                    case _SAP_DIVIDE:
                        tmp0 = sap_div(tmp1, tmp2, MAX(tmp1->n_scale, tmp2->n_scale));
                        break;
                    case _SAP_MODULO:
                        tmp0 = sap_mod(tmp1, tmp2, MAX(tmp1->n_scale, tmp2->n_scale));
                        break;
                    case _SAP_POWER:
                        tmp0 = sap_raise(tmp1, tmp2, MAX(tmp1->n_scale, tmp2->n_scale));
                        break;
                    default:
                        sap_warn("Unsupported operation", 0);
                        tmp0 = sap_copy_num(_zero_);
                        break;
                    }
                }
                else /* Invalid expression */
                {
                    sap_warn("Invalid expression.", 0);
                    tmp0 = sap_copy_num(_zero_);
                }
            }

            sap_token_trans2num(*ptr, tmp0);
            sap_free_num(&tmp0);
            sap_free_num(&tmp1);
            sap_free_num(&tmp2);
            tmp0 = NULL;
            tmp1 = NULL;
            tmp2 = NULL;

            /* Push the calculated result. */
            sap_stack_push(stk, *ptr);
        }
        ptr++;
    }

    sap_num result; /* Store the result */
    sap_token final = sap_stack_pop(stk);
    if (final != NULL && sap_is_operand(final))
        result = sap_copy_num(_sap_evaluate_operand(final)->val);
    else
    {
        sap_warn("Invalid expression.", 0);
        result = sap_copy_num(_zero_);
    }

    /* Clean up*/
cleanup:
    free(ptr0);
    sap_free_num(&tmp0);
    sap_free_num(&tmp1);
    sap_free_stack(&stk);
    sap_free_tokens(tokens);

    // debug
    if (debug)
    {
        char *p = sap_num2str(result);
        printf("[Evaluator Debugger] Result: %s\n", p);
        free(p);
    }

    return result;
}

/* Execute the statement and output the result produced. */
sap_num sap_execute(char *stmt)
{
    sap_token *tokens = sap_parse_expr(stmt);
    sap_num result = _sap_evaluate(&tokens);
    return result;
}

/* Reset the whole sap library. */
sap_num sap_reset_all()
{
    lut_free_table(&symbols);
    symbols = lut_new_table();
}