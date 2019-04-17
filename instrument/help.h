/* Copyright 2006 wParam, licensed under the GPL */

#ifndef _HELP_H_
#define _HELP_H_

typedef struct
{
	char *name;
	char *general;
	char *usage;
	char *details;
} help_t;

typedef help_t * lpHelp;

#endif