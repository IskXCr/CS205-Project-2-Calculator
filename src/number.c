/* This file serves as the library for Simple Arithmetic Program, aka SAP.

   Number.c implements arbitrary precision number arithmetics, along with
   some common operations that can be performed on those numbers.

   This library also provides utilities for outputting numbers and reading numbers.

   The major reference for this library is GNU bc program. The library
   may be seen as a subset for the library used in GNU bc. However, the implementation is completely rewritten
   to satisfy the specific purpose (as a simple subset). */

#include "number.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Common macros */

#ifdef MIN
#undef MIN
#endif
#define MIN(a, b) (((a) <= (b)) ? (a) : (b))

#ifdef MAX
#undef MAX
#endif
#define MAX(a, b) (((a) >= (b)) ? (a) : (b))

/* Global constants */

sap_num _zero_;
sap_num _one_;
sap_num _two_;

/* Negate the sign and return */
static sign _sap_negate(sign op) { return op == POS ? NEG : POS; }

/* Normalize the operand after operation. */
static void _sap_normalize(sap_num op);

/* Call this before initialize the whole library. */
void sap_init_lib(void)
{
    _zero_ = sap_new_num(1, 0);
    _one_ = sap_new_num(1, 0);
    _one_->n_val[0] = 1;
    _two_ = sap_new_num(1, 0);
    _two_->n_val[0] = 2;
}

static sap_num _sap_free_list = NULL; /* This linked list is used to prevent frequent malloc() operation and
                                         facilitate the reuse of the structure. */

/* new_num allocates a number and sets fields to known values. Initially it is 0.
   The storage allocated for n_ptr is initialized and the fields are all set to 0. */
sap_num sap_new_num(int length, int scale)
{
    sap_num tmp;

    if (_sap_free_list != NULL)
    {
        tmp = _sap_free_list;
        _sap_free_list = _sap_free_list->n_next;
    }
    else
    {
        tmp = (sap_num)malloc(sizeof(sap_struct));
        if (tmp == NULL)
            out_of_memory();
    }

    tmp->n_sign = POS;
    tmp->n_refs = 1;
    tmp->n_len = length;
    tmp->n_scale = scale;
    tmp->n_ptr = (char *)malloc(length + scale);
    if (tmp->n_ptr == NULL)
        out_of_memory();
    tmp->n_val = tmp->n_ptr;
    memset(tmp->n_ptr, 0, length + scale);
}

/* Free the number from the caller's prospective. Struct is reused, but the underlying storage for number is released. */
void sap_free_num(sap_num *op)
{
    if (*op == NULL)
        return;
    (*op)->n_refs--;
    if ((*op)->n_refs == 0)
    {
        if ((*op)->n_ptr != NULL)
            free((*op)->n_ptr);
        (*op)->n_next = _sap_free_list;
        _sap_free_list = (*op);
    }
    *op = NULL;
}

/* Make a copy of the number by solely increasing the reference count. */
sap_num sap_copy_num(sap_num src)
{
    src->n_refs++;
    return src;
}

/* Initialize a number by making it a copy of zero. */
void sap_init_num(sap_num *op)
{
    *op = sap_copy_num(_zero_);
}

/* Convert string to a number. Base 10 only.
   Invalid number representation will result in a 0 in return value. */
sap_num sap_str2num(char *ptr)
{
    char *ptr0 = ptr; /* For walking through the string */
    int n_len = 0;
    int n_scale = 0;
    int zero_int = 0;

    if (ptr0 == NULL || *ptr0 == '\0')
        return sap_copy_num(_zero_);

    /* Check validity of the number */
    if (*ptr0 == '+' || *ptr0 == '-')
        ptr0++;
    while (*ptr0 == '0') /* Skipping leading zeroes */
        ptr0++;
    while (isdigit(*ptr0))
        ptr0++, n_len++;
    if (*ptr0 == '.')
        ptr0++;
    while (isdigit(*ptr0))
        ptr0++, n_scale++;
    if (*ptr0 != '\0' || (n_len + n_scale) == 0)
        return sap_copy_num(_zero_);
    /* In case that the integral part is zero, we should still reserve the integral part in the storage. */
    if (n_len == 0)
    {
        n_len = 1;
        zero_int = 1;
    }

    /* Starting converting */
    sap_num tmp = sap_new_num(n_len, n_scale);
    ptr0 = ptr;
    char *ptrn = tmp->n_val;
    if (*ptr0 == '+' || *ptr0 == '-')
    {
        if (*ptr0 == '-')
            tmp->n_sign = NEG;
        ptr0++;
    }
    while (*ptr0 == '0')
        ptr0++;
    if (zero_int)
        *ptrn++ = 0;
    while (isdigit(*ptr0))
        *ptrn++ = *ptr0++ - '0';
    if (*ptr0 == '.')
        ptr0++;
    while (isdigit(*ptr0))
        *ptrn++ = *ptr0++ - '0';
    return tmp;
}

/* Convert the number to string represented by a char array terminating with '\0'.
   The caller must call free() on the char pointer after usage. */
char *sap_num2str(sap_num op)
{
    if (op == NULL || sap_is_zero(op))
    {
        char *tmp = (char *)malloc(2);
        *tmp = '0';
        *(tmp + 1) = '\0';
        return tmp;
    }
    char *tmp = (char *)malloc((op->n_sign == NEG ? 1 : 0) + op->n_len + (op->n_scale <= 0 ? 0 : 1) + op->n_scale + 1);
    if (tmp == NULL)
        out_of_memory();

    /* Start copying. */
    char *buf = tmp, *ptr = op->n_val; /* buf for placing the character, ptr for walking through the digits */
    if (op->n_sign == NEG)
        *buf++ = '-';
    for (int i = 0; i < op->n_len; ++i)
        *buf++ = *ptr++ + '0';
    if (op->n_scale > 0)
    {
        *buf++ = '.';
        for (int i = 0; i < op->n_scale; ++i)
            *buf++ = *ptr++ + '0';
    }
    *buf++ = '\0';
    return tmp;
}

/* Return TRUE if the number is zero. */
int sap_is_zero(sap_num op)
{
    char *ptr = op->n_val;
    for (int i = 0; i < op->n_len + op->n_scale; ++i)
        if (*(ptr + i) != 0)
            return FALSE;
    return TRUE;
}

/* Return TRUE IFF the operand has only 1 digit after zero. Determined by scale. */
int sap_is_near_zero(sap_num op, int scale)
{
    int end = MIN(op->n_scale, scale);
    if (op->n_scale >= scale)
        if (*(op->n_val + op->n_len + scale - 1) > 1)
            return FALSE;
    for (int i = 0; i < end - 1; ++i)
        if (*(op->n_val + op->n_len + i) != 0)
            return FALSE;
    return TRUE;
}

/* Return TRUE IFF the operand is negative. 0 is positive. */
int sap_is_neg(sap_num op)
{
    return !sap_is_zero(op) && (op->n_sign == NEG);
}

/* Test if two numbers are equal. These numbers don't have to be normalized. */
static int _sap_is_equal(sap_num op1, sap_num op2, int use_sign)
{
    if ((use_sign && op1->n_sign != op2->n_sign))
        return FALSE;

    /* Compare the integral part first. */
    char *ptr1 = op1->n_val; /* For limiting the leading zero */
    char *ptr2 = op2->n_val; /* For limiting the leading zero */
    while (*ptr1 == 0 && ptr1 < op1->n_val + op1->n_len)
        ptr1++;
    while (*ptr2 == 0 && ptr2 < op2->n_val + op2->n_len)
        ptr2++;
    char *ptr1b = op1->n_val + op1->n_len - 1; /* For the end of the integral part of op1 */
    char *ptr2b = op2->n_val + op2->n_len - 1; /* For the end of the integral part of op2 */
    for (; ptr1b >= ptr1 && ptr2b >= ptr2; ptr1b--, ptr2b--)
        if (*ptr1b != *ptr2b)
            return FALSE;
    /* Test if there are extra digits not processed. */
    if ((ptr1b < ptr1) ^ (ptr2b < ptr2))
        return FALSE;

    /* Compare the fractional part. */
    ptr1 = op1->n_val + op1->n_len + op1->n_scale - 1; /* For limiting the trailing zero */
    ptr2 = op2->n_val + op2->n_len + op2->n_scale - 1; /* For limiting the trailing zero */
    while (*ptr1 == 0 && ptr1 >= op1->n_val + op1->n_len)
        ptr1--;
    while (*ptr2 == 0 && ptr2 >= op2->n_val + op2->n_len)
        ptr2--;
    ptr1b = op1->n_val + op1->n_len; /* For the start of the fractional part of op1 */
    ptr2b = op2->n_val + op2->n_len; /* For the start of the fractional part of op2 */
    for (; ptr1b <= ptr1 && ptr2b <= ptr2; ptr1b++, ptr2b++)
        if (*ptr1b != *ptr2b)
            return FALSE;
    if ((ptr1b > ptr1) ^ (ptr2b > ptr2))
        return FALSE;
    return TRUE;
}

/* Test if op1 < op2. These numbers don't have to be normalized. */
static int _sap_is_less_than(sap_num op1, sap_num op2, int use_sign)
{
    if (_sap_is_equal(op1, op2, use_sign))
        return FALSE;
    if (use_sign)
    {
        if (op1->n_sign == NEG && op2->n_sign == POS)
            return TRUE;
        if (op1->n_sign == POS && op2->n_sign == NEG)
            return FALSE;
    }

    /* Now the two numbers are of the same the sign. */
    /* Compare the integral part first. */
    char *ptr1 = op1->n_val; /* Skipping leading zeroes */
    char *ptr2 = op2->n_val; /* Skipping leading zeroes */
    while (*ptr1 == 0 && ptr1 < op1->n_val + op1->n_len)
        ptr1++;
    while (*ptr2 == 0 && ptr2 < op2->n_val + op2->n_len)
        ptr2++;
    char *ptr1b = op1->n_val + op1->n_len - 1; /* For the end of the integral part of op1 */
    char *ptr2b = op2->n_val + op2->n_len - 1; /* For the end of the integral part of op2 */

    /* Compare the number of significant integral digits first */
    if (ptr1b - ptr1 != ptr2b - ptr2)
        if (ptr1b - ptr1 < ptr2b - ptr2)
            return !use_sign || op1->n_sign == POS;
        else if (ptr1b - ptr1 > ptr2b - ptr2)
            return use_sign && op1->n_sign == NEG;

    /* Now the length of the integral part are the same. */
    /* Compare the digits */
    for (; ptr1 <= ptr1b; ptr1++)
        if (*ptr1 < *ptr2)
            return !use_sign || op1->n_sign == POS;
        else if (*ptr1 > *ptr2)
            return use_sign && op1->n_sign == NEG;

    /* Now the integral parts are the same. */
    /* Compare fractional part. */
    ptr1 = op1->n_val + op1->n_len + op1->n_scale - 1; /* For limiting the trailing zero */
    ptr2 = op2->n_val + op2->n_len + op2->n_scale - 1; /* For limiting the trailing zero */
    while (*ptr1 == 0 && ptr1 >= op1->n_val + op1->n_len)
        ptr1--;
    while (*ptr2 == 0 && ptr2 >= op2->n_val + op2->n_len)
        ptr2--;
    ptr1b = op1->n_val + op1->n_len; /* For the start of the fractional part of op1 */
    ptr2b = op2->n_val + op2->n_len; /* For the start of the fractional part of op2 */

    /* Comparing significant digits */
    for (; ptr1b <= ptr1 && ptr2b <= ptr2; ptr1b++, ptr2b++)
        if (*ptr1b < *ptr2b)
            return !use_sign || op1->n_sign == POS;
        else if (*ptr1b > *ptr2b)
            return use_sign && op1->n_sign == NEG;

    if ((ptr1b > ptr1) ^ (ptr2b > ptr2))
        if (ptr1b > ptr1)
            return !use_sign || op1->n_sign == POS;
        else
            return use_sign && op1->n_sign == NEG;
    return TRUE;
}

/* Internal implementation for comparing numbers, supports comparison without the sign. */
static _sap_compare_impl(sap_num op1, sap_num op2, int use_sign)
{
    if (_sap_is_equal(op1, op2, use_sign))
        return 0;
    else if (_sap_is_less_than(op1, op2, use_sign))
        return -1;
    else
        return 1;
}

/* Compare two numbers, return -1 if op1 < op2, 0 if op1 == op2 and 1 if op1 > op2. */
int sap_compare(sap_num op1, sap_num op2)
{
    return _sap_compare_impl(op1, op2, TRUE);
}

/* Internal implementation for adding two numbers, assuming they have the same sign.
   Requires scale_min to add extra digits for the result. The sign of the result is specified. */
static sap_num _sap_add_impl(sap_num op1, sap_num op2, int scale_min, sign op_sign)
{
    int len = MAX(op1->n_len, op2->n_len);
    int scale = MAX(op1->n_scale, op2->n_scale);
    sap_num tmp = sap_new_num(len + 1, MAX(scale, scale_min));
    tmp->n_sign = op_sign;

    /* Fill trailing zeroes. */
    if (scale_min > scale)
        for (int i = tmp->n_len + scale; i < tmp->n_len + scale_min; ++i)
            *(tmp->n_val + i) = 0;

    /* Copying the fractional part. The one with longer scale first. */
    sap_num wop = (scale == op1->n_scale) ? op1 : op2; /* The working operand. */
    for (int i = scale - 1; i >= 0; --i)
        *(tmp->n_val + tmp->n_len + i) = *(wop->n_val + wop->n_len + i);

    /* Then perform the addition. */
    char carry = 0; /* The carry for addition. */
    wop = (scale == op1->n_scale) ? op2 : op1;
    for (int i = wop->n_scale - 1; i >= 0; --i)
    {
        char *p = tmp->n_val + tmp->n_len + i;
        *p += *(wop->n_val + wop->n_len + i) + carry;
        if (*p >= 10)
        {
            *p -= 10;
            carry = 1;
        }
        else
            carry = 0;
    }

    /* Copying the integral part. The one with longer len first, starting from the LSB. */
    wop = (len == op1->n_len) ? op1 : op2;
    for (int i = 0; i < wop->n_len; ++i)
        *(tmp->n_val + tmp->n_len - i - 1) = *(wop->n_val + wop->n_len - i - 1);

    /* Count the previous carry from the fractional part. */
    if (carry == 1)
        *(tmp->n_val + tmp->n_len - 1) += 1;

    /* Perform the addition. */
    carry = 0;
    wop = (len == op1->n_len) ? op2 : op1;
    for (int i = 0; i < wop->n_len; ++i)
    {
        char *p = tmp->n_val + tmp->n_len - i - 1;
        *p += *(wop->n_val + wop->n_len - i - 1) + carry;
        if (*p >= 10)
        {
            *p -= 10;
            carry = 1;
        }
        else
            carry = 0;
    }
    if (carry == 1)
        *(tmp->n_val + tmp->n_len - wop->n_len - 1) += 1;
    _sap_normalize(tmp);
    return tmp;
}

/* Internal implementation for subtracting abs(op2) from abs(op1), assuming (op1) > (op2). The sign is determined by op_sign. */
static sap_num _sap_sub_impl(sap_num op1, sap_num op2, int scale_min, sign op_sign)
{
    int len = MAX(op1->n_len, op2->n_len);
    int scale = MAX(op1->n_scale, op2->n_scale);
    sap_num tmp = sap_new_num(len, MAX(scale, scale_min));
    tmp->n_sign = op_sign;

    /* Fill trailing zeroes. */
    if (scale_min > scale)
        for (int i = tmp->n_len + scale; i < tmp->n_len + scale_min; ++i)
            *(tmp->n_val + i) = 0;

    /* Copying the larger one. */
    for (int i = op1->n_scale - 1; i >= 0; --i)
        *(tmp->n_val + tmp->n_len + i) = *(op1->n_val + op1->n_len + i);
    for (int i = 0; i < op1->n_len; ++i)
        *(tmp->n_val + tmp->n_len - i - 1) = *(op1->n_val + op1->n_len - i - 1);

    /* Start subtracting. */
    char borrow = 0;
    for (int i = op2->n_scale - 1; i >= 0; --i)
    {
        char *p = tmp->n_val + tmp->n_len + i;
        char *q = op2->n_val + op2->n_len + i;
        if (*p < *q + borrow)
        {
            *p += 10 - *q - borrow;
            borrow = 1;
        }
        else
        {
            *p -= *q + borrow;
            borrow = 0;
        }
    }

    for (int i = 0; i < op2->n_len; ++i)
    {
        char *p = tmp->n_val + tmp->n_len - i - 1;
        char *q = op2->n_val + op2->n_len - i - 1;
        if (*p < *q + borrow)
        {
            *p += 10 - *q - borrow;
            borrow = 1;
        }
        else
        {
            *p -= *q + borrow;
            borrow = 0;
        }
    }
    if (borrow >= 1)
        if (op2->n_len < tmp->n_len)
            *(tmp->n_val + tmp->n_len - op2->n_len - 1) -= borrow;
        else
            sap_warn("Internal error: subtraction_impl performed on invalid operands.");
    _sap_normalize(tmp);
    return tmp;
}

/* Add two numbers. */
sap_num sap_add(sap_num op1, sap_num op2, int scale_min)
{
    if (op1->n_sign == op2->n_sign)
        return _sap_add_impl(op1, op2, scale_min, op1->n_sign);

    sap_num tmp;
    if (op1->n_sign == NEG)
        tmp = op1, op1 = op2, op2 = tmp;

    /* Now op1 is positive and op2 is negative. */
    switch (_sap_compare_impl(op1, op2, FALSE))
    {
    case -1:
        return _sap_sub_impl(op2, op1, scale_min, NEG);
    case 0:
        return sap_copy_num(_zero_);
    case 1:
        return _sap_sub_impl(op1, op2, scale_min, POS);
    }
}

/* Subtract two numbers. */
sap_num sap_sub(sap_num op1, sap_num op2, int scale_min)
{
    if (op1->n_sign != op2->n_sign)
        return _sap_add_impl(op1, op2, scale_min, op1->n_sign);

    switch (_sap_compare_impl(op1, op2, FALSE))
    {
    case -1:
        return _sap_sub_impl(op2, op1, scale_min, _sap_negate(op1->n_sign));
    case 0:
        return sap_copy_num(_zero_);
    case 1:
        return _sap_sub_impl(op1, op2, scale_min, op1->n_sign);
    }
}

/* Internal implementation for multiplying two numbers. */
static sap_num _sap_mul_impl(sap_num op1, sap_num op2, int scale)
{
    
}