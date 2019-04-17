/* Copyright 2013 wParam, licensed under the GPL */

#include <windows.h>
#include "sl.h"


#define DESKTOP_ALL_ACCESS (DESKTOP_CREATEMENU | \
		DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE | \
		DESKTOP_HOOKCONTROL | DESKTOP_JOURNALPLAYBACK | \
		DESKTOP_JOURNALRECORD | DESKTOP_READOBJECTS | \
		DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS | DESKTOP_SWITCHDESKTOP)

void DeskCreate (parerror_t *pe, char *deskname, char *commandline)
{
	HDESK desk = NULL;
	PROCESS_INFORMATION pi;
	STARTUPINFO si = {sizeof (si)};
	SECURITY_ATTRIBUTES sa;
	int st;

	fail_if (!deskname, 0, PE_BAD_PARAM (1));
	fail_if (!commandline, 0, PE_BAD_PARAM (2));

	//oddly enough, this handle must be inherited or the new process freaks out.
	sa.nLength = sizeof (sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	desk = CreateDesktop (deskname, NULL, NULL, 0, DESKTOP_ALL_ACCESS, &sa);
	fail_if (!desk, 0, PE_STR ("CreateDesktop failed to create a desktop named %s, %i", COPYSTRING (deskname), GetLastError (), 0, 0));

	si.lpDesktop = deskname;
	st = CreateProcess (NULL, commandline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	fail_if (!st, 0, PE ("CreateProcess failed, %i", GetLastError (), 0, 0, 0));

	CloseHandle (pi.hProcess);
	CloseHandle (pi.hThread);

	CloseDesktop (desk);

	return;
failure:

	if (desk)
		CloseDesktop (desk);

	return;
}

void DeskCreateConsole (parerror_t *pe, char *deskname, char *commandline, int fillattrib, int fullscreen, char *workdir, char *title)
{
	HDESK desk = NULL;
	PROCESS_INFORMATION pi;
	STARTUPINFO si = {sizeof (si)};
	SECURITY_ATTRIBUTES sa;
	int st;

	fail_if (!deskname, 0, PE_BAD_PARAM (1));
	fail_if (!commandline, 0, PE_BAD_PARAM (2));
	//it's ok for working dir and title to be NULL

	//oddly enough, this handle must be inherited or the new process freaks out.
	sa.nLength = sizeof (sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	desk = CreateDesktop (deskname, NULL, NULL, 0, DESKTOP_ALL_ACCESS, &sa);
	fail_if (!desk, 0, PE_STR ("CreateDesktop failed to create a desktop named %s, %i", COPYSTRING (deskname), GetLastError (), 0, 0));

	si.lpDesktop = deskname;
	si.dwFillAttribute = fillattrib;
	si.dwFlags = STARTF_USEFILLATTRIBUTE;
	if (fullscreen)
		si.dwFlags |= STARTF_RUNFULLSCREEN;
	si.lpTitle = title;

	st = CreateProcess (NULL, commandline, NULL, NULL, TRUE, 0, NULL, workdir, &si, &pi);
	fail_if (!st, 0, PE ("CreateProcess failed, %i", GetLastError (), 0, 0, 0));

	CloseHandle (pi.hProcess);
	CloseHandle (pi.hThread);

	CloseDesktop (desk);

	return;
failure:

	if (desk)
		CloseDesktop (desk);

	return;
}

void DeskSwitch (parerror_t *pe, char *deskname)
{
	HDESK desk = NULL;
	DERR st;

	fail_if (!deskname, 0, PE_BAD_PARAM (1));

	desk = OpenDesktop (deskname, 0, FALSE, DESKTOP_SWITCHDESKTOP);
	fail_if (!desk, 0, PE_STR ("Could not open desktop %s, %i", COPYSTRING (deskname), GetLastError (), 0, 0));

	st = SwitchDesktop (desk);
	fail_if (!st, 0, PE ("SwitchDesktop returned %i", GetLastError (), 0, 0, 0));

	CloseDesktop (desk);

	return;

failure:
	if (desk)
		CloseDesktop (desk);

}

char *DeskCurrent (parerror_t *pe)
{
	HDESK desk = NULL;
	char *deskname = NULL; //this will be returned
	int desklen = 0;
	DERR st;

	desk = OpenInputDesktop (0, FALSE, 0);
	fail_if (!desk, 0, PE ("Could not open input desktop, %i", GetLastError (), 0, 0, 0));

	//desklen is already zero, so if it isn't set, assume an error happened.
	st = GetUserObjectInformation (desk, UOI_NAME, &st, sizeof (st), &desklen);
	fail_if (desklen == 0, 0, PE ("Could not get length of name of input desktop, %i", GetLastError (), 0, 0, 0));

	deskname = ParMalloc (desklen);
	fail_if (!deskname, 0, PE_OUT_OF_MEMORY (desklen));

	st = GetUserObjectInformation (desk, UOI_NAME, deskname, desklen, &desklen);
	fail_if (!st, 0, PE ("Could not get name of input desktop, %i", GetLastError (), 0, 0, 0));

	//cool.
	CloseDesktop (desk);

	return deskname;

failure:

	if (desk)
		CloseHandle (desk);

	if (deskname)
		ParFree (deskname);

	return NULL;

}

static BOOL CALLBACK E_DesktopsProc (char *desktop, LPARAM lParam)
{
	PlPrintf ("%s\n", desktop);
	return TRUE;
}

void DeskEnumDesktops (parerror_t *pe, char *winstation)
{
	HWINSTA winsta = NULL;
	DERR st;
	BOOL close = FALSE;

	if (winstation)
	{
		winsta = OpenWindowStation (winstation, FALSE, WINSTA_ENUMDESKTOPS);
		fail_if (!winsta, 0, PE ("OpenWindowStation failed: %i", GetLastError (), 0, 0, 0));
		close = TRUE;
	}
	else
	{
		winsta = GetProcessWindowStation ();
		//Leave close = false; closing this handle is bad news
	}

	st = EnumDesktops (winsta, E_DesktopsProc, 0);
	fail_if (!st, 0, PE ("EnumDesktops failed: %i", GetLastError (), 0, 0, 0));
	
	return;
failure:
	if (winsta && close)
		CloseHandle (winsta);
}
