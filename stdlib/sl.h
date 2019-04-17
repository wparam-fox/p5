/* Copyright 2008 wParam, licensed under the GPL */

#ifndef _SL_H_
#define _SL_H_


//define IS_PLUGIN so that the COPYSTRING macro will use PlStrlen and such.
#define IS_PLUGIN
#include "..\common\libplug.h"



int UtilReverseColor (int l);


typedef struct
{
	unsigned short int cksum;
	unsigned short int size; //size, in bytes, of the data contained in ->data
	BYTE data[0];
} ckblock_t;

//returns from ParMalloc:
char *CkConvertToOutput (parerror_t *pe, void *in, int inlen, char *type);
//returns from PlMalloc:
ckblock_t *CkConvertFromOutput (parerror_t *pe, char *in, char *typeexpected);
unsigned short CkComputeChecksum (ckblock_t *in);

void IcWindowFinalCleanup (void);

#endif