/* The source file for the virtual machine of the SAP program */
#include "sapdefs.h"
#include "sap.h"

/* Global constants */
static lut_table symbols;

/* Functions */

/* Handle common exceptions occurred. 
   The function that have called this will by themselves free the resource if required. */
static void handle(void)
{
    printf("Internal Exception occurred. Restoring to last state and releasing resources.\n");
}

/* Initialize the whole sap library. This function can be called only once. */
void sap_init_lib(void)
{
    utils_init_lib(&handle);
    sap_init_number_lib();
    
    symbols = lut_new_table();
}

/* Convert an infix expression to postfix. */
static sap_token *_sap_to_postfix(sap_token *tokens)
{
    sap_token *result = (sap_token *)malloc(sap_get_token_arr_length(tokens) * sizeof(sap_token));
}

/* Evaluate an expression in tokens. Will not free the resource thereafter. 
   Partial reference: https://www.engr.mun.ca/~theo/Misc/exp_parsing.htm */
static sap_num _sap_evaluate(sap_token *tokens)
{
    /* Skip some unimportant situations. */
    if ((*tokens)->type == _SAP_END_OF_STMT) /* If the end of statement is the only element, return. */
        return sap_copy_num(_zero_);
    

}

/* Execute the statement and output the result produced. */
sap_num sap_execute(char *stmt)
{
    sap_token *tokens = sap_parse_expr(stmt);
    sap_num result = _sap_evaluate(tokens);
    sap_free_tokens(tokens);
    return result;
}

/* Reset the whole sap library. */
sap_num sap_reset_all()
{
    lut_free_table(&symbols);
    symbols = lut_new_table();
}