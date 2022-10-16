/* The single file for all library inclusions and constants, definitions for sap.c */
#ifndef _SAPDEFS_H
#define _SAPDEFS_H

#include "global.h"
#include "number.h"
#include "lut.h"
#include "parser.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

/* Definitions */
#define VERSION "0.01 ALPHA"

#ifdef TRUE
#undef TRUE
#endif
#define TRUE 1

#ifdef FALSE
#undef FALSE
#endif
#define FALSE 0

#endif