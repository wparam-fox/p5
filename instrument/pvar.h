/* Copyright 2006 wParam, licensed under the GPL */

#ifndef _PVAR_H_
#define _PVAR_H_

#define PVAR_FLOAT 1
#define PVAR_INT 2
#define PVAR_CHAR 4
#define PVAR_HWND 8
#define PVAR_ERROR 16
#define PVAR_STRUCT 32

typedef struct
{
	char name[30];
	
	int _int;
	float _float;
	char *_char;
	
	
	int flags;
} pvar_t;

#endif