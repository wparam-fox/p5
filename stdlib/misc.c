/* Copyright 2008 wParam, licensed under the GPL */

#include <windows.h>
#include "sl.h"


void MiscCreateDosDevice (parerror_t *pe, char *devname, char *target)
{
	int st;

	fail_if (!devname, 0, PE_BAD_PARAM (1));
	fail_if (!target, 0, PE_BAD_PARAM (2));

	st = DefineDosDevice (DDD_RAW_TARGET_PATH, devname, target);
	fail_if (!st, 0, PE ("DefineDosDevice failed, %i", GetLastError (), 0, 0, 0));

	//return;
failure:
	return;
}

void MiscDeleteDosDev (parerror_t *pe, char *devname)
{
	int st;

	fail_if (!devname, 0, PE_BAD_PARAM (1));

	st = DefineDosDevice (DDD_REMOVE_DEFINITION, devname, NULL);
	fail_if (!st, 0, PE ("DefineDosDevice failed, %i", GetLastError (), 0, 0, 0));

	//return;
failure:
	return;
}