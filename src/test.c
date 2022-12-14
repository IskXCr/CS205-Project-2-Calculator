/* Source file for test routines. */

#include "test.h"
#include "number.h"
#include "lut.h"
#include "parser.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

/* Function declarations */

static void test_number(void);

static void test_lut(void);

static void test_parser(void);

static void test_util_fetch_expr(void);

static void test_sap(void);

/* Perform all test routines. */
void test(void)
{
    test_number();
    test_lut();
    test_parser();
    test_util_fetch_expr();
    test_sap();
}

static void
test_number(void)
{
    sap_num n1 = sap_str2num("-1.03");
    sap_num n2 = sap_str2num("2.57");
    sap_num tmp; /* Intermediate result */
    char *p; /* For storing strings */

    p = sap_num2str(n1);
    printf("Op1: %s\n",p);
    free(p);
    
    p = sap_num2str(n2);
    printf("Op2: %s\n", p);
    free(p);

    tmp = sap_mul(n1, n2, 4);
    p = sap_num2str(tmp);
    sap_free_num(&tmp);
    printf("Mul: %s\n", p);
    free(p);

    tmp = sap_div(n1, n2, 4);
    p = sap_num2str(tmp);
    sap_free_num(&tmp);
    printf("Div: %s\n", p);
    free(p);

    tmp = sap_sqrt(n2, 9);
    p = sap_num2str(tmp);
    sap_free_num(&tmp);
    printf("sqrt(op2): %s\n", p);
    free(p);

    tmp = sap_raise(n1, n2, 1);
    p = sap_num2str(tmp);
    sap_free_num(&tmp);
    printf("Pow(op1, op2): %s\n", p);
    free(p);

    sap_free_num(&n1);
    sap_free_num(&n2);
}

static void
test_lut(void)
{
    lut_table table = lut_new_table();
    printf("Table newed.\n");
    sap_num one = sap_copy_num(_one_);
    sap_num two = sap_copy_num(_two_);
    sap_num e = sap_copy_num(_e_);
    sap_num pi = sap_copy_num(_pi_);
    lut_insert(table, "xy", one);
    lut_insert(table, "yz", two);
    lut_insert(table, "pi", pi);
    lut_insert(table, "e", e);
    sap_num tmp;
    if ((tmp = lut_find(table, "xy")) != NULL)
        printf("xy = %s\n", sap_num2str(tmp));
    if ((tmp = lut_find(table, "yz")) != NULL)
        printf("yz = %s\n", sap_num2str(tmp));
    if ((tmp = lut_find(table, "pi")) != NULL)
        printf("pi = %s\n", sap_num2str(tmp));
    if ((tmp = lut_find(table, "e")) != NULL)
        printf("e = %s\n", sap_num2str(tmp));
    if ((tmp = lut_find(table, "ux")) == NULL)
        printf("ux = Not exist\n");

    lut_free_table(&table);
    printf("Table freed.\n");

    table = lut_new_table();
    printf("Second table newed.\n");
    lut_insert(table, "xy", one);
    lut_insert(table, "yz", two);
    lut_insert(table, "pi", pi);
    lut_insert(table, "e", e);
    if ((tmp = lut_find(table, "xy")) != NULL)
        printf("xy = %s\n", sap_num2str(tmp));
    if ((tmp = lut_find(table, "yz")) != NULL)
        printf("yz = %s\n", sap_num2str(tmp));
    if ((tmp = lut_find(table, "pi")) != NULL)
        printf("pi = %s\n", sap_num2str(tmp));
    if ((tmp = lut_find(table, "e")) != NULL)
        printf("e = %s\n", sap_num2str(tmp));
    if ((tmp = lut_find(table, "ux")) == NULL)
        printf("ux = Not exist\n");

    lut_free_table(&table);
    printf("Table freed.\n");
}

static void
test_parser(void)
{
    char *exp = "sqrt(x + 3) + sin(y = 7)\n";
    printf("Parse expression: %s", exp);
    sap_token *array = sap_parse_expr(exp);
    printf("Result:\n");
    array--;
    do
    {
        array++;
        char *p = _sap_debug_token2text(*array);
        printf("%s\n", p);
        free(p);
    } while ((*array)->type != _SAP_END_OF_STMT);
}

static void
test_util_fetch_expr(void)
{
    char *p = {"parse3567;6391;xprdc\n0\n"};
    printf("Testing expression fetching:\n%s", p);
    char **ptr0 = fetch_expr(p);
    char **ptr = ptr0;
    while (*ptr != NULL)
    {
        printf("Token: {%s}\n", *ptr);
        ptr++;
    }
}

static void
test_sap(void)
{
}
