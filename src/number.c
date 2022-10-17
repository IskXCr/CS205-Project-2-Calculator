/* This file serves as the library for Simple Arithmetic Program, aka SAP.

   Number.c implements arbitrary precision number arithmetics, along with
   some common operations that can be performed on those numbers.

   This library also provides utilities for outputting numbers and reading numbers.

   The major reference for this library is GNU bc program. A part of this library
   may be seen as a subset for the library used in GNU bc. However, the implementation is completely written from scratch
   to satisfy the specific purpose (as a simple subset and include some math functions). */

#include "number.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

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
sap_num _e_;  /* Math constant with limited precision. */
sap_num _pi_; /* Math constant with limited precision. */

/* Negate the sign and return */
static sign _sap_negate(sign op) { return op == POS ? NEG : POS; }

/* Normalize the operand after operation. */
static void _sap_normalize(sap_num op)
{
    if (op->n_ptr == NULL)
        return;

    char *ptr = op->n_ptr;                           /* Used to store the first nonzero digit of the integral part */
    for (int i = 0; i < op->n_len && *ptr == 0; ++i) /* Skipping leading zeroes. */
        ptr++;
    /* If it happens that the entire integral part is zero */
    if (ptr >= op->n_ptr + op->n_len)
    {
        char *p = (char *)malloc(1 + op->n_scale);
        if (p == NULL)
            out_of_memory();
        *p = 0;
        memcpy(p + 1, op->n_ptr + op->n_len, op->n_scale);
        free(op->n_ptr);
        op->n_len = 1;
        op->n_val = op->n_ptr = p;
    }
    else /* There is at least one nonzero digit */
    {
        int len = op->n_len - (ptr - op->n_ptr);
        char *p = (char *)malloc(len + op->n_scale);
        if (p == NULL)
            out_of_memory();
        memcpy(p, ptr, len);
        memcpy(p + len, op->n_ptr + op->n_len, op->n_scale);
        free(op->n_ptr);
        op->n_len = len;
        op->n_val = op->n_ptr = p;
    }
}

/* Truncate the number to scale. */
static void _sap_truncate(sap_num op, int scale, int round)
{
    if (op->n_ptr == NULL)
        return;
    if (op->n_scale <= scale)
        return;

    char *ptr = op->n_ptr;
    char *new_ptr = (char *)malloc(op->n_len + scale);
    if (new_ptr == NULL)
        out_of_memory();
    memcpy(new_ptr, ptr, op->n_len + scale);
    if (round && *(ptr + op->n_len + scale) >= 5)
        (*(new_ptr + op->n_len + scale - 1))++;
    free(ptr);
    op->n_scale = scale;
    op->n_ptr = op->n_val = new_ptr;
}

/* Get a replicate of the number, mainly for thread safety. */
sap_num sap_replicate_num(sap_num op)
{
    sap_num tmp = sap_new_num(op->n_len, op->n_scale);
    tmp->n_sign = op->n_sign;
    memcpy(tmp->n_ptr, op->n_val, op->n_len + op->n_scale);
    return tmp;
}

/* Initialize the whole number library.
   This function must be called only once during the execution or memory leak may occur. */
void sap_init_number_lib(void)
{
    _zero_ = sap_new_num(1, 0);
    _one_ = sap_new_num(1, 0);
    *(_one_->n_val) = 1;
    _two_ = sap_new_num(1, 0);
    *(_two_->n_val) = 2;
    _e_ = sap_str2num("2.71828182845904523536");
    _pi_ = sap_str2num("3.14159265358979323846");
}

static sap_num _sap_free_list = NULL; /* This linked list is used to prevent frequent malloc() operation and
                                         facilitate reuses of the structure. */

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
    return tmp;
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

/* This function converts double to its closest sap_num. Caution: NaN and Inf are not supported. */
sap_num sap_double2num(double val)
{
    char *p = (char *)malloc(300);
    if (p == NULL)
        out_of_memory();
    sprintf(p, "%lf", val);
    sap_num tmp = sap_str2num(p);
    free(p);
    return tmp;
}

/* This function converts int to its sap_num equivalent. */
sap_num sap_int2num(int val)
{
    char *p = (char *)malloc(30);
    if (p == NULL)
        out_of_memory();
    sprintf(p, "%d", val);
    sap_num tmp = sap_str2num(p);
    free(p);
    return tmp;
}

/* Convert the number to string represented by a char array terminating with '\0'.
   The caller must call free() on the char pointer after usage. */
char *sap_num2str(sap_num op)
{
    char *tmp;
    if (op == NULL || sap_is_zero(op))
    {
        tmp = (char *)malloc(2);
        if (tmp == NULL)
            out_of_memory();
        *tmp = '0';
        *(tmp + 1) = '\0';
        return tmp;
    }
    tmp = (char *)malloc((op->n_sign == NEG ? 1 : 0) + op->n_len + (op->n_scale <= 0 ? 0 : 1) + op->n_scale + 1);
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
    *buf = '\0';
    return tmp;
}

/* Convert the number to its closest double equivalent. */
double sap_num2double(sap_num op)
{
    char *p = sap_num2str(op);
    double val;
    sscanf(p, "%lf", &val);
    free(p);
    return val;
}

/* Convert the number to its closest int equivalent. */
int sap_num2int(sap_num op)
{
    char *p = sap_num2str(op);
    int val;
    sscanf(p, "%d", &val);
    free(p);
    return val;
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
    for (; ptr1 <= ptr1b; ptr1++, ptr2++)
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
static int _sap_compare_impl(sap_num op1, sap_num op2, int use_sign)
{
    if (op1 == op2)
        return 0;
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

/* Negate the operand. */
void sap_negate(sap_num op)
{
    op->n_sign = _sap_negate(op->n_sign);
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
    for (int i = wop->n_scale - 1; i >= 0; --i)
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
        {
            sap_free_num(&tmp);
            sap_warn("Internal error: subtraction_impl performed on invalid operands: ", 3, sap_num2str(op1), " and ", sap_num2str(op2));
        }
    _sap_normalize(tmp);
    return tmp;
}

/* Add two numbers and return a new number as the result. */
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

/* Subtract two numbers and return a new number as the result. */
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

/* Left shift the number in 10's base. Negative shift value indicates to do right shift. */
static sap_num _sap_shift(sap_num op, int shift)
{
    int len = op->n_len;
    int scale = op->n_scale;

    if (shift == 0)
        return sap_copy_num(op);
    else if (shift < 0)
    {
        /* Process the length */
        shift = -shift;
        len -= shift;
        if (len <= 0)
            len = 1;
        scale += shift;

        /* Copying the number and return. */
        sap_num tmp = sap_new_num(len, scale);
        tmp->n_sign = op->n_sign;
        for (int i = 0; i < op->n_len + op->n_scale; ++i)
            *(tmp->n_val + tmp->n_len + tmp->n_scale - i - 1) = *(op->n_val + op->n_len + op->n_scale - i - 1);
        return tmp;
    }
    else
    {
        /* Process the length */
        scale -= shift;
        if (scale <= 0)
            scale = 0;
        len += shift;

        /* Copying the number and return. */
        sap_num tmp = sap_new_num(len, scale);
        for (int i = 0; i < op->n_len + op->n_scale; ++i)
            *(tmp->n_val + i) = *(op->n_val + i);
        return tmp;
    }
}

/* Internal simple multiplication for handling small numbers. Both of the operands are assumed positive integers. */
static sap_num _sap_simple_mul(sap_num op1, sap_num op2)
{
    int len = op1->n_len + op2->n_len;
    sap_num result = sap_new_num(len, 0);

    /* Simulate hand multiplication. */
    for (int i = 0; i < op2->n_len; ++i)
    {
        for (int j = 0; j < op1->n_len; ++j)
        {
            char *p = result->n_val + result->n_len - i - j - 1; /* The target position */
            char *vp = op2->n_val + op2->n_len - i - 1;
            char *vs = op1->n_val + op1->n_len - j - 1;
            *p += *vp * *vs;
            if (*p >= 10)
            {
                *(p - 1) += *p / 10;
                *p %= 10;
            }
        }
    }
    _sap_normalize(result);
    return result;
}

/* Decompose a positive integer into the form x1 * B^m + x0.  (^ denotes power here) */
static void _sap_karatsuba_decomp(sap_num op1, sap_num *x1, sap_num *x0, int m)
{
    int llen = op1->n_len - m; /* The length of x1. */
    *x1 = sap_new_num(llen, 0);
    *x0 = sap_new_num(m, 0);
    memcpy((*x1)->n_val, op1->n_val, llen);
    memcpy((*x0)->n_val, op1->n_val + llen, m);
}

#define _KARATSUBA_THRESHOLD 2

/* Internal simple multiplication for recursive Karatsuba's multiplication method.
   Both of the operands are assumed positive integers. */
static sap_num _sap_rec_mul(sap_num op1, sap_num op2)
{
    // todo: implement this recursive multiplication
    if (op1->n_len <= _KARATSUBA_THRESHOLD || op2->n_len <= _KARATSUBA_THRESHOLD)
        return _sap_simple_mul(op1, op2);

    /* Karatsuba's method: x = x1*B^m + x0, y = y1*B^m + y0, xy = z2*B^(2m) + z1 * B^m + z0. */
    sap_num result, x1, x0, y1, y0, z2, z1, z0;
    int shift;

    shift = MIN(op1->n_len / 2, op2->n_len / 2);
    _sap_karatsuba_decomp(op1, &x1, &x0, shift);
    _sap_karatsuba_decomp(op2, &y1, &y0, shift);

    z2 = _sap_rec_mul(x1, y1);
    z0 = _sap_rec_mul(x0, y0);

    /* tmp1 = x1 + x0, tmp2 = y1 + y0, tmp3 = z2 + z0, tmp4 = tmp1 * tmp2 */
    /* z1 = tmp4 - tmp3 */
    sap_num tmp1, tmp2, tmp3, tmp4;
    tmp1 = sap_add(x1, x0, 0);
    tmp2 = sap_add(y1, y0, 0);
    tmp3 = sap_add(z2, z0, 0);
    tmp4 = _sap_rec_mul(tmp1, tmp2);
    z1 = sap_sub(tmp4, tmp3, 0);

    /* Free the intermediate variables first. */
    sap_free_num(&x1);
    sap_free_num(&x0);
    sap_free_num(&y1);
    sap_free_num(&y0);
    sap_free_num(&tmp1);
    sap_free_num(&tmp2);
    sap_free_num(&tmp3);
    sap_free_num(&tmp4);

    sap_num tmp5, tmp6, tmp7;
    tmp5 = _sap_shift(z2, shift * 2);
    tmp6 = _sap_shift(z1, shift);
    tmp7 = sap_add(tmp5, tmp6, 0);
    result = sap_add(tmp7, z0, 0);

    /* Then free the intermediate variables generated afterwards. */
    sap_free_num(&tmp5);
    sap_free_num(&tmp6);
    sap_free_num(&tmp7);
    sap_free_num(&z2);
    sap_free_num(&z1);
    sap_free_num(&z0);

    return result;
}

/* Internal implementation for multiplying two numbers. */
static sap_num _sap_mul_impl(sap_num op1, sap_num op2, int scale)
{
    sap_num tmp1, tmp2, result0, result;

    /* If the numbers have fractional parts, first convert them to integer, then perform the multiplication. */
    tmp1 = _sap_shift(op1, op1->n_scale);
    tmp2 = _sap_shift(op2, op2->n_scale);
    result0 = _sap_rec_mul(tmp1, tmp2);
    result = _sap_shift(result0, -(op1->n_scale + op2->n_scale));
    _sap_truncate(result, scale, FALSE);                                            /* Truncate the number (only the fractional part) to meet scale requirements. */
    result->n_sign = (op1->n_sign == POS) ? op2->n_sign : _sap_negate(op1->n_sign); /* Negate the sign when op1 is negative. */

    sap_free_num(&tmp1);
    sap_free_num(&tmp2);
    sap_free_num(&result0);
    return result;
}

/* Multiply two numbers. The fractional part will be truncated to the size. (Must be larger than 0).
   Return a new number as the result. */
sap_num sap_mul(sap_num op1, sap_num op2, int scale)
{
    return _sap_mul_impl(op1, op2, scale);
}

/* Some useful routines for divisions. */

/* Increase the number by 1.0 * 10^offset, ignoring the sign. Also, assume the storage is enough, and offset is valid. */
static void _sap_self_increase(sap_num op1, int int_offset)
{
    if (op1->n_ptr == NULL) /* Cannot increase a reference number */
        return;

    char carry = 0;
    *(op1->n_ptr + op1->n_len - 1 - int_offset) += 1;
    for (int i = op1->n_len - 1 - int_offset; i >= 0; --i)
    {
        *(op1->n_ptr + i) += carry;
        if (*(op1->n_ptr + i) >= 10)
        {
            *(op1->n_ptr + i) -= 10;
            carry = 1;
        }
        else
        {
            carry = 0;
        }
    }
    if (carry == 1)
        sap_warn("Self increase error: storage not enough: ", 1, sap_num2str(op1));
}

/* Internal simple division for handling small numbers. High complexity. Signs are ignored. */
static sap_num _sap_simple_high_prec_div(sap_num dividend, sap_num divisor, int scale)
{
    if (sap_is_zero(divisor))
    {
        sap_warn("0 divisor detected: ", 3, sap_num2str(dividend), " / ", sap_num2str(divisor));
        return sap_copy_num(_zero_);
    }

    int len;        /* Length of the integral part */
    sap_num result; /* For storing the result */

    len = MAX(dividend->n_len, divisor->n_len) + MAX(dividend->n_scale, divisor->n_scale);
    result = sap_new_num(len, scale);

    /* Simulate division. */
    sap_num tmp = sap_replicate_num(dividend);
    sap_num tmp2;

    while (_sap_compare_impl(tmp, divisor, FALSE) >= 0)
    {
        tmp2 = _sap_sub_impl(tmp, divisor, 0, POS);
        sap_free_num(&tmp);
        tmp = tmp2;
        _sap_self_increase(result, 0);
    }
    if (scale > 0) /* If fractional result are required. */
    {
        for (int i = 1; i <= scale; ++i)
        {
            tmp2 = _sap_shift(tmp, 1);
            sap_free_num(&tmp);
            tmp = tmp2;

            /* Try subtracting the shifted part. */
            while (_sap_compare_impl(tmp, divisor, FALSE) >= 0)
            {
                tmp2 = _sap_sub_impl(tmp, divisor, 0, POS);
                sap_free_num(&tmp);
                tmp = tmp2;
                _sap_self_increase(result, -i);
                // printf("Comparing result: op1(%s), op2=%s, res=%d\n", sap_num2str(tmp), sap_num2str(divisor), _sap_compare_impl(tmp, divisor, FALSE));
            }
        }
    }

    _sap_normalize(result);
    sap_free_num(&tmp);
    return result;
}

/* Internal implementation for division. */
static sap_num _sap_div_impl(sap_num dividend, sap_num divisor, int scale)
{
    sap_num result = _sap_simple_high_prec_div(dividend, divisor, scale);
    result->n_sign = (dividend->n_sign == POS) ? divisor->n_sign : _sap_negate(divisor->n_sign);
    return result;
}

/* Divide dividend by divisor. The fractional part will be truncated to the size. (Must be larger than 0).
   Return a new number as the result. */
sap_num sap_div(sap_num dividend, sap_num divisor, int scale)
{
    return _sap_div_impl(dividend, divisor, scale);
}

/* Internal simple division and modulus for handling small numbers.
   High complexity. Signs are ignored. Integer quotient only. */
static void _sap_simple_divmod(sap_num dividend, sap_num divisor, sap_num *quotient, sap_num *remainder)
{
    if (sap_is_zero(divisor))
    {
        sap_warn("0 divisor detected: ", 3, sap_num2str(dividend), " / ", sap_num2str(divisor));
        *quotient = sap_copy_num(_zero_);
        *remainder = sap_copy_num(_zero_);
        return;
    }

    int len; /* Length of the integral part */

    len = MAX(dividend->n_len, divisor->n_len);
    if (quotient != NULL)
        *quotient = sap_new_num(len, 0);

    /* Simulate division. */
    sap_num tmp = sap_replicate_num(dividend);
    sap_num tmp2;
    while (_sap_compare_impl(tmp, divisor, FALSE) >= 0)
    {
        tmp2 = _sap_sub_impl(tmp, divisor, 0, POS);
        sap_free_num(&tmp);
        tmp = tmp2;
        if (quotient != NULL)
            _sap_self_increase(*quotient, 0);
    }

    if (remainder != NULL)
        *remainder = tmp;
}

/* Internal implementation for modulus. */
static sap_num _sap_mod_impl(sap_num dividend, sap_num divisor, int scale)
{
    sap_num remainder;
    _sap_simple_divmod(dividend, divisor, NULL, &remainder);
    remainder->n_sign = dividend->n_sign;
    _sap_truncate(remainder, scale, FALSE);
    return remainder;
}

/* Divide dividend by divisor. 
   Return a new number as the result.*/
sap_num sap_mod(sap_num dividend, sap_num divisor, int scale)
{
    return _sap_mod_impl(dividend, divisor, scale);
}

/* Internal implementation for simultaneous division and modulus. It is assumed that both the quotient and the remainder are not NULL. */
static void _sap_divmod_impl(sap_num dividend, sap_num divisor, sap_num *quotient, sap_num *remainder, int scale)
{
    _sap_simple_divmod(dividend, divisor, quotient, remainder);
    (*quotient)->n_sign = (dividend->n_sign == POS) ? divisor->n_sign : _sap_negate(divisor->n_sign);
    (*remainder)->n_sign = dividend->n_sign;
    _sap_truncate(*remainder, scale, FALSE);
}

/* Divide dividend by divisor, get both quotient and remainder. 
   Return a new number as the result.*/
void sap_divmod(sap_num dividend, sap_num divisor, sap_num *quotient, sap_num *remainder, int scale)
{
    _sap_divmod_impl(dividend, divisor, quotient, remainder, scale);
}

/* Internal implementation for evaluating sqrt(op) to up to *scale* number of digits after the decimal point. */
static sap_num _sap_sqrt_impl(sap_num op, int scale)
{
    /* Skipping some simple situations. */
    if (sap_is_neg(op))
    {
        sap_warn("Function SQRT performed on negative operand: ", 1, sap_num2str(op));
        return sap_copy_num(_zero_);
    }
    if (sap_is_zero(op))
        return sap_copy_num(_zero_);
    else if (sap_compare(op, _one_) == 0)
        return sap_copy_num(_one_);

    /* Since the first derivative of x^2 is positive and increasing in the first quadrant,
       we can evaluate this with newton's iteration. */
    /* From calculation we know that this iteration must work. */

    /* Major reference for calculating scales: See `number.c` in the source code of GNU bc. */
    int rscale = MAX(op->n_scale, scale);                        /* The final real scale used */
    int cscale = (sap_compare(op, _one_) > 0) ? 3 : op->n_scale; /* Current scale for arithmetic operations */

    int done = FALSE;

    sap_num cguess = NULL;   /* Current guess */
    sap_num nguess = NULL;   /* Next guess */
    sap_num diff = NULL;     /* For evaluating difference to control the precision */
    sap_num one_half = NULL; /* For division by 2 */
    sap_num tmp1 = NULL;
    sap_num tmp2 = NULL;

    one_half = sap_new_num(1, 1);
    one_half->n_val[1] = 5; /* Assign it +0.5 */

    /* Place the initial guess */
    cguess = sap_copy_num(_one_);

    while (!done)
    {
        // todo: fix
        tmp1 = sap_div(op, cguess, cscale);
        tmp2 = sap_add(tmp1, cguess, cscale);
        nguess = sap_mul(tmp2, one_half, cscale);
        diff = sap_sub(cguess, nguess, cscale + 1);
        if (sap_is_near_zero(diff, cscale))
        {
            if (cscale < rscale + 1)                  /* If the precision is not enough */
                cscale = MIN(cscale * 3, rscale + 1); /* Adjusting the precision. */
            else
                done = TRUE;
        }
        sap_free_num(&tmp1);
        sap_free_num(&tmp2);
        sap_free_num(&cguess);
        sap_free_num(&diff);
        cguess = nguess;
    }
    _sap_truncate(cguess, scale, TRUE);
    return cguess;
}

/* Calculate square root of op. op must be positive, or the output will be 0.
   Return a new number as the result. */
sap_num sap_sqrt(sap_num op, int scale)
{
    return _sap_sqrt_impl(op, scale);
}

/* Some useful routines for calculating transcendental functions. */

/* Internal implementation for calculating sin(op). */
static sap_num _sap_sin_impl(sap_num op, int scale)
{
    double val = sap_num2double(op);
    sap_num result = sap_double2num(sin(val));
    _sap_truncate(result, scale, FALSE);
    return result;
}

/* Calculate sin(op) in radians. Currently the precision is limited.
   Return a new number as the result. */
sap_num sap_sin(sap_num op, int scale)
{
    return _sap_sin_impl(op, scale);
}

/* Internal implementation for calculating cos(op). */
static sap_num _sap_cos_impl(sap_num op, int scale)
{
    double val = sap_num2double(op);
    sap_num result = sap_double2num(cos(val));
    _sap_truncate(result, scale, FALSE);
    return result;
}

/* Calculate cos(op) in radians. Currently the precision is limited.
   Return a new number as the result. */
sap_num sap_cos(sap_num op, int scale)
{
    return _sap_cos_impl(op, scale);
}

/* Internal implementation for calculating arctan(op). */
static sap_num _sap_arctan_impl(sap_num op, int scale)
{
    double val = sap_num2double(op);
    sap_num result = sap_double2num(atan(val));
    _sap_truncate(result, scale, FALSE);
    return result;
}

/* Calculate arctan(op) in radians. Currently the precision is limited.
   Return a new number as the result. */
sap_num sap_arctan(sap_num op, int scale)
{
    return _sap_arctan_impl(op, scale);
}

/* Internal implementation for calculating ln(op). */
static sap_num _sap_ln_impl(sap_num op, int scale)
{
    double val = sap_num2double(op);
    sap_num result = sap_double2num(log(val));
    _sap_truncate(result, scale, FALSE);
    return result;
}

/* Calculate ln(op). Currently the precision is limited.
   Return a new number as the result. */
sap_num sap_ln(sap_num op, int scale)
{
    return _sap_ln_impl(op, scale);
}

/* Internal implementation for calculating raise(op, expo). */
static sap_num _sap_raise_impl(sap_num base, sap_num expo, int scale)
{
    /* Process simple situations first. */
    if (expo->n_scale > 0)
    {
        sap_warn("Non integer exponent: ", 3, sap_num2str(base), " ^ ", sap_num2str(expo));
        return sap_copy_num(_zero_);
    }
    if (sap_compare(expo, _zero_) == 0)
    {
        sap_num tmp = sap_new_num(1, scale);
        *tmp->n_ptr = 1;
        return tmp;
    }

    /* Start multiplication */
    sap_num result; /* Result of the process. */
    sap_num expo0;  /* Replicated exponent for subtraction. */
    sap_num tmp1;   /* Temporary variable */

    /* Initialize variables. */
    int rscale = MAX(base->n_scale, scale);
    int cscale = rscale * sap_num2int(expo); /* It is easy to accumulate errors when a wrong scale is selected.
                                                Try to maximize the scale here. */
    int do_reverse = FALSE;
    result = sap_replicate_num(base);
    expo0 = sap_replicate_num(expo);

    if (sap_compare(expo, _zero_) < 0)
    {
        do_reverse = TRUE;
        expo0->n_sign = -expo0->n_sign;
    }

    /* Minus one first, for replicating the number to its place. */
    tmp1 = sap_sub(expo0, _one_, 0);
    sap_free_num(&expo0);
    expo0 = tmp1;

    while (sap_compare(expo0, _zero_) > 0)
    {
        /* Reduce the count. */
        tmp1 = sap_sub(expo0, _one_, 0);
        sap_free_num(&expo0);
        expo0 = tmp1;

        /* Multiply. */
        tmp1 = sap_mul(result, base, cscale);
        sap_free_num(&result);
        result = tmp1;
    }

    /* If the exponent is negative before, take the reciprocal */
    if (do_reverse)
    {
        tmp1 = sap_div(_one_, result, cscale);
        sap_free_num(&result);
        result = tmp1;
    }

    _sap_truncate(result, rscale, FALSE);
    return result;
}

/* Calculate base^expo. Expo must be a integer.
   Return a new number as the result. */
sap_num sap_raise(sap_num base, sap_num expo, int scale)
{
    return _sap_raise_impl(base, expo, scale);
}

/* Calculate exp(op). Op must be a valid integer. Currently the precision is limited.
   Return a new number as the result. */
sap_num sap_exp(sap_num expo, int scale)
{
    return sap_raise(_e_, expo, scale);
}