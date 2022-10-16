/* Header file for SAP */
#ifndef _SAP_H
#define _SAP_H

/* Included libraries */

#include "number.h"

/* Function prototypes */

void sap_init_lib(void);

sap_num sap_execute(char *stmt);

sap_num sap_reset_all(void);

#endif