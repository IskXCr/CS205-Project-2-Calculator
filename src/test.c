/* Source file for test routines. */
#include "test.h"
#include "number.h"
#include <stdio.h>


void test(void)
{
    sap_init_lib();
    sap_num n1 = sap_str2num("-2.3");
    sap_num n2 = sap_str2num("7");
    printf("Op1: %s\n", sap_num2str(n1));
    printf("Op2: %s\n", sap_num2str(n2));
    printf("Div: %s\n", sap_num2str(sap_div(n1, n2, 4)));
    // printf("sqrt(op1): %s\n", sap_num2str(sap_sqrt(n1, 9)));
    printf("Pow(-2.3, 7): %s\n", sap_num2str(sap_raise(n1, n2, 1)));
} 