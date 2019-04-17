/* Copyright 2008 wParam, licensed under the GPL */

#include <windows.h>
#include "govern.h"
#include <stdio.h>

int GovIsPluginValue = 0;
char GbName[MAX_PATH] = "p5-Governor";




void GovSetPlugin (void)
{
	GovIsPluginValue = 1;
}

int GovIsPlugin (void)
{
	return GovIsPluginValue;
}

extern GovDebugLevel;
void GovDprintf (int level, char *format, ...)
{
	char buffer[1024];

	if (level <= GovDebugLevel)
	{
		va_list marker;
		va_start (marker, format);
		
		_vsnprintf (buffer, 1024, format, marker);
		buffer[1023] = '\0';

		OutputDebugString (buffer);

		va_end (marker);
	}
}

extern int GovDesktopsDifferent;
void GovSetDesktopsDifferent (int newval)
{
	GovDesktopsDifferent = newval;
}

int GovAreDesktopsDifferent (void)
{
	return GovDesktopsDifferent;
}

int GovGetDebugLevel (void)
{
	return GovDebugLevel;
}

void GovSetDebugLevel (int level)
{
	GovDebugLevel = level;
}


int GovGetName (char *outbuf, int outlen, char *type)
{
	char desknamebuf[MAX_PATH];
	DERR st;
	DWORD desknamelen;
	char *extra = "";

	/* MSDN says I don't need to close the handle from GetThreadDesktop... */
	st = GetUserObjectInformation (GetThreadDesktop (GetCurrentThreadId ()), UOI_NAME, desknamebuf, MAX_PATH - 1, &desknamelen);
	fail_if (!st, 0, 0); /* As long as this is the only fail, don't print it */
	desknamebuf[MAX_PATH - 1] = '\0';

	if (strcmp (type, "mailslot") == 0)
		extra = "\\\\.\\Mailslot\\";

	if (GovDesktopsDifferent)
		_snprintf (outbuf, outlen - 1, "%s%s:%s:%s", extra, GbName, desknamebuf, type);
	else
		_snprintf (outbuf, outlen - 1, "%s%s:%s", extra, GbName, type);
	outbuf[outlen - 1] = '\0';

	return 1;
failure:
	return 0;
}

char *GovScriptGetName (parerror_t *pe)
{
	char *out = NULL;
	DERR st;

	out = ParMalloc (strlen (GbName) + 1);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (strlen (GbName) + 1));

	strcpy (out, GbName);

	return out;
failure:
	/*if (out)
		ParFree (out);*/
	return NULL;
}

void GovScriptSetName (parerror_t *pe, char *newname)
{
	DERR st;

	fail_if (!newname, 0, PE_BAD_PARAM (1));
	fail_if (strlen (newname) + 1 > sizeof (GbName), 0, PE ("Name too large (%i > %i)\n", strlen (newname) + 1, sizeof (GbName), 0, 0));

	strcpy (GbName, newname);

failure:
	return;
}

extern int GovPrintGovernsVal;
int GovPrintGoverns (void)
{
	return GovPrintGovernsVal;
}

void GovSetPrintGoverns (int newval)
{
	GovPrintGovernsVal = 1;
}


/* NOTHING BUT SHARED VARIABLES BELOW THIS POINT! */
#ifdef SEVEN
#pragma section("govfoo", read, write, shared)
#else
#pragma data_seg("govfoo")
#endif

__declspec (allocate ("govfoo")) int GovDebugLevel = NONE;
__declspec (allocate ("govfoo")) int GovDesktopsDifferent = 1;
__declspec (allocate ("govfoo")) int GovPrintGovernsVal = 0;

