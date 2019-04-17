/* Copyright 2013 wParam, licensed under the GPL */

#include <windows.h>
#include "sl.h"


//(sl.win.activate [hwnd])
void WinBringToFront (parerror_t *pe, HWND win)
{
	HMODULE hmod;
	DERR st;
	VOID (WINAPI * SwitchToThisWindow) (HWND, BOOL);

	fail_if (!win, 0, PE_BAD_PARAM (1));

	hmod = GetModuleHandle ("user32.dll");
	fail_if (!hmod, 0, PE ("GetModuleHandle returned NULL for user32.dll, %i", GetLastError (), 0, 0, 0));

	SwitchToThisWindow = (void *)GetProcAddress (hmod, "SwitchToThisWindow");
	fail_if (!SwitchToThisWindow, 0, PE ("GetProcAddress couldn't find SwitchToThisWindow, %i", GetLastError (), 0, 0, 0));

	SwitchToThisWindow (win, TRUE);

failure:
	return;
}



BOOL CALLBACK WinRepaintGoTime (HWND hwnd, LPARAM lParam)
{
	parerror_t *pe = (parerror_t *)lParam;
	DERR st;

	//I don't think this can happen, but just in case
	fail_if (pe->error, 0, 0);

	st = InvalidateRect (hwnd, NULL, TRUE);
	fail_if (!st, 0, PE ("InvalidateRect failed, %i", GetLastError (), 0, 0, 0));

	st = EnumChildWindows (hwnd, WinRepaintGoTime, lParam);
	fail_if (pe->error, 0, 0); //pass the error back
	fail_if (!st && GetLastError () != NO_ERROR && GetLastError () != ERROR_NO_MORE_ITEMS, 0, PE ("EnumChildWindows failed, %i", GetLastError (), 0, 0, 0));

	return TRUE;
failure:
	return FALSE;
}





//(sl.win.repaint [hwnd])
void WinRepaint (parerror_t *pe, HWND win)
{
	DERR st;

	fail_if (!win, 0, PE_BAD_PARAM (1));

	st = InvalidateRect (win, NULL, TRUE);
	fail_if (!st, 0, PE ("InvalidateRect failed, %i", GetLastError (), 0, 0, 0));

	st = EnumChildWindows (win, WinRepaintGoTime, (LPARAM)pe);
	fail_if (pe->error, 0, 0); //pass back
	fail_if (!st && GetLastError () != NO_ERROR && GetLastError () != ERROR_NO_MORE_ITEMS, 0, PE ("EnumChildWindows failed, %i", GetLastError (), 0, 0, 0));

	//fall through
failure:
	return;
}


/*
Can be done with an alias.
void CMD_WIN_MakeLower (int argc, char **argv)
{
	POINT p;
	HWND child;
	char *alloc;
	int crap;
	GetCursorPos (&p);
	child = WindowFromPoint (p);
	if (!child)
		return COMMAND_WINDOWNOTFOUND;
	crap=SendMessage (child, WM_GETTEXTLENGTH, 0, 0);
	alloc = (char *)malloc(sizeof(char) * (crap + 3));

	SendMessage (child, WM_GETTEXT, crap+3, (LPARAM)alloc);
	CharLower (alloc);
	SendMessage (child, WM_SETTEXT, 0, (LPARAM)alloc);
	free(alloc);

	return COMMAND_NOERROR;
}
*/

//(sl.win.postmessage [hwnd] [i:message] [i:wParam] [i:lParam])
void WinPostMessage (parerror_t *pe, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DERR st;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	st = PostMessage (hwnd, message, wParam, lParam);
	fail_if (!st, 0, PE ("PostMessage failed, %i", GetLastError (), 0, 0, 0));

failure:
	return;
}

//(sl.win,sendmessage [hwnd] [i:message] [i:wParam] [i:lParam])
int WinSendMessage (parerror_t *pe, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DERR st;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	st = SendMessage (hwnd, message, wParam, lParam);

	return st;
failure:
	return 0;
}


#define LWA_COLORKEY            0x00000001
#define LWA_ALPHA               0x00000002
#define WS_EX_LAYERED           0x00080000
//(sl.win.alpha [hwnd] [i:blend val])
void WinAlphaBlend (parerror_t *pe, HWND hwnd, int blend)
{
	HMODULE hmod;
	BOOL (WINAPI * SetLayeredWindowAttributes) (HWND, COLORREF, BYTE, DWORD);
	DERR st;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	hmod = GetModuleHandle ("user32.dll");
	fail_if (!hmod, 0, PE ("GetModuleHandle failed for user32.dll, %i", GetLastError (), 0, 0, 0));

	SetLayeredWindowAttributes = (void *)GetProcAddress (hmod, "SetLayeredWindowAttributes");
	fail_if (!SetLayeredWindowAttributes, 0, PE ("GetProcAddress failed finding SetLayeredWindowAttributes (nt < 5.0?), %i", GetLastError (), 0, 0, 0));

	if ((blend & 0xFF) == 0xFF)
	{
		SetLastError (0);
		st = GetWindowLong (hwnd, GWL_EXSTYLE);
		fail_if (!st && GetLastError () != NO_ERROR, 0, PE ("GetWindowLong failed for target window, %i", GetLastError (), 0, 0, 0));

		st &= ~WS_EX_LAYERED;

		SetLastError (0);
		st = SetWindowLong (hwnd, GWL_EXSTYLE, st);
		fail_if (!st && GetLastError () != NO_ERROR, 0, PE ("SetWindowLong failed for target window, %i", GetLastError (), 0, 0, 0));

		return; //that's all we had to do.
	}


	SetLastError (0);
	st = GetWindowLong (hwnd, GWL_EXSTYLE);
	fail_if (!st && GetLastError () != NO_ERROR, 0, PE ("GetWindowLong failed for target window, %i", GetLastError (), 0, 0, 0));

	st |= WS_EX_LAYERED;

	SetLastError (0);
	st = SetWindowLong (hwnd, GWL_EXSTYLE, st);
	fail_if (!st && GetLastError () != NO_ERROR, 0, PE ("SetWindowLong failed for target window, %i", GetLastError (), 0, 0, 0));


	st = SetLayeredWindowAttributes (hwnd, 0x00ffffff, (BYTE)blend, LWA_ALPHA);
	fail_if (!st, 0, PE ("SetLayeredWindowAttribs failed, %i", GetLastError (), 0, 0, 0));

failure:
	return;
}



//(sl.win.trans [hwnd] [i:color])
void WinSetTrans (parerror_t *pe, HWND hwnd, COLORREF color)
{
	HMODULE hmod;
	BOOL (WINAPI * SetLayeredWindowAttributes) (HWND, COLORREF, BYTE, DWORD);
	DERR st;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	hmod = GetModuleHandle ("user32.dll");
	fail_if (!hmod, 0, PE ("GetModuleHandle failed for user32.dll, %i", GetLastError (), 0, 0, 0));

	SetLayeredWindowAttributes = (void *)GetProcAddress (hmod, "SetLayeredWindowAttributes");
	fail_if (!SetLayeredWindowAttributes, 0, PE ("GetProcAddress failed finding SetLayeredWindowAttributes (nt < 5.0?), %i", GetLastError (), 0, 0, 0));
	
	if ((int)color == -1)
	{
		SetLastError (0);
		st = GetWindowLong (hwnd, GWL_EXSTYLE);
		fail_if (!st && GetLastError () != NO_ERROR, 0, PE ("GetWindowLong failed for target window, %i", GetLastError (), 0, 0, 0));

		st &= ~WS_EX_LAYERED;

		SetLastError (0);
		st = SetWindowLong (hwnd, GWL_EXSTYLE, st);
		fail_if (!st && GetLastError () != NO_ERROR, 0, PE ("SetWindowLong failed for target window, %i", GetLastError (), 0, 0, 0));

		return; //that's all we had to do.
	}

	SetLastError (0);
	st = GetWindowLong (hwnd, GWL_EXSTYLE);
	fail_if (!st && GetLastError () != NO_ERROR, 0, PE ("GetWindowLong failed for target window, %i", GetLastError (), 0, 0, 0));

	st |= WS_EX_LAYERED;

	SetLastError (0);
	st = SetWindowLong (hwnd, GWL_EXSTYLE, st);
	fail_if (!st && GetLastError () != NO_ERROR, 0, PE ("SetWindowLong failed for target window, %i", GetLastError (), 0, 0, 0));

	st = SetLayeredWindowAttributes (hwnd, UtilReverseColor (color), 255, LWA_COLORKEY);
	fail_if (!st, 0, PE ("SetLayeredWindowAttribs failed, %i", GetLastError (), 0, 0, 0));

failure:
	return;

}


//(sl.win.ontop [hwnd] [i:bool ontop])
void WinOnTop (parerror_t *pe, HWND hwnd, int ontop)
{

	DERR st;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	if (ontop)
	{
		st = SetWindowPos (hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		fail_if (!st, 0, PE ("SetWindowPos failed to set HWND_TOPMOST, %i", GetLastError (), 0, 0, 0));
	}
	else
	{
		st = SetWindowPos (hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		fail_if (!st, 0, PE ("SetWindowPos failed to set HWND_NOTTOPMOST, %i", GetLastError (), 0, 0, 0));
	}

failure:
	return;
}

//(sl.win.setpos [hwnd] [i:left] [i:top] [i:width] [i:height])
void WinSetPos (parerror_t *pe, HWND hwnd, int left, int top, int width, int height)
{
	RECT r;
	DERR st;
	int w, h;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	//if any one parameter specifies a relative change, we need to get the original rect
	if (left == -555 || top == -555 || width == -555 || height == -555)
	{
		st = GetWindowRect (hwnd, &r);
		fail_if (!st, 0, PE ("Could not get starting client rect, %i", GetLastError (), 0, 0, 0));

		w = r.right - r.left;
		h = r.bottom - r.top;
	}

	if (left != -555)
		r.left = left;
	if (top != -555)
		r.top = top;
	if (width != -555)
		w = width;
	if (height != -555)
		h = height;

	st = SetWindowPos (hwnd, hwnd, r.left, r.top, w, h, SWP_NOZORDER);
	fail_if (!st, 0, PE ("SetWindowPos failed for window %X, %i", hwnd, GetLastError (), 0, 0));

failure:
	return;
}

//(sl.win.setpos [hwnd] [i:side] [i:newval])
void WinSetPosSingle (parerror_t *pe, HWND hwnd, int side, int val)
{
	RECT r;
	DERR st;
	int w, h;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));


	st = GetWindowRect (hwnd, &r);
	fail_if (!st, 0, PE ("Could not get starting client rect, %i", GetLastError (), 0, 0, 0));

	w = r.right - r.left;
	h = r.bottom - r.top;

	if (side == 1)
		r.left = val;
	else if (side == 2)
		r.top = val;
	else if (side == 3)
		w = val;
	else if (side == 4)
		h = val;
	else
		fail_if (TRUE, 0, PE ("Invalid side %i specified", side, 0, 0, 0));

	st = SetWindowPos (hwnd, hwnd, r.left, r.top, w, h, SWP_NOZORDER);
	fail_if (!st, 0, PE ("SetWindowPos failed for window %X, %i", hwnd, GetLastError (), 0, 0));

failure:
	return;
}

//(sl.win.getpos [hwnd] [i:side])
int WinGetPosSingle (parerror_t *pe, HWND hwnd, int side)
{
	RECT r;
	DERR st;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	st = GetWindowRect (hwnd, &r);
	fail_if (!st, 0, PE ("Could not get window rect, %i", GetLastError (), 0, 0, 0));

	if (side == 1)
		return r.left;
	else if (side == 2)
		return r.top;
	else if (side == 3)
		return r.right - r.left;
	else if (side == 4)
		return r.bottom - r.top;
	else
		fail_if (TRUE, 0, PE ("Invalid side %i specified", side, 0, 0, 0));

	//this line should be unreachable
	//return -1;
failure:
	return -1;
}




//(sl.win.workarea [i:left] [i:top] [i:right] [i:bottom])
void WinWorkArea (parerror_t *pe, int left, int top, int right, int bottom)
{
	RECT r;
	DERR st;

	r.left = left;
	r.top = top;
	r.right = right;
	r.bottom = bottom;

	st = SystemParametersInfo (SPI_SETWORKAREA, 0, &r, SPIF_SENDCHANGE);
	fail_if (!st, 0, PE ("Could not set work area, %i", GetLastError (), 0, 0, 0));

failure:
	return;
}

//(sl.win.workarea [i:side] [i:value])
void WinWorkAreaSingle (parerror_t *pe, int side, int newval)
{
	DERR st;
	RECT r;
	DEVMODE dm;

	st = SystemParametersInfo (SPI_GETWORKAREA, 0, &r, 0);
	fail_if (!st, 0, PE ("Could not get starting work area, %i", GetLastError (), 0, 0, 0));

	if (side == 0) //special case
	{
		//figure out the display resolution, because passing NULL for the rect does not reset the
		//work area like the docs say.
		dm.dmSize = sizeof (DEVMODE);
		dm.dmDriverExtra = 0;
		st = EnumDisplaySettings (NULL, ENUM_CURRENT_SETTINGS, &dm);
		fail_if (!st, 0, PE ("EnumDisplaySettings failed, %i", GetLastError (), 0, 0, 0));

		r.top = 0;
		r.left = 0;
		r.right = dm.dmPelsWidth;
		r.bottom = dm.dmPelsHeight;
		
		st = SystemParametersInfo (SPI_SETWORKAREA, 0, &r, SPIF_SENDCHANGE);
		fail_if (!st, 0, PE ("Could not restore work area to full screen, %i", GetLastError (), 0, 0, 0));
		return;
	}
	else if (side == 1)
		r.left = newval;
	else if (side == 2)
		r.top = newval;
	else if (side == 3)
		r.right = newval;
	else if (side == 4)
		r.bottom = newval;
	else
		fail_if (TRUE, 0, PE ("Invalid side %i specified", side, 0, 0, 0));

	st = SystemParametersInfo (SPI_SETWORKAREA, 0, &r, SPIF_SENDCHANGE);
	fail_if (!st, 0, PE ("Could not set work area, %i", GetLastError (), 0, 0, 0));

failure:
	return;
}

//(sl.win.getworkarea [i:side])
int WinGetWorkArea (parerror_t *pe, int side)
{
	DERR st;
	RECT r;

	st = SystemParametersInfo (SPI_GETWORKAREA, 0, &r, 0);
	fail_if (!st, 0, PE ("Could not get work area, %i", GetLastError (), 0, 0, 0));

	if (side == 1)
		return r.left;
	else if (side == 2)
		return r.top;
	else if (side == 3)
		return r.right;
	else if (side == 4)
		return r.bottom;
	else
		fail_if (TRUE, 0, PE ("Invalid side %i specified", side, 0, 0, 0));

	//can't get here
	//return -1;
failure:
	return -1;
}



int WinTileHorizontal (parerror_t *pe)
{
	DERR st;

	st = TileWindows (NULL, MDITILE_HORIZONTAL, NULL, 0, NULL);
	fail_if (!st, 0, PE ("TileWindow failed, %i", GetLastError (), 0, 0, 0));

	return st;
failure:
	return 0;
}

int WinTileVertical (parerror_t *pe)
{
	DERR st;

	st = TileWindows (NULL, MDITILE_VERTICAL, NULL, 0, NULL);
	fail_if (!st, 0, PE ("TileWindow failed, %i", GetLastError (), 0, 0, 0));

	return st;
failure:
	return 0;
}


void WinArrangeIcons (parerror_t *pe)
{
	DERR st;

	st = ArrangeIconicWindows (GetDesktopWindow ());
	fail_if (!st, 0, PE ("ArrangeIconicWindows failed, %i", GetLastError (), 0, 0, 0));

failure:
	return;
}

int WinCascadeWindows (parerror_t *pe)
{
	DERR st;

	st = CascadeWindows (NULL, 0, NULL, 0, NULL);
	fail_if (!st, 0, PE ("CascadeWindows failed, %i", GetLastError (), 0, 0, 0));

	return st;
failure:
	return 0;
}

#if 0

BOOL CALLBACK SdownProd (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND ctrl;
	UINT flags=0;
	int loff=0;
	
	switch (message)
	{
	case WM_INITDIALOG:
		ctrl = GetDlgItem (hwnd, IDC_SHUTDOWN);
		SendMessage (ctrl, BM_SETCHECK, BST_CHECKED, 0);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;
		case IDOK:
			ctrl = GetDlgItem (hwnd, IDC_SUSPEND);
			if (SendMessage (ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				EndDialog (hwnd, 0);
				SetSystemPowerState (0, FALSE);
				return TRUE;
			}
			ctrl = GetDlgItem (hwnd, IDC_SHUTDOWN);
			if (SendMessage (ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
				flags |= EWX_SHUTDOWN;
			ctrl = GetDlgItem (hwnd, IDC_POWERDOWN);
			if (SendMessage (ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
				flags |= EWX_POWEROFF;
			ctrl = GetDlgItem (hwnd, IDC_RESTART);
			if (SendMessage (ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
				flags |= EWX_REBOOT;
			ctrl = GetDlgItem (hwnd, IDC_RELOGIN);
			if (SendMessage (ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				flags |= EWX_REBOOT;
				loff=1;
			}
			ctrl = GetDlgItem (hwnd, IDC_FORCE);
			if (SendMessage (ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
				flags |= EWX_FORCE;
			if (flags != 0 || loff)
				ExitWindowsEx (flags, 0);
			EndDialog (hwnd, 0);
			return TRUE;
		}
	}
	return FALSE;
}


int CMD_Shutdown (int argc, char **argv)
{
	DialogBox(instance, MAKEINTRESOURCE(IDD_SHUTDOWN), NULL, SdownProd);
	return COMMAND_NOERROR;
}

#endif



HWND WinGetParent (parerror_t *pe, HWND hwnd)
{
	HWND out;
	DERR st;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	out = GetParent (hwnd);
	fail_if (!out && GetLastError () != NO_ERROR, 0, PE ("Error getting window parent, %i", GetLastError (), 0, 0, 0));

	return out;
failure:
	return NULL;
}

HWND WinSetParent (parerror_t *pe, HWND hwnd, HWND newparent)
{
	HWND out;
	DERR st;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	out = SetParent (hwnd, newparent);
	fail_if (!out && GetLastError () != NO_ERROR, 0, PE ("Error setting new parent, %i", GetLastError (), 0, 0, 0));

	return out;
failure:
	return NULL;
}


int WinGetWindowLong (parerror_t *pe, HWND hwnd, int index)
{
	DERR st;
	int out;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	SetLastError (0);
	out = GetWindowLong (hwnd, index);
	fail_if (!out && GetLastError () != NO_ERROR, 0, PE ("Error getting window long at %i; %i", index, GetLastError (), 0, 0));

	return out;

failure:
	return 0;
}

int WinSetWindowLong (parerror_t *pe, HWND hwnd, int index, int newval)
{
	DERR st;
	int out;
	
	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	SetLastError (0);
	out = SetWindowLong (hwnd, index, newval);
	fail_if (!out && GetLastError () != NO_ERROR, 0, PE ("SetWindowLong failed on index %i; %i", index, GetLastError (), 0, 0));

	return out;
failure:
	return 0;
}

void WinShowWindow (parerror_t *pe, HWND hwnd)
{
	DERR st;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	ShowWindowAsync (hwnd, SW_SHOW);

	//return;
failure:
	return;
}

void WinHideWindow (parerror_t *pe, HWND hwnd)
{
	DERR st;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	ShowWindowAsync (hwnd, SW_HIDE);

	//return;
failure:
	return;
}

int WinIsVisible (parerror_t *pe, HWND hwnd)
{
	DERR st;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	return IsWindowVisible (hwnd);
failure:
	return 0;
}


HWND WinSetFocus (parerror_t *pe, HWND hwnd)
{
	DWORD tid;
	DWORD othertid;
	DERR st;
	HWND out;
	int attached = 0;

	if (!hwnd)
	{
		SetLastError (NO_ERROR);
		out = SetFocus (NULL);
		fail_if (!out && GetLastError () != NO_ERROR, 0, PE ("SetFocus failed, %i", GetLastError (), 0, 0, 0));

		return out;
	}

	tid = GetCurrentThreadId ();
	othertid = GetWindowThreadProcessId (hwnd, NULL);
	fail_if (!othertid, 0, PE ("Unable to get other thread's id, %i", GetLastError (), 0, 0, 0));

	if (tid != othertid)
	{
		st = AttachThreadInput (tid, othertid, TRUE);
		fail_if (!st, 0, PE ("Could not attach to target thread %i, %i", othertid, GetLastError (), 0, 0));
		attached = 1;
	}

	SetLastError (NO_ERROR);
	out = SetFocus (hwnd);
	fail_if (!out && GetLastError () != NO_ERROR, 0, PE ("SetFocus failed, %i", GetLastError (), 0, 0, 0));
	//if it's NULL we'll just return that.

	if (attached)
	{
		AttachThreadInput (tid, othertid, FALSE);
		//if this fails, we're screwed.
	}

	return out;
failure:

	if (attached)
		AttachThreadInput (tid, othertid, FALSE);

	return NULL;
}


void WinSetForeground (parerror_t *pe, HWND hwnd)
{
	DERR st;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	st = SetForegroundWindow (hwnd);
	fail_if (!st, 0, PE ("SetForegroundWindow call failed.  (It does not return error information)", 0, 0, 0, 0));

	return;
failure:
	return;
}


void WinRaise (parerror_t *pe, HWND hwnd)
{
	DERR st;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	st = SetWindowPos (hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE);
	fail_if (!st, 0, PE ("SetWindowPos failed, %i", GetLastError (), 0, 0, 0));

	st = SetWindowPos (hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_ASYNCWINDOWPOS);
	fail_if (!st, 0, PE ("SetWindowPos failed, %i", GetLastError (), 0, 0, 0));

	st = SetWindowPos (hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_ASYNCWINDOWPOS);
	fail_if (!st, 0, PE ("SetWindowPos failed, %i", GetLastError (), 0, 0, 0));

	//return;
failure:
	return;
}


int WinSetWindowText (parerror_t *pe, HWND hwnd, char *text)
{
	DERR st;
	LRESULT res;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));
	fail_if (!text, 0, PE_BAD_PARAM (2));

	st = SendMessageTimeout (hwnd, WM_SETTEXT, 0, (LPARAM)text, SMTO_ABORTIFHUNG | SMTO_BLOCK, 2000, &res);
	fail_if (!st && GetLastError () == NO_ERROR, 0, PE ("WM_SETTEXT message timed out", 0, 0, 0, 0));
	fail_if (!st, 0, PE ("SendMessageTimeout failed, %i", GetLastError (), 0, 0, 0));

	return res;
failure:
	return 0;
}

char *WinGetWindowText (parerror_t *pe, HWND hwnd)
{
	DERR st;
	char *out = NULL;
	int len, dummy;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	st = SendMessageTimeout (hwnd, WM_GETTEXTLENGTH, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 2000, &len);
	fail_if (!st && GetLastError () == NO_ERROR, 0, PE ("WM_GETTEXTLENGTH message timed out", 0, 0, 0, 0));
	fail_if (!st, 0, PE ("SendMessageTimeout failed, %i", GetLastError (), 0, 0, 0));

	out = ParMalloc (len + 2);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (len + 2));

	if (len)
	{
		st = SendMessageTimeout (hwnd, WM_GETTEXT, len + 1, (LPARAM)out, SMTO_ABORTIFHUNG | SMTO_BLOCK, 2000, &dummy);
		fail_if (!st && GetLastError () == NO_ERROR, 0, PE ("WM_GETTEXT message timed out", 0, 0, 0, 0));
		fail_if (!st, 0, PE ("SendMessageTimeout failed, %i", GetLastError (), 0, 0, 0));
	}
	else
	{
		*out = '\0';
	}

	out[len + 1] = '\0';

	return out;
failure:
	if (out)
		ParFree (out);

	return NULL;
}



#define WINDOWSHADE_PROP "PIX_WSmode"
void WinSetWindowshade (parerror_t *pe, HWND hwnd)
{
	DERR st;
	int temp;
	int propset = 0;
	RECT r;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	temp = (int)GetProp (hwnd, WINDOWSHADE_PROP);
	//GetProp apparently doesn't return failures
	if (temp)
	{
		//it's set, so disable windowshade

		st = GetWindowRect (hwnd, &r);
		fail_if (!st, 0, PE ("Could not get window rect, %i", GetLastError (), 0, 0, 0));

		st = SetWindowPos (hwnd, NULL, r.left, r.top, r.right - r.left, temp, SWP_NOZORDER);
		fail_if (!st, 0, PE ("Could not set window pos, %i", GetLastError (), 0, 0, 0));

		RemoveProp (hwnd, WINDOWSHADE_PROP);

		return;
	}

	//it isn't set, so enable windowshade.

	st = GetWindowRect (hwnd, &r);
	fail_if (!st, 0, PE ("Could not get window rect, %", GetLastError (), 0, 0, 0));

	st = SetProp (hwnd, WINDOWSHADE_PROP, (HANDLE)(r.bottom - r.top));
	fail_if (!st, 0, PE ("Could not save old height value, %i", GetLastError (), 0, 0, 0));
	propset = 1;

	st = SetWindowPos (hwnd, NULL, r.left, r.top, r.right - r.left, GetSystemMetrics (SM_CYMIN), SWP_NOZORDER | SWP_NOSENDCHANGING);
	fail_if (!st, 0, PE ("Could not change window pos, %i", GetLastError (), 0, 0, 0));

	return;

failure:
	if (propset)
		RemoveProp (hwnd, WINDOWSHADE_PROP);
}

int WinIsWindowshade (parerror_t *pe, HWND hwnd)
{
	DERR st;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	return GetProp (hwnd, WINDOWSHADE_PROP) ? 1 : 0;
failure:
	return 0;
}


char *WinGetThreadProdId (parerror_t *pe, HWND hwnd)
{
	char *out = NULL;
	DWORD threadid, procid;
	char buffer[40];
	DERR st;

	threadid = GetWindowThreadProcessId (hwnd, &procid);

	PlSnprintf (buffer, 39, "%i %i", procid, threadid);
	buffer[39] = '\0';

	out = ParMalloc (PlStrlen (buffer) + 1);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (PlStrlen (buffer) + 1));
	PlStrcpy (out, buffer);

	return out;
failure:
	return NULL;
}

