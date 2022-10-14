/* Header file for arbitrary precision number */

#ifndef _NUMBER_H
#define _NUMBER_H


/* Definitions */
#ifdef TRUE
#undef TRUE
#endif
#define TRUE 1

#ifdef FALSE
#undef FALSE
#endif
#define FALSE 0


/* Struct declarations */

typedef enum
{
    POS,
    NEG
} sign;

typedef struct sap_struct *sap_num;

typedef struct sap_struct
{
    sign n_sign;  /* For the sign of the number. To specify, zero has positive sign. */
    int n_refs;   /* For counting how many references are pointed to this number. 
                     If 0, the structure will be appended to the available resource list. */ 
    struct sap_struct *n_next; /* For storing the next node when in the sap_free_list */

    int n_len;    /* For number of digits before the decimal point */
    int n_scale;  /* For number of digits after the decimal point */

    /* The structure of the storage is shown as follows:
       |---(MSB)---Digits---(LSB)---|---(MSB)---Fractions---(LSB)---|
     */
    char *n_ptr;  /* For internal storage. This may be NULL to indicate that 
                     the value actually points to the storage in another number. */
    char *n_val;  /* For pointer to actual value. */
} sap_struct;


/* Global constants */

extern sap_num _zero_;
extern sap_num _one_;
extern sap_num _two_;


/* Function prototypes */

void sap_init_lib(void);

sap_num sap_new_num(int length, int scale);

void sap_free_num(sap_num *op);

sap_num sap_copy_num(sap_num src);

void sap_init_num(sap_num *op);

sap_num sap_str2num(char *ptr);

char *sap_num2str(sap_num op);

int sap_is_zero(sap_num op);

int sap_is_near_zero(sap_num op, int scale);

int sap_is_neg(sap_num op);

int sap_compare(sap_num op1, sap_num op2);

sap_num sap_add(sap_num op1, sap_num op2, int scale_min);

sap_num sap_sub(sap_num op1, sap_num op2, int scale_min);

sap_num sap_mul(sap_num op1, sap_num op2, int scale);

sap_num sap_div(sap_num dividend, sap_num divisor, int scale);

sap_num sap_mod(sap_num dividend, sap_num divisor, int scale);

void sap_divmod(sap_num dividend, sap_num divisor, sap_num *quotient, sap_num *remainder, int scale);

sap_num sap_sqrt(sap_num op1, int scale);

sap_num sap_exp(sap_num base, sap_num expo, int scale);

#endif