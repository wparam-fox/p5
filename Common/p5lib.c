/* Copyright 2006 wParam, licensed under the GPL */

#include <windows.h>

#define fail_if(cond, status, other) if (cond) { st = status ; other ; /*printf ("line: %i\n", __LINE__);*/ goto failure ; }
#define DERRCODE(base, extent) (((base) << 24) | ((extent) << 16))
#define DERRINFO(errcode, info) ((errcode) | ((info) & 0xFFFF))
#define GETDERRCODE(val) ((val) & 0xFFFF0000)
#define GETDERRINFO(val) ((val) & 0xFFFF)
typedef unsigned long DERR;

#define DERR_OK(a) ((a) == PF_SUCCESS)
#define DI(val) st = DERRINFO (st, (val))
#define DIGLE DI (GetLastError ())


#define PF_DERR_BASE		0x00
#define PF_DEFAULT_NAME_ERROR		DERRCODE (PF_DERR_BASE, 0x0A)
#define PF_GET_DESKTOP_NAME_ERROR	DERRCODE (PF_DERR_BASE, 0x07)
#define PF_WHAT_ARE_YOU_THINKING	DERRCODE (PF_DERR_BASE, 0x08)
#define PF_SUCCESS					0
#define PF_BUFFER_TOO_SMALL			DERRCODE (PF_DERR_BASE, 0x09)
#define PF_INVALID_PARAMETER		DERRCODE (PF_DERR_BASE, 0x12)
#define PF_FILE_NOT_FOUND			DERRCODE (PF_DERR_BASE, 0x0D)
#define PF_WRITE_FILE_FAILED		DERRCODE (PF_DERR_BASE, 0x0C)



//The default pipe name is calculated as follows:
//
//	p5.[desktop name].control[.debug]
//
//   .debug is appended for debug builds.
//
DERR PfGetDefaultPipeName (HDESK desktop, char *deskname, char *nameout, int *namelen)
{
	int desknamelen;
	int res;
	DERR st;
	char *p1 = "\\\\.\\MAILSLOT\\p5.";
#ifdef _DEBUG
	char *p2 = ".control.debug";
#else
	char *p2 = ".control";
#endif
	char *out;
	int len;

	fail_if ((desktop && deskname) || (!desktop && !deskname), PF_DEFAULT_NAME_ERROR, 0);

	if (desktop)
	{
		desknamelen = 0;
		res = GetUserObjectInformation (desktop, UOI_NAME, &res, sizeof (res), &desknamelen);
		fail_if (!desknamelen, PF_GET_DESKTOP_NAME_ERROR, DIGLE);
		desknamelen--; //this function counts the null character, we don't want to do that.
	}
	else
	{
		desknamelen = strlen (deskname);
	}

	if (!nameout) //they just want the length
	{
		fail_if (!namelen, PF_WHAT_ARE_YOU_THINKING, 0xA55);
		*namelen = strlen (p1) + strlen (p2) + desknamelen + 1; //1 for the null
		return PF_SUCCESS;
	}

	//copy this thing over
	out = nameout;
	len = *namelen;

	res = strlen (p1);
	fail_if (len < res + 1, PF_BUFFER_TOO_SMALL, 0);
	strcpy (out, p1);
	out += res;
	len -= res;

	if (desktop)
	{
		res = GetUserObjectInformation (desktop, UOI_NAME, out, len, &desknamelen);
		fail_if (!res, PF_GET_DESKTOP_NAME_ERROR, DIGLE);

		desknamelen--;

		out += desknamelen;
		len -= desknamelen;
	}
	else
	{
		res = strlen (deskname);
		fail_if (len < res + 1, PF_BUFFER_TOO_SMALL, 0);
		strcpy (out, deskname);
		out += res;
		len -= res;
	}

	res = strlen (p2);
	fail_if (len < res + 1, PF_BUFFER_TOO_SMALL, 0);
	strcpy (out, p2);
	out += res;
	len -= res;

	//this is mostly a sanity check, there should be no way len can be 0 at this
	//point, it should always be 1 or more
	fail_if (!len, PF_BUFFER_TOO_SMALL, 0);

	*out = '\0'; //also shouldn't be necessary.

	return PF_SUCCESS;
failure:
	return st;
}


DERR PfSendMailCommand (char *name, char *line)
{
	HANDLE slot = NULL;
	int st;
	DWORD read;

	if (!line || !*line)
		return PF_SUCCESS; //if we have nothing to parse, we've succeeded

	if (!name)
		return PF_INVALID_PARAMETER;

	slot = CreateFile (name, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	fail_if (slot == INVALID_HANDLE_VALUE, PF_FILE_NOT_FOUND, slot = NULL; DIGLE);

	st = WriteFile (slot, line, strlen (line), &read, NULL);
	fail_if (!st, PF_WRITE_FILE_FAILED, DIGLE);

	CloseHandle (slot);
	slot = NULL;

	return PF_SUCCESS;

failure:

	if (slot)
		CloseHandle (slot);

	return st;
}