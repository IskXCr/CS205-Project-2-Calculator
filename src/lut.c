/* Source file for the implementation of the lookup table. */
#include "lut.h"
#include "number.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

/* Common macros */

#ifdef MIN
#undef MIN
#endif
#define MIN(a, b) (((a) <= (b)) ? (a) : (b))

#ifdef MAX
#undef MAX
#endif
#define MAX(a, b) (((a) >= (b)) ? (a) : (b))

/* Functions */

/* Hash function for converting string to an integer.
   Reference: http://www.cse.yorku.ca/~oz/hash.html */
static unsigned int hash(unsigned char *str)
{
    unsigned int hash = 5381; /* ? */
    int c;

    while (c = *str++)                   /* c != '\0' */
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/* Create a new node. No validity check. */
static lut_node _lut_new_node(char *key, sap_num val)
{
    lut_node tmp = (lut_node)malloc(sizeof(lut_node_struct));

    /* Allocate extra memory for storing the key. */
    int len = strlen(key) + 1;
    char *p = (char *)malloc(len);
    if (p == NULL)
        out_of_memory();
    memcpy(p, key, len);

    tmp->key = p;
    tmp->val = sap_copy_num(val); /* Ensure that the value won't be freed and thus point to nowhere. */
    tmp->next = NULL;
    return tmp;
}

/* Free the resource of a node and set the corresponding pointer to NULL.
   The relation in the linked list is not handled. */
static void _lut_free_node(lut_node *node)
{
    if (node == NULL || *node == NULL)
        return;

    free((*node)->key);
    sap_free_num(&((*node)->val));
    free(*node);
    *node = NULL;
}

/* Initialize a new LUT table. */
lut_table lut_new_table(void)
{
    lut_table tmp = (lut_table)malloc(sizeof(lut_table_struct));
    if (tmp == NULL)
        out_of_memory();

    /* Set entries. */
    tmp->entries = (lut_node *)malloc(_LUT_DEFAULT_CAPACITY * sizeof(lut_node));
    if (tmp->entries == NULL)
        out_of_memory();
    memset(tmp->entries, 0, _LUT_DEFAULT_CAPACITY * sizeof(lut_node)); /* Set all entries to NULL. */

    /* Set capacity. */
    tmp->capacity = _LUT_DEFAULT_CAPACITY;
    return tmp;
}

/* Free a hashtable and all its related resources. */
void lut_free_table(lut_table *table)
{
    if (table == NULL || *table == NULL)
        return;

    for (int i = 0; i < (*table)->capacity; ++i)
        if (*((*table)->entries + i) != NULL)
            _lut_free_node((*table)->entries + i);
    free((*table)->entries);
    free(*table);
    *table = NULL;
}

/* Find the value of a key in the hashtable. Table must be valid and nonnull, and key must be nonnull.
   Return NULL if such entry doesn't exist. */
sap_num lut_find(lut_table table, char *key)
{
    unsigned int key0 = hash(key) % (table->capacity);
    lut_node target = *(table->entries + key0);
    sap_num result = NULL;

    while (target != NULL)
        if (strcmp(target->key, key) == 0)
        {
            result = target->val;
            break;
        }
        else
        {
            target = target->next;
        }
    return result;
}

/* Insert a value to hashtable. If the key already correspond to an entry, overwrite it. */
void lut_insert(lut_table table, char *key, sap_num val)
{
    unsigned int key0 = hash(key) % (table->capacity);
    lut_node target = *(table->entries + key0), prev = NULL;

    while (target != NULL)
        if (strcmp(target->key, key) == 0)
        {
            sap_free_num(&(target->val));
            target->val = val;
            break;
        }
        else
        {
            prev = target;
            target = target->next;
        }

    if (target == NULL) /* Cannot find. Append the entry. */
        if (prev != NULL)
            prev->next = _lut_new_node(key, val);
        else
            *(table->entries + key0) = _lut_new_node(key, val);
}

/* Delete a value from hashtable. */
void lut_delete(lut_table table, char *key)
{
    unsigned int key0 = hash(key) % (table->capacity);
    lut_node target = *(table->entries + key0), prev = NULL;

    while (target != NULL)
        if (strcmp(target->key, key) == 0)
        {
            if (prev != NULL)
                prev->next = target->next;
            _lut_free_node(&target);
            break;
        }
        else
        {
            prev = target;
            target = target->next;
        }
}

/* Reset a hashtable. */
void lut_reset_all(lut_table table)
{
    for (int i = 0; i < table->capacity; ++i)
        if (*(table->entries + i) != NULL)
            _lut_free_node(table->entries + i);
    memset(table->entries, 0, table->capacity * sizeof(lut_node)); /* Set all entries to NULL. */
}