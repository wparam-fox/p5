/* Copyright 2007 wParam, licensed under the GPL */
//#define WINVER 0x500
#include <windows.h>
#include "runbox.h"
#include "resource.h"


typedef struct history_t
{
	struct history_t *next;
	struct history_t **self;

	char line[0];
} history_t;


#define RBSTAT_HIDDEN 0
#define RBSTAT_SUCCESS 1
#define RBSTAT_FAILURE 2
#define RBSTAT_PASSIVE 3
typedef struct
{
	HWND hwnd;
	HFONT font;
	HWND edit; //save it to save time
	HWND report;
	//parser_t *par;

	int parserunning;

	int active;
	int closing; //all messages are ignored if this is set. (set during closing)
	int border;
	int multiline;

	int errstat;
	//char *error; //last error string.  If status is an error and error is NULL, "Out of memory" will be displayed
	//int lastline;
	HICON icons[4]; //1 icon for each RBSTAT_*

	history_t *hist; //points into RbHistory somewhere.

} runbox_t;

#define RBSTYLE_FLOATING 1
#define RBSTYLE_TOP 2

runbox_t *RbData = NULL;
int RbHardLimit = 8192;
int RbLineHeight = 16;
int RbMaxHistory = 50;
parser_t *RbParser = NULL;
history_t *RbHistory = NULL;
int RbHistoryCount = 0;

//appearance settings/configuration
int RbStyle = RBSTYLE_TOP;
int RbAlwaysOnTop = TRUE;
int RbShowInAltTab = FALSE;
int RbAllowClose = TRUE;
int RbMultiline = TRUE;

#define LIGHT_WIDTH 20

#define WM_ENTERPRESSED (WM_USER + 2)
#define WM_RESET (WM_USER + 3)
#define WM_DOSIZE (WM_USER + 4)
#define WM_CHECKHEIGHT (WM_USER + 5)
#define WM_DIMLIGHT (WM_USER + 6)
#define WM_HISTORYMOVE (WM_USER + 7)
#define WM_SELECTALL (WM_USER + 8)


void RbHistChopEnd (history_t **hist)
{
	history_t *temp;

	while (*hist)
	{
		if (!(*hist)->next) //end of the list.
		{
			temp = *hist;
			*hist = NULL; //removes temp from the list

			PlFree (temp);
			return;
		}

		hist = &(*hist)->next;
	}

	//I don't think you can get here

}

int RbAddToHistory (char *line)
{
	history_t *walk;

	//walk through, add the item if it's not found
	//if it is found, move it to the head.
	//if count exceeds 

	walk = RbHistory;
	while (walk)
	{
		if (strcmp (line, walk->line) == 0)
		{
			//ok.  a match.  unlink it.
			if (walk->next)
				walk->next->self = walk->self;
			*walk->self = walk->next;

			//unlinked.  Make it the head.
			walk->next = RbHistory;
			walk->self = &RbHistory;
			if (RbHistory)
				RbHistory->self = &walk->next;
			RbHistory = walk;

			return 1;
		}

		walk = walk->next;
	}

	//not found; add it.

	walk = PlMalloc (sizeof (history_t) + strlen (line) + 1);
	if (!walk)
		return 0;

	strcpy (walk->line, line);
	walk->next = RbHistory;
	walk->self = &RbHistory;
	if (RbHistory)
		RbHistory->self = &walk->next;
	RbHistory = walk;

	RbHistoryCount++;
	if (RbHistoryCount > RbMaxHistory)
	{
		RbHistChopEnd (&RbHistory);
		RbHistoryCount--;
	}

	return 1;
}

void RbClearHistory (void)
{
	history_t *walk, *temp;

	walk = RbHistory;
	while (walk)
	{
		temp = walk->next;
		PlFree (walk);
		walk = temp;
	}

	RbHistory = NULL;
}


int RbAppendChar (HANDLE file, char c, parerror_t *pe, char *buffer, int *blen, int bmax)
{
	DERR st;
	DWORD read;


	buffer[*blen] = c;
	(*blen)++;

	if (*blen == bmax) //buffer is now full.
	{
		//write out the data.
		st = WriteFile (file, buffer, bmax, &read, NULL);
		fail_if (!st || read != (unsigned)bmax, 0, PE ("WriteFile failed, %i", GetLastError (), 0, 0, 0));

		*blen = 0;
	}

	return 1;
failure:
	return 0;
}

int RbAppendString (HANDLE file, char *str, parerror_t *pe, char *buffer, int *blen, int bmax)
{
	while (*str)
	{
		if (!RbAppendChar (file, *str, pe, buffer, blen, bmax))
			return 0;

		str++;
	}

	//don't write '\0'
	return 1;
}


#define RHIST_MAX 1024
void RbWriteHistory (parerror_t *pe, char *filename)
{
	DERR st;
	HANDLE file = NULL;
	history_t *walk;
	char buffer[RHIST_MAX];
	DWORD blen, read;
	char *a, temp;

	fail_if (!PlIsMainThread (), 0, PE ("rb.savehistory must run in the main thread", 0, 0, 0, 0));
	fail_if (!filename, 0, PE_BAD_PARAM (1));

	file = CreateFile (filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
	fail_if (file == INVALID_HANDLE_VALUE, GetLastError (), file = NULL; PE_STR ("Could not create file %s, %i", COPYSTRING (filename), st, 0, 0));

	blen = 0;

	st = RbAppendString (file, "(p5.exec.fg.require)\r\n", pe, buffer, &blen, RHIST_MAX);
	fail_if (!st, 0, 0); //pe is already setup

	//find the end of the list.
	walk = RbHistory;
	while (walk)
	{
		if (!walk->next)
			break;

		walk = walk->next;
	}

	while (walk)
	{
		st = RbAppendString (file, "(rb.addhistory \"", pe, buffer, &blen, RHIST_MAX);
		fail_if (!st, 0, 0);

		a = walk->line;
		while (*a)
		{
			temp = *a;

			if (*a == '\\')
			{
				st = RbAppendChar (file, '\\', pe, buffer, &blen, RHIST_MAX);
				fail_if (!st, 0, 0);
			}
			else if (*a == '\"')
			{
				st = RbAppendChar (file, '\\', pe, buffer, &blen, RHIST_MAX);
				fail_if (!st, 0, 0);
			}
			else if (*a == '\r')
			{
				st = RbAppendChar (file, '\\', pe, buffer, &blen, RHIST_MAX);
				fail_if (!st, 0, 0);
				temp = 'r';
			}
			else if (*a == '\n')
			{
				st = RbAppendChar (file, '\\', pe, buffer, &blen, RHIST_MAX);
				fail_if (!st, 0, 0);
				temp = 'n';
			}


			st = RbAppendChar (file, temp, pe, buffer, &blen, RHIST_MAX);

			a++;
		}

		st = RbAppendString (file, "\")\r\n", pe, buffer, &blen, RHIST_MAX);

		if (walk->self == &RbHistory)
			break;
		walk = (history_t *)walk->self;
	}

	//write out the remainder.
	if (blen)
	{
		st = WriteFile (file, buffer, blen, &read, NULL);
		fail_if (!st || read != blen, 0, PE ("WriteFile failed, %i", GetLastError (), 0, 0, 0));
	}

	CloseHandle (file);

	return;
failure:

	if (file)
		CloseHandle (file);

	return;
}
















LRESULT CALLBACK RbEditSubclassProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
	case WM_CHAR:
		switch (wParam)
		{
		case VK_RETURN:
			if (!SendMessage (GetParent (hwnd), WM_ENTERPRESSED, 0, 0))
				return 0;
			break;

	
		case VK_BACK:
		case VK_DELETE:
			PostMessage (GetParent (hwnd), WM_CHECKHEIGHT, 0, 0);
			break;

		
		case VK_ESCAPE:
			SendMessage (GetParent (hwnd), WM_RESET, 0, 0);
			return 0;

		


		
		}

		//MULTIPLE runbox issue:
		//hack for speeding things up: only post a light off message if the light isn't
		//already off.  If we support multiple runboxes, this will need to change.
		if (RbData && RbData->errstat != RBSTAT_PASSIVE)
			PostMessage (GetParent (hwnd), WM_DIMLIGHT, 0, 0);

		break;


	case WM_KEYDOWN:
		{
			//DWORD ax, bx, ay, by;
			//LRESULT res;

			switch (wParam)
			{
			/*
			case VK_DOWN:
			case VK_UP:
				CallWindowProc ((WNDPROC)GetWindowLong (hwnd, GWL_USERDATA), hwnd, EM_GETSEL, (WPARAM)&ax, (LPARAM)&bx);

				res = CallWindowProc ((WNDPROC)GetWindowLong (hwnd, GWL_USERDATA), hwnd, message, wParam, lParam);

				CallWindowProc ((WNDPROC)GetWindowLong (hwnd, GWL_USERDATA), hwnd, EM_GETSEL, (WPARAM)&ay, (LPARAM)&by);

				//if the cursor didn't move during the keypress, then we must be at either the top or the
				//bottom, and thus should browse history
				if (ax == ay && bx == by) 
					PostMessage (GetParent (hwnd), WM_HISTORYMOVE, 0, wParam == VK_DOWN ? -1 : 1);

				if (RbData && RbData->errstat != RBSTAT_PASSIVE)
					PostMessage (GetParent (hwnd), WM_DIMLIGHT, 0, 0);
				
				return res;
			*/

			case VK_NEXT:
				PostMessage (GetParent (hwnd), WM_HISTORYMOVE, 0, -1);
				if (RbData && RbData->errstat != RBSTAT_PASSIVE)
					PostMessage (GetParent (hwnd), WM_DIMLIGHT, 0, 0);
				break;

			case VK_PRIOR:
				PostMessage (GetParent (hwnd), WM_HISTORYMOVE, 0, 1);
				if (RbData && RbData->errstat != RBSTAT_PASSIVE)
					PostMessage (GetParent (hwnd), WM_DIMLIGHT, 0, 0);
				break;
			}

		}
		break;


	}

	return CallWindowProc ((WNDPROC)GetWindowLong (hwnd, GWL_USERDATA), hwnd, message, wParam, lParam);
}

int RbCountNesting (char *line)
{
	int out = 0;
	int quoting = 0;

	while (*line)
	{
		if (!quoting)
		{
			if (*line == '(')
				out++;

			if (*line == ')')
				out--;

			if (*line == '\"')
				quoting = 1;
		}
		else
		{
			if (*line == '\\' && *(line + 1) == '\\') //quoted \, pass both
				line++;
			else if (*line == '\\' && *(line + 1) == '\"') //quoted quote, don't stop
				line++; //must bypass 2 charactert this time
			else if (*line == '\"')
				quoting = 0;
		}

		line++;
	}

	return out;
}

int RbResize (runbox_t *run)
{
	RECT r, client;
	int h;
	DERR st;
	char *fmt;

	int ex, ey; //extra pixels in x and y that make up the window border, etc.
	int cx, cy; //total size we have to work with in the client area
	int numlines;
	int reportlines;
	int b = run->border;
	//int ee = 2; //extra pixels the edit box needs for things to align properly

	h = RbLineHeight;
	fail_if (!h, 0, fmt = "WTF man, don't set RbLineHeight to 0!");

	//first calculate ex and ey by dthe difference between the window and client rect sizes
	st = GetWindowRect (run->hwnd, &r);
	fail_if (!st, GetLastError (), fmt = "GetWindowRect failed, %i\n");
	st = GetClientRect (run->hwnd, &client);
	fail_if (!st, GetLastError (), fmt = "GetClientRect failed, %i\n");

	ex = (r.right - r.left) - (client.right - client.left);
	ey = (r.bottom - r.top) - (client.bottom - client.top);
	cx = client.right - client.left;
	cy = client.bottom - client.top;

	if (!run->active)
	{
		st = SetWindowPos (run->hwnd, NULL, 0, 0, cx + ex, h + ey + b * 2, SWP_NOMOVE | SWP_NOZORDER);
		fail_if (!st, GetLastError (), fmt = "SetWindowPos failed, %i\n");

		st = SetWindowPos (run->edit, NULL, 0, 0, cx, h + b * 2, SWP_NOZORDER);
		fail_if (!st, GetLastError (), fmt = "SetWindowPos failed for edit window, %i\n");

		return 1;
	}

	numlines = SendMessage (run->edit, EM_GETLINECOUNT, 0, 0);
	fail_if (!numlines, GetLastError (), fmt = "SendMessage returned 0 for the number of lines, %i\n");

	if (run->errstat != RBSTAT_HIDDEN) //if we're displaying a success or failure
	{
		reportlines = SendMessage (run->report, EM_GETLINECOUNT, 0, 0);
		fail_if (!reportlines, GetLastError (), fmt = "SendMessage returned 0 for the number of lines, %i\n");
	}
	else
	{
		reportlines = 0;
	}

	//PlPrintf ("Lines: %i %i %i %i %i %i\n", numlines, reportlines, cx, cy, ex + cx, ey + (h * (reportlines + numlines)));

	//first set the height of the main window
	st = SetWindowPos (run->hwnd, NULL, 0, 0, ex + cx, ey + (h * (reportlines + numlines)) + b * 2, SWP_NOMOVE | SWP_NOZORDER);
	fail_if (!st, GetLastError (), fmt = "SetWindowPos failed for main window, %i\n");

	//first edit window
	st = SetWindowPos (run->edit, NULL, 0, 0, cx, h * numlines + b * 2, SWP_NOZORDER);
	fail_if (!st, GetLastError (), fmt = "SetWindowPos failed for edit window, %i\n");

	st = SetWindowPos (run->report, NULL, LIGHT_WIDTH, h * numlines + b * 2, cx - LIGHT_WIDTH, h * (reportlines == 0 ? 1 : reportlines), SWP_NOZORDER);
	fail_if (!st, GetLastError (), fmt = "SetWindowPos failed for report window, %i\n");

	//ok, we should be good.
	return 1;

failure:
	PlPrintf (fmt, st);

	return 0;
}

int RbGetHeight (runbox_t *run)
{
	RECT r, client;
	int ch;
	DERR st;
	char *fmt = "BEJEEZUS!";

	int ey; //extra pixels in x and y that make up the window border, etc.
	//int cx, cy; //total size we have to work with in the client area
	int numlines;
	int reportlines;
	int b = run->border;
	//int ee = 2; //extra pixels the edit box needs for things to align properly

	ch = RbLineHeight;
	fail_if (!ch, 0, fmt = "WTF man, don't set RbLineHeight to 0!");

	//first calculate ex and ey by dthe difference between the window and client rect sizes
	st = GetWindowRect (run->hwnd, &r);
	fail_if (!st, GetLastError (), fmt = "GetWindowRect failed, %i\n");
	st = GetClientRect (run->hwnd, &client);
	fail_if (!st, GetLastError (), fmt = "GetClientRect failed, %i\n");

	ey = (r.bottom - r.top) - (client.bottom - client.top);

	numlines = SendMessage (run->edit, EM_GETLINECOUNT, 0, 0);
	fail_if (!numlines, GetLastError (), fmt = "SendMessage returned 0 for the number of lines, %i\n");

	if (run->errstat != RBSTAT_HIDDEN) //if we're displaying a success or failure
	{
		reportlines = SendMessage (run->report, EM_GETLINECOUNT, 0, 0);
		fail_if (!reportlines, GetLastError (), fmt = "SendMessage returned 0 for the number of lines, %i\n");
	}
	else
	{
		reportlines = 0;
	}

	return ey + (ch * (reportlines + numlines)) + b * 2;
failure:
	return 0;

}

void RbInvalidateLight (runbox_t *run)
{
	RECT r;
	int st, temp;;

	st = GetWindowRect (run->edit, &r);
	fail_if (!st, 0, 0);

	temp = r.bottom - r.top;

	st = GetWindowRect (run->hwnd, &r);
	fail_if (!st, 0, 0);

	r.bottom = r.bottom - r.top; //r.bottom of the update region is the height of the window
	r.top = temp;
	r.left = 0;
	r.right = LIGHT_WIDTH;

	InvalidateRect (run->hwnd, &r, FALSE);
failure:
	return;
}

void RbSetNonHidden (runbox_t *run, char *str, int type)
{
	if (!str)
		str = "(null)";

	SendMessage (run->report, WM_SETTEXT, 0, (LPARAM)str);

	if (run->errstat != type)
		RbInvalidateLight (run);

	run->errstat = type;

	RbResize (run);

}

void RbSetFailure (runbox_t *run, char *str)
{
	RbSetNonHidden (run, str, RBSTAT_FAILURE);
}

void RbSetSuccess (runbox_t *run, char *str)
{
	RbSetNonHidden (run, str, RBSTAT_SUCCESS);
}

void RbSetPassive (runbox_t *run)
{
	//if the error bar is hidden, don't do anything to it.
	//SetPassive's only purpose is to set the icon from red/green
	//to grey, indicating that the info listed in the box is no
	//longer necessarily valid for the command in the entry box.

	if (run->errstat != RBSTAT_HIDDEN)
	{
		if (run->errstat != RBSTAT_PASSIVE)
			RbInvalidateLight (run);

		run->errstat = RBSTAT_PASSIVE;

		//the text string stays the same, passive is only set 
	}
}

void RbSetHidden (runbox_t *run)
{
	run->errstat = RBSTAT_HIDDEN;
	RbResize (run);
}

char RbGetFirstNonWhitespace (char *a)
{
	while (*a && (*a == ' ' || *a == '\t' || *a == '\n' || *a == '\r'))
		a++;
	return *a;
}


//always return failure.  We need RbProcEnterPressed to return 0
//so enter is not inserted into the box
int RbRunProgram (runbox_t *run, char *cmdline)
{
	DERR st;
	char buffer[256];

	st = PlProperShell (cmdline);
	if (!DERR_OK (st))
	{
		PlSnprintf (buffer, 255, "ProperShell returned failure; {%s,%i}", PlGetDerrString (st), GETDERRINFO (st));
		buffer[255] = '\0';
		RbSetFailure (run, buffer);
		return 0;
	}
	else
	{
		RbSetSuccess (run, "ProperShell returned success");
		return 0;
	}

}

//always return failure.  We need RbProcEnterPressed to return 0
//so enter is not inserted into the box
int RbRunInP3 (runbox_t *run, char *line)
{
	DERR st;
	char buffer[256];

	st = PlPixshellThreeRun (line, 1);
	if (!DERR_OK (st))
	{
		PlSnprintf (buffer, 255, "PixshellThreeRun returned {%s,%i}", PlGetDerrString (st), GETDERRINFO (st));
		buffer[255] = '\0';
		RbSetFailure (run, buffer);
		return 0;
	}
	else
	{
		RbSetSuccess (run, "Command sent successfully");
		return 0;
	}

}

LRESULT RbProcEnterPressed (runbox_t *run)
{
	char *buffer = NULL;
	int nesting;
	int len = SendMessage (run->edit, WM_GETTEXTLENGTH, 0, 0);
	DERR st;
	char *output;
	char error[1024];
	char firstchar;
	POINT pos;

	fail_if (len > RbHardLimit, 0, RbSetFailure (run, "Runbox size limit reached; consider using rb.limit to increase it."));

	buffer = PlMalloc (len + 1);
	fail_if (!buffer, 0, RbSetFailure (run, "Out of memory allocating temp buffer."));

	SendMessage (run->edit, WM_GETTEXT, len + 1, (LPARAM)buffer);

	firstchar = RbGetFirstNonWhitespace (buffer);

	if (firstchar == '(')
	{
		nesting = RbCountNesting (buffer);

		if (run->multiline && nesting == 0 && (GetAsyncKeyState (VK_SHIFT) & 0x8000) == 0)
		{
			//this doesn't need to be done if the line isn't complete

			//make sure the cursor is at the end of the command.  If it isn't, move it there
			//and pretend like the command is incomplete.  (It is REALLY bad to go back to a
			//a history command, planning to add a line to it, only to hit enter and have the
			//command run because it was technically complete.  This way, if you didn't want
			//to run it, it isn't run, and if you did want to run it, you only have to hit
			//enter again.
			st = GetCaretPos (&pos);
			fail_if (!st, 0, RbSetFailure (run, "ERROR: Could not get caret position."));

			st = SendMessage (run->edit, EM_CHARFROMPOS, 0, MAKELPARAM (pos.x, pos.y));
			if (LOWORD (st) != len)
			{
				//SendMessage (run->edit, EM_SETSEL, len, len);
				nesting = -1; //force no parse
			}
		}

		//definitely DO need to test this again.
		if (nesting == 0) //full expression; parse it
		{
			fail_if (run->parserunning, 0, RbSetFailure (run, "ERROR: Current command not yet finished."));

			//mark this in case the parse runs something with a message loop in it (such as a message
			//box or bg.sync)
			run->parserunning = 1;
			st = PlRunParser (RbParser, buffer, &output, &nesting, error, 1024);
			run->parserunning = 0;

			//MULTIPLE runbox issue:
			//it's possible that the runbox may have been destroyed, so check for
			//the value of RbData before touching *run again.  This will be a 
			//real bitch to fix if we try to support multiple runboxen
			if (RbData)
			{
				if (!DERR_OK (st)) //if the parse failed
					RbSetFailure (run, error);

				if (DERR_OK (st)) //or if we succeeded
					RbSetSuccess (run, output);

				if (nesting) //if RbCountNesting failed to count properly
					RbSetFailure (run, "RbCountNesting has failed to detect the state of the expression");

			}

			//regardless, now we free output and reset the parser, just in case
			if (output)
				ParFree (output);

			PlResetParser (RbParser);

			//add the line to the history
			RbAddToHistory (buffer);
			run->hist = RbHistory; //buffer should now be the first entry in history

			PlFree (buffer);

			SendMessage (run->edit, EM_SETSEL, 0, -1);

			return 0;
		}

		//don't let an enter get in if it's multiline
		fail_if (!run->multiline, 0, RbSetFailure (run, "ERROR: Line is not complete"));


		//RbResize (run); --need to do this after the edit box gets the enter message
		//otherwise, EM_GETLINECOUNT returns 1 too few
		st = PostMessage (run->hwnd, WM_DOSIZE, 0, 0);
		fail_if (!st, 0, RbSetFailure (run, "Failed to post size message to main runbox window"));
	}
	else if (firstchar == '\0')
	{
		;
	}
	else if (firstchar == '!')
	{
		//send it to p3, if it's running

		//add the line to the history
		RbAddToHistory (buffer);
		run->hist = RbHistory; //buffer should now be the first entry in history

		st = RbRunInP3 (run, buffer);
		fail_if (!st, 0, 0);
	}
	else
	{
		//add the line to the history
		RbAddToHistory (buffer);
		run->hist = RbHistory; //buffer should now be the first entry in history

		//try to run it as a process
		st = RbRunProgram (run, buffer);
		fail_if (!st, 0, 0); //RbSetFailure already called
	}


	PlFree (buffer);

	return 1;

failure:
	if (buffer)
		PlFree (buffer);

	return 0;
}

void RbDestroy (runbox_t *run);
void RbCloseRunbox (runbox_t *run)
{
	//MULTIPLE runbox issue:
	//obviously this needs to change for mrunboxen

	RbDestroy (run);
	RbData = NULL;
}

void RbProcPaint (runbox_t *run)
{
	PAINTSTRUCT ps;
	HDC hdc = NULL;
	DERR st;
	char *fmt = "BEJEEZUS!";

	hdc = BeginPaint (run->hwnd, &ps);
	fail_if (!hdc, GetLastError (), fmt = "BeginPaint failed, %i");

	//st = BitBlt (hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top, NULL, 0, 0, BLACKNESS);
	//fail_if (!st, GetLastError (), fmt = "BitBlt failed, %i");

	if (run->errstat >= 1 && run->errstat <= 3)
	{
		int ch, numlines; 

		ch = RbLineHeight;
		fail_if (!ch, 0, fmt = "WTF why is RbLineHeight 0?");

		numlines = SendMessage (run->edit, EM_GETLINECOUNT, 0, 0);
		fail_if (!numlines, GetLastError (), fmt = "SendMessage returned 0 for the number of lines, %i\n");

		st = DrawIconEx (hdc, 0, ch * numlines + run->border * 2, run->icons[run->errstat], 16, 16, 0, NULL, DI_NORMAL);
		fail_if (!st, GetLastError (), fmt = "DrawIcon failed, %i");
	}

	EndPaint (run->hwnd, &ps);
	hdc = NULL;

	return;
failure:
	if (hdc)
		EndPaint (run->hwnd, &ps);

	PlPrintf (fmt, st);
}

void RbMoveHistory (runbox_t *run, int direction)
{

	//first, set the new run->hist pointer.  Make sure not to move
	//past the end of the list or behind the beginning.
	if (run->hist)
	{
		if (direction < 0)
		{
			if (run->hist->self != &RbHistory)
				run->hist = (history_t *)run->hist->self; //this is why the "next" must be the first element in history_t
		}
		else
		{
			if (run->hist->next)
				run->hist = run->hist->next;
		}
	}
	else
	{
		//if it hasn't been set yet, set it to the head of the history.
		run->hist = RbHistory;
	}

	//this can happen if no history is stored yet and he presses up or down
	if (!run->hist)
		return;

	//so now run->hist points to the new thing, or to the current, if we're
	//at one of the ends.  So update the edit box.

	SetWindowText (run->edit, run->hist->line);

	SendMessage (run->edit, EM_SETSEL, 0, -1);
}


#define GetNumLines(hwnd) SendMessage (hwnd, EM_GETLINECOUNT, 0, 0)

LRESULT CALLBACK RbProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//MULTIPLE runbox issue:
	//always access RbData through this run pointer.  That way, if we want to
	//support multiple runboxes for some reason, we'll be able to by just changing
	//this line and the workings of RbCloseRunbox above
	runbox_t *run = RbData;

	if (!run)
	{
		//PlPrintf ("P5 RUNBOX ERROR: RbData has been lost.\n");
		return DefWindowProc (hwnd, message, wParam, lParam);
	}

	if (run->closing)
		return DefWindowProc (hwnd, message, wParam, lParam);
	

	switch (message)
	{
	//case WM_CREATE:
		//return RbProcCreate (hwnd, message, wParam, lParam);
	case WM_CLOSE:
		if (RbAllowClose)
			RbCloseRunbox (run);
		return 0;

	case WM_WINDOWPOSCHANGING:
		//when the runbox is inactive, we want to violate the normal height restrictions,
		//so let anything go if the window is inactive
		if (!run->active)
			return 0;

		if (!(((WINDOWPOS *)lParam)->flags & SWP_NOSIZE))
		{
			int temp = RbGetHeight (run);

			//if there wasn't an error, enforce the height.  If we don't do
			//this, really odd things start to happen, like the window rect will
			//change size for no reason while the user is draggin the window edge.
			if (temp)
				((WINDOWPOS *)lParam)->cy = temp;
		}
		return 0;

	case WM_WINDOWPOSCHANGED:
		//likewise for this message and active status
		if (!run->active)
			return 0;
		RbResize (run);
		return 0;

	case WM_CHECKHEIGHT:
		{
			
			RbResize (run);

			return 1;

		}

	case WM_ENTERPRESSED:
		return RbProcEnterPressed (run);


	case WM_DOSIZE:
		RbResize (run);
		return 0;

	case WM_RESET:
		PlResetParser (RbParser);
		SetWindowText (run->edit, "");
		RbSetHidden (run);

		return 0;

	case WM_SETFOCUS:
		SetFocus (run->edit);
		return 0;

	case WM_LBUTTONDOWN:
		RbSetHidden (run);
		return 0;

	case WM_PAINT:
		RbProcPaint (run);
		return 0;

	case WM_DIMLIGHT:
		RbSetPassive (run);
		return 0;

	case WM_ACTIVATE:
		if (LOWORD (wParam) != WA_INACTIVE)
		{
			run->active = 1;
			RbResize (run);
		}
		else
		{
			run->active = 0;
			RbResize (run);
		}
		return 0;

	case WM_HISTORYMOVE:
		RbMoveHistory (run, lParam);
		RbResize (run);
		return 0;

	case WM_SELECTALL:
		SendMessage (run->edit, EM_SETSEL, 0, -1);
		return 0;

	}

	return DefWindowProc (hwnd, message, wParam, lParam);
}



runbox_t *RbCreate (parerror_t *pe)
{
	DERR st;
	runbox_t *run = NULL;
	LONG temp;

	run = PlMalloc (sizeof (runbox_t));
	fail_if (!run, 0, PE_OUT_OF_MEMORY (sizeof (runbox_t)));
	PlMemset (run, 0, sizeof (runbox_t)); 

	if (RbStyle == RBSTYLE_FLOATING)
	{
		run->hwnd = CreateWindowEx (WS_EX_TOOLWINDOW | (RbShowInAltTab ? WS_EX_APPWINDOW : 0), RUNBOX_CLASS, "p5 Runbox", 
			WS_POPUP | WS_OVERLAPPEDWINDOW, 500, 500, 300, 40, NULL, NULL, instance, 0);
	}
	else
	{
		//attempt to place it in the center of the screen.
		st = GetSystemMetrics (SM_CXSCREEN);
		if (st)
		{
			st = st / 2 - 150;
		}
		else
		{
			st = 500;
		}

		run->hwnd = CreateWindowEx (WS_EX_TOOLWINDOW | (RbShowInAltTab ? WS_EX_APPWINDOW : 0), RUNBOX_CLASS, "p5 Runbox", 
			WS_POPUP, st, 0, 300, 40, NULL, NULL, instance, 0);
	}
	fail_if (!run->hwnd, 0, PE ("CreateWindowEx failed, %i", GetLastError (), 0, 0, 0));

	if (RbAlwaysOnTop)
	{
		st = SetWindowPos (run->hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		fail_if (!st, 0, PE ("SetWindowPos failed, %i", GetLastError (), 0, 0, 0));
	}

	run->font = CreateFont (RbLineHeight, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Courier New");
	fail_if (!run->font, 0, PE ("CreateFont failed, %i", GetLastError (), 0, 0, 0));

	run->edit = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | WS_BORDER, 0, 0, 0, 0, run->hwnd, (HMENU)1, instance, NULL);
	fail_if (!run->edit, 0, PE ("CreateWindow failed for the edit box, %i", GetLastError (), 0, 0, 0));

	run->report = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 0, 0, 0, 0, run->hwnd, (HMENU)2, instance, NULL);
	fail_if (!run->edit, 0, PE ("CreateWindow failed for the results box, %i", GetLastError (), 0, 0, 0));

	//honestly, sometimes it's ok to ignore failures!  why am I doing this??
	SetLastError (0); //stupid setwindowlong
	temp = SetWindowLong (run->edit, GWL_WNDPROC, (LONG)RbEditSubclassProc);
	fail_if (!temp && GetLastError () != 0, 0, PE ("Could not subclass edit box, %i", GetLastError (), 0, 0, 0));
	SetLastError (0);
	st = SetWindowLong (run->edit, GWL_USERDATA, temp);
	fail_if (!st && GetLastError () != 0, 0, PE ("Could not subclass edit box, %i", GetLastError (), 0, 0, 0));
	
	run->border = 2; //trust me... 1 doesn't work, even though the border WS_BORDER puts on is only 1 pixel.

	//"this message does not return a value", sWEET!
	SendMessage (run->edit, WM_SETFONT, (WPARAM)run->font, MAKELPARAM (0, 0));

	//st = PlCreateParser (&run->par);
	//fail_if (!DERR_OK (st), st, PE ("Could not create a parser, {%s,%i}", PlGetDerrString (st), GETDERRINFO (st), 0, 0));

	//we have to use LoadImage.  LoadIcon returns a handle to an icon that CANNOT BE DESTROYED.  At least not
	//by destroy icon.  It may be destroyed when the dll is unloaded, but that's still unacceptable.
	run->icons[0] = LoadImage (instance, MAKEINTRESOURCE (IDI_RUNBOX), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	fail_if (!run->icons[0], 0, PE ("LoadIcon failed for runbox, %i", GetLastError(), 0, 0, 0));
	run->icons[1] = LoadImage (instance, MAKEINTRESOURCE (IDI_GREENLIGHT), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	fail_if (!run->icons[0], 0, PE ("LoadIcon failed for green light, %i", GetLastError(), 0, 0, 0));
	run->icons[2] = LoadImage (instance, MAKEINTRESOURCE (IDI_REDLIGHT), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	fail_if (!run->icons[0], 0, PE ("LoadIcon failed for red light, %i", GetLastError(), 0, 0, 0));
	run->icons[3] = LoadImage (instance, MAKEINTRESOURCE (IDI_OFFLIGHT), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	fail_if (!run->icons[0], 0, PE ("LoadIcon failed for off light, %i", GetLastError(), 0, 0, 0));

	//don't care if these fail.
	SetActiveWindow (run->hwnd);
	SetFocus (run->edit);

	run->multiline = RbMultiline;
	
	run->active = 1;
	RbResize (run);

	return run;

failure:

	if (run)
	{

		if (run->edit)
			DestroyWindow (run->edit);

		if (run->report)
			DestroyWindow (run->report);

		if (run->font)
			DeleteObject (run->font);

		if (run->hwnd)
			DestroyWindow (run->hwnd);

		if (run->icons[0])
			DestroyIcon (run->icons[0]);
		if (run->icons[1])
			DestroyIcon (run->icons[1]);
		if (run->icons[2])
			DestroyIcon (run->icons[2]);
		if (run->icons[3])
			DestroyIcon (run->icons[3]);

		PlFree (run);
	}

	return NULL;
}

void RbDestroy (runbox_t *run)
{

	//if (run->par)
	//	PlDestroyParser (run->par);
	run->closing = 1;

	if (run->edit)
		DestroyWindow (run->edit);

	if (run->report)
		DestroyWindow (run->report);

	if (run->font)
		DeleteObject (run->font);

	if (run->hwnd)
		DestroyWindow (run->hwnd);

	if (run->icons[0])
		DestroyIcon (run->icons[0]);
	if (run->icons[1])
		DestroyIcon (run->icons[1]);
	if (run->icons[2])
		DestroyIcon (run->icons[2]);
	if (run->icons[3])
		DestroyIcon (run->icons[3]);


	PlFree (run);
}

//called by denit routines
void RbDestroyAll (void)
{
	//MULTIPLE runbox issue:
	if (RbData)
	{
		RbDestroy (RbData);
		RbData = NULL;
	}
}

int RbShow (parerror_t *pe)
{
	DERR st;

	fail_if (!PlIsMainThread (), 0, PE ("rb.show must run in the main thread", 0, 0, 0, 0));
	fail_if (RbData, 0, PE ("Runbox is already open", 0, 0, 0, 0));

	RbData = RbCreate (pe);
	fail_if (!RbData, 0, 0);

	ShowWindow (RbData->hwnd, SW_SHOW);

	return 1;

failure:
	return 0;
}

int RbHide (parerror_t *pe)
{
	DERR st;

	fail_if (!PlIsMainThread (), 0, PE ("rb.hide must run in the main thread", 0, 0, 0, 0));
	fail_if (!RbData, 0, PE ("Runbox is not open", 0, 0, 0, 0));

	RbDestroy (RbData);
	RbData = NULL;

	return 1;
failure:
	return 0;
}

int RbToggle (parerror_t *pe)
{
	if (RbData)
		return RbHide (pe);
	else
		return RbShow (pe);
}

int RbSetHardLimit (int count, int newcount)
{
	int old;

	if (count == 1) //function only got 1 parameter, don't access newcount
		return RbHardLimit;

	old = RbHardLimit;
	RbHardLimit = newcount;

	return old;
}

int RbSetLineHeight (int count, int newcount)
{
	int old;

	if (count == 1) //function only got 1 parameter, don't access newcount
		return RbLineHeight;

	old = RbLineHeight;
	RbLineHeight = newcount;

	return old;
}


void RbAddHistoryLine (parerror_t *pe, char *line)
{
	DERR st;

	fail_if (!line, 0, PE_BAD_PARAM (1));
	fail_if (!PlIsMainThread (), 0, PE ("rb.addhistory must run in the main thread", 0, 0, 0, 0));

	st = RbAddToHistory (line);
	fail_if (!st, 0, PE ("Unable to add line to history", 0, 0, 0, 0));

failure:
	return;
}


void RbScriptClearHistory (parerror_t *pe)
{
	DERR st;

	//the reason it must run in the main thread is to avoid the need for synchronization of the
	//RbHistory list.  (i.e. "because I'm lazy")
	fail_if (!PlIsMainThread (), 0, PE ("rb.clearhistory must run in the main thread", 0, 0, 0, 0));
	RbClearHistory ();

	//MULTIPLE runbox issue:
	if (RbData)
		RbData->hist = NULL;

failure:
	return;
}


int RbSetOnTop (parerror_t *pe, int count, int newval)
{
	DERR st;

	if (count == 2) //newval not present
		return RbAlwaysOnTop;

	//if it's open, and we're the main thread, make the change now.
	if (PlIsMainThread () && RbData && newval != RbAlwaysOnTop)
	{
		st = SetWindowPos (RbData->hwnd, newval ? HWND_TOPMOST : HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		fail_if (!st, 0, PE ("SetWindowPos failed, %i", GetLastError (), 0, 0, 0));
	}

	st = RbAlwaysOnTop;
	RbAlwaysOnTop = newval;
	return st;
failure:
	return 0;
}

int RbSetAltTab (int count, int newval)
{
	int old;

	if (count == 1) //function only got 1 parameter, don't access newcount
		return RbShowInAltTab;

	old = RbShowInAltTab;
	RbShowInAltTab = newval;

	return old;
}

int RbSetFloating (int count, int newval)
{
	int old;

	if (count == 1) //function only got 1 parameter, don't access newcount
		return (RbStyle == RBSTYLE_FLOATING ? 1 : 0);

	old = (RbStyle == RBSTYLE_FLOATING ? 1 : 0);
	RbStyle = (newval == 1 ? RBSTYLE_FLOATING : RBSTYLE_TOP);

	return old;
}

void RbActivate (parerror_t *pe)
{
	DERR st;
	HMODULE hmod;
	FARPROC foo;
	HWND target;

	hmod = GetModuleHandle ("user32.dll");
	fail_if (!hmod, 0, PE ("Could not get user32.dll module handle, %i", GetLastError (), 0, 0, 0));

	foo = GetProcAddress (hmod, "SwitchToThisWindow");
	fail_if (!foo, 0, PE ("Could not get proc address, %i", GetLastError (), 0, 0, 0));

	target = FindWindow (RUNBOX_CLASS, NULL);
	fail_if (!target, 0, PE ("Runbox not found: %i", GetLastError (), 0, 0, 0));

	/*
	target = GetDlgItem (target, 1);
	fail_if (!target, 0, PE ("Runbox has no edit box: %i", GetLastError (), 0, 0, 0));
	*/

	((void (WINAPI *)(HWND, int))foo) (target, 1);

	PostMessage (target, WM_SELECTALL, 0, 0);

	//SendMessage (target, EM_SETSEL, 0, -1);

failure:
	return;
}

int RbIsOpen (void)
{
	if (RbData)
		return 1;
	return 0;
}
	
int RbSetAllowClose (int count, int newval)
{
	int old;

	if (count == 1) //function only got 1 parameter, don't access newcount
		return RbAllowClose;

	old = RbAllowClose;
	RbAllowClose = newval;

	return old;
}

int RbSetMaxHistory (int count, int newval)
{
	int old;

	if (count == 1) //function only got 1 parameter, don't access newcount
		return RbMaxHistory;

	old = RbMaxHistory;
	RbMaxHistory = newval;

	return old;
}

int RbSetMultiline (int count, int newval)
{
	int old;

	if (count == 1) //function only got 1 parameter, don't access newcount
		return RbMultiline;

	old = RbMultiline;
	RbMultiline = newval;

	return old;
}