/* Copyright 2007 wParam, licensed under the GPL */

#include <windows.h>
#include "sl.h"


HWND HwndWindowFromPointDisabled (parerror_t *pe, POINT p)
{
	HWND h = NULL, ch;
	POINT cpoint;
	DERR st;

	ch = WindowFromPoint (p);
	fail_if (!ch, 0, PE ("No window found at point (%i,%i), %i", p.x, p.y, GetLastError (), 0));

	while (h != ch)
	{
		h = ch;

		cpoint = p;
		st = ScreenToClient (h, &cpoint);
		fail_if (!st, 0, PE ("ScreenToClient failed, %i", GetLastError (), 0, 0, 0));

		ch = ChildWindowFromPointEx (h, cpoint, CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);

		//if something went wrong, return the next best thing.
		if (!ch)
            return h;
	}

	return ch;
failure:
	return NULL;
}

HWND HwndGetFocusWindow (parerror_t *pe)
{
	DWORD tid;
	DWORD othertid;
	HWND hwnd = GetForegroundWindow ();
	DERR st;
	int attached = 0;

	fail_if (!hwnd, 0, PE ("No foreground window found (%i)", GetLastError (), 0, 0, 0));

	tid = GetCurrentThreadId ();
	othertid = GetWindowThreadProcessId (hwnd, NULL);
	fail_if (!othertid, 0, PE ("Unable to get other thread's id, %i", GetLastError (), 0, 0, 0));

	if (tid != othertid)
	{
		st = AttachThreadInput (tid, othertid, TRUE);
		fail_if (!st, 0, PE ("Could not attach to target thread %i, %i", othertid, GetLastError (), 0, 0));
		attached = 1;
	}

	hwnd = GetFocus ();
	//if it's NULL we'll just return that.

	if (attached)
	{
		AttachThreadInput (tid, othertid, FALSE);
		//if this fails, we're screwed.
	}

	return hwnd;
failure:
	if (attached)
		AttachThreadInput (tid, othertid, FALSE);

	return NULL;
}


HWND HwndGetByClass (parerror_t *pe, char *classname)
{
	DERR st;
	HWND out;

	fail_if (!classname, 0, PE_BAD_PARAM (1));

	out = FindWindow (classname, NULL);
	//fail_if (!out, 0, PE ("Window not found, %i", GetLastError (), 0, 0, 0));

	return out;
failure:
	return NULL;
}


BOOL CALLBACK HwndGetByTitleProc (HWND hwnd, LPARAM lParam)
{
	void **params = (void **)lParam;
	DERR st;
	int mode = (int)params[1];
	char *buffer = NULL;
	int blen, dummy;
	parerror_t *pe = params[3];

	//GetWindowTextLength likes to return 259, "No more data is available".  Genius.
	st = SendMessageTimeout (hwnd, WM_GETTEXTLENGTH, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 2000, &blen);
	fail_if (!st && GetLastError () == NO_ERROR, 0, PE ("WM_GETTEXTLENGTH message timed out", 0, 0, 0, 0));
	fail_if (!st, 0, PE ("SendMessageTimeout failed, %i", GetLastError (), 0, 0, 0));
	blen++; //blen is definitely enough space for the text and the null

	buffer = PlMalloc (blen + 1); //allocate an extra space for a backup null
	fail_if (!buffer, FALSE, PE_OUT_OF_MEMORY (blen + 1));

	st = SendMessageTimeout (hwnd, WM_GETTEXT, blen, (LPARAM)buffer, SMTO_ABORTIFHUNG | SMTO_BLOCK, 2000, &dummy);
	fail_if (!st && GetLastError () == NO_ERROR, 0, PE ("WM_GETTEXT message timed out", 0, 0, 0, 0));
	fail_if (!st, 0, PE ("SendMessageTimeout failed, %i", GetLastError (), 0, 0, 0));
	buffer[blen] = '\0'; //add the emergency null

	if (mode == 1) //do partial match
	{
		if (PlStrncmp (buffer, params[0], PlStrlen (params[0])) == 0)
		{
			params[2] = hwnd;
		}
	}
	else if (mode == 2)
	{
		if (PlStrstr (buffer, params[0]))
		{
			params[2] = hwnd;
		}
	}
	else
	{
		if (PlStrcmp (buffer, params[0]) == 0)
		{
			params[2] = hwnd;
		}
	}

	PlFree (buffer);

	return params[2] ? FALSE : TRUE;

failure:
	if (buffer)
		PlFree (buffer);

	return FALSE;

}



HWND HwndGetByTitle (parerror_t *pe, char *title, int partial)
{
	void *params[4];
	DERR st;

	fail_if (!title, 0, PE_BAD_PARAM (1));


	if (partial == 3)
	{
		params[2] = FindWindow (NULL, title);
	}
	else
	{

		params[0] = title;
		params[1] = (void *)partial;
		params[2] = NULL; //return value
		params[3] = pe;

		st = EnumWindows (HwndGetByTitleProc, (LPARAM)params);
		fail_if (pe->error, 0, 0); //pass any given error on up
		fail_if (!st && GetLastError () != NO_ERROR && GetLastError () != ERROR_NO_MORE_ITEMS, 0, PE ("EnumWindows failed, %i", GetLastError (), 0, 0, 0));
		//fail_if (!params[2], 0, PE ("Window not found", 0, 0, 0, 0));
	}

	return params[2];

failure:
	return NULL;
}


HWND HwndGetByMouse (parerror_t *pe)
{
	POINT p;
	DERR st;
	HWND out;

	st = GetCursorPos (&p);
	fail_if (!st, 0, PE ("GetCursorPos failed, %i", GetLastError (), 0, 0, 0));

	out = WindowFromPoint (p);
	//fail_if (!out, 0, PE ("Window not found, %i", GetLastError (), 0, 0, 0));

	return out;
failure:
	return NULL;
}

HWND HwndGetByMouseDisabled (parerror_t *pe)
{
	POINT p;
	DERR st;
	HWND out;

	st = GetCursorPos (&p);
	fail_if (!st, 0, PE ("GetCursorPos failed, %i", GetLastError (), 0, 0, 0));

	out = HwndWindowFromPointDisabled (pe, p);
	fail_if (pe->error, 0, 0); //if pe was set, leave it alone
	//fail_if (!out, 0, PE ("Window not found, %i", GetLastError (), 0, 0, 0));
	//I think if the function returns NULL that pe will always be set, but I'm
	//not sure.

	return out;
failure:
	return NULL;
}


HWND HwndGetByActive (parerror_t *pe)
{
	return GetForegroundWindow ();
}

HWND HwndGetByFocus (parerror_t *pe)
{
	DERR st;
	HWND out;

	out = HwndGetFocusWindow (pe);
	fail_if (pe->error, 0, 0); //don't upset PE if it was set
	//fail_if (!out, 0, PE ("Window not found", 0, 0, 0, 0));

	return out;
failure:
	return NULL;
}

HWND HwndGetByDlgId (parerror_t *pe, HWND hwnd, int id)
{
	HWND out;
	DERR st;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	out = GetDlgItem (hwnd, id);

	return out;
failure:
	return NULL;
}

HWND HwndGetParent (parerror_t *pe, HWND hwnd)
{
	HWND out;
	DERR st;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	out = GetParent (hwnd);

	return out;
failure:
	return NULL;
}