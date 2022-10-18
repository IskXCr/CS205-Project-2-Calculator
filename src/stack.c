#include "stack.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

/* Definitions */

/* Functions */

/* Create a new stack and return the pointer to stack_struct.
   Default size can be fetched from _STACK_DEFAULT_SIZE */
stack sap_new_stack(void)
{
    stack tmp;       /* Store the result */
    sap_token *base; /* Pointer to the base element of the array */

    /* Initializations */
    tmp = (stack)malloc(sizeof(stack_struct));
    if (tmp == NULL)
        out_of_memory();
    base = (sap_token *)malloc(_STACK_DEFAULT_SIZE * sizeof(sap_token));
    if (base == NULL)
        out_of_memory();
    
    tmp->max_size = _STACK_DEFAULT_SIZE;
    tmp->ptr = tmp->base = base;

    return tmp;
}

/* Free a stack and release all its resources. The pointer passed will be set to NULL. */
void sap_free_stack(stack *s)
{
    free((*s)->base);
    *s = NULL;
}

/* Return TRUE if the stack has content. FALSE otherwise. */
int sap_stack_has_element(stack s)
{
    if (s->ptr > s->base)
        return TRUE;
    else
        return FALSE;
}

/* Return TRUE if the stack is empty. FALSE otherwise. */
int sap_stack_empty(stack s)
{
    return !sap_stack_has_element(s);
}

/* Push the reference of an element to the stack. The stack will dynamically increase its size. 
   No copy is saved. Only the sap_token (pointer) is placed into the container. */
void sap_stack_push(stack s, sap_token element)
{
    if (s->ptr - s->base == s->max_size)
    {
        int size = s->max_size;
        s->base = (sap_token *)realloc(s->base, size + _STACK_DEFAULT_SIZE);
        if (s->base == NULL)
            out_of_memory();

        s->max_size = size + _STACK_DEFAULT_SIZE;
        s->ptr = s->base + size;
    }
    *s->ptr = element;
    s->ptr++;
}

/* Pop an element from the stack and return that. If no element remains, return NULL. */
sap_token sap_stack_pop(stack s)
{
    if (s->ptr > s->base)
        return *(--(s->ptr));
    else
        return NULL;
}

/* Peek the top element from the stack and return that. If no element remains, return NULL. */
sap_token sap_stack_top(stack s)
{
    if (s->ptr > s->base)
        return *(s->ptr - 1);
    else
        return NULL;
}

/* Reset the stack. Internally the stack base is reset. */
void sap_stack_reset(stack s)
{
    free(s->base);
    s->base = (sap_token *)malloc(_STACK_DEFAULT_SIZE * sizeof(sap_token));
    if (s->base == NULL)
        out_of_memory();
    s->max_size = _STACK_DEFAULT_SIZE;
    s->ptr = s->base;
}