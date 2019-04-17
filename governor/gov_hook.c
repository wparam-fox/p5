/* Copyright 2008 wParam, licensed under the GPL */

#include <windows.h>
#include "govern.h"



void GhSendCommand1 (HWND param1, char *command)
{
	DERR st;
	HANDLE slot = NULL;
	char name[MAX_PATH];
	char buffer[GOV_MAX_COMMAND_LENGTH];
	DWORD read;

	st = GovGetName (name, MAX_PATH, "mailslot");
	fail_if (!st, 0, DP (ERROR, "GhSendCommand: Could not get slot name (%i)\n", GetLastError ()));

	slot = CreateFile (name, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	fail_if (slot == INVALID_HANDLE_VALUE, 0, DP (ERROR, "GhSendCommand: Could not open slot (%i)\n", GetLastError ()));

	strncpy (buffer + sizeof (HWND), command, GOV_MAX_COMMAND_LENGTH - 1 - sizeof (HWND));
	buffer[GOV_MAX_COMMAND_LENGTH - 1] = '\0';
	*((HWND *)buffer) = param1;

	st = WriteFile (slot, buffer, GOV_MAX_COMMAND_LENGTH, &read, NULL);
	fail_if (!st, 0, DP (ERROR, "GhSendCommand: Could not write to mailslot, %i\n", GetLastError ()));
	fail_if (read != GOV_MAX_COMMAND_LENGTH, 0, DP (ERROR, "GhSendCommand: Did not write all data to slot\n"));

	CloseHandle (slot);

	return;
failure:
	if (slot)
		CloseHandle (slot);

}

void GhSendCommand (char *command)
{
	GhSendCommand1 (NULL, command);
}


int GhFuncStdGoTime (sectionobj_t *o, int *retval, WPARAM wParam, LPARAM lParam)
{
	GhSendCommand1 ((HWND)wParam, o->strings + strlen (o->strings) + 1);
	return 0;
}

int GhFuncStdDenyAction (sectionobj_t *o, int *retval, WPARAM wParam, LPARAM lParam)
{
	*retval = 1;
	return 1;
}

int GhFuncActivateOne (sectionobj_t *o, int *retval, WPARAM wParam, LPARAM lParam)
{
	DERR st;
	if (GetProp ((HWND)wParam, "p5-GovernDone"))
		return 0;

	st = SetProp ((HWND)wParam, "p5-GovernDone", (HANDLE)1);
	if (!st)
		return -1;

	GhSendCommand1 ((HWND)wParam, o->strings + strlen (o->strings) + 1);
	return 0;
}

int GhFuncInsertAfter (sectionobj_t *o, int *retval, WPARAM wParam, LPARAM lParam)
{
	HWND foo;

	foo = (HWND)atoi (o->strings + strlen (o->strings) + 1);
	GovDprintf (NOISE, "Zorder: %i = %i\n", ((CBT_CREATEWND *)lParam)->hwndInsertAfter, foo);
	((CBT_CREATEWND *)lParam)->lpcs->x = (int)foo;

	return 0;
}


#define KEY_NONE 0
#define KEY_CLASS 1
#define KEY_TITLE 2
#define KEY_BLANK 3
typedef struct
{
	int code;
	int keytype;
	char *type;

	/* Return 1 to block the message, -1 on error.  You only need
	 * to set *retval if you return 1.*/
	int (*func)(sectionobj_t *, int *retval, WPARAM, LPARAM);
} actiontype_t;

static actiontype_t actions[] = 
{
	{HCBT_ACTIVATE,		KEY_CLASS, ACTIVATE RUN CLASS,		GhFuncStdGoTime},
	{HCBT_ACTIVATE,		KEY_CLASS, ACTIVATE_ONE RUN CLASS,	GhFuncActivateOne},
	{HCBT_CREATEWND,	KEY_CLASS, CREATE RUN CLASS,		GhFuncStdGoTime},
	{HCBT_DESTROYWND,	KEY_CLASS, DESTROY RUN CLASS,		GhFuncStdGoTime},
	{HCBT_MINMAX,		KEY_CLASS, MINMAX RUN CLASS,		GhFuncStdGoTime},
	{HCBT_MOVESIZE,		KEY_CLASS, MOVESIZE RUN CLASS,		GhFuncStdGoTime},
	{HCBT_SETFOCUS,		KEY_CLASS, SETFOCUS RUN CLASS,		GhFuncStdGoTime},

	{HCBT_ACTIVATE,		KEY_CLASS, ACTIVATE DENY CLASS,		GhFuncStdDenyAction},
	{HCBT_CREATEWND,	KEY_CLASS, CREATE DENY CLASS,		GhFuncStdDenyAction},
	{HCBT_DESTROYWND,	KEY_CLASS, DESTROY DENY CLASS,		GhFuncStdDenyAction},
	{HCBT_MINMAX,		KEY_CLASS, MINMAX DENY CLASS,		GhFuncStdDenyAction},
	{HCBT_MOVESIZE,		KEY_CLASS, MOVESIZE DENY CLASS,		GhFuncStdDenyAction},
	{HCBT_SETFOCUS,		KEY_CLASS, SETFOCUS DENY CLASS,		GhFuncStdDenyAction},

	//{HCBT_CREATEWND,	KEY_CLASS, CREATE ZORDER CLASS,		GhFuncInsertAfter},

	{HCBT_ACTIVATE,		KEY_TITLE, ACTIVATE RUN TITLE,		GhFuncStdGoTime},
	{HCBT_ACTIVATE,		KEY_TITLE, ACTIVATE_ONE RUN TITLE,	GhFuncActivateOne},
	{HCBT_CREATEWND,	KEY_TITLE, CREATE RUN TITLE,		GhFuncStdGoTime},
	{HCBT_DESTROYWND,	KEY_TITLE, DESTROY RUN TITLE,		GhFuncStdGoTime},
	{HCBT_MINMAX,		KEY_TITLE, MINMAX RUN TITLE,		GhFuncStdGoTime},
	{HCBT_MOVESIZE,		KEY_TITLE, MOVESIZE RUN TITLE,		GhFuncStdGoTime},
	{HCBT_SETFOCUS,		KEY_TITLE, SETFOCUS RUN TITLE,		GhFuncStdGoTime},

	{HCBT_ACTIVATE,		KEY_TITLE, ACTIVATE DENY TITLE,		GhFuncStdDenyAction},
	{HCBT_CREATEWND,	KEY_TITLE, CREATE DENY TITLE,		GhFuncStdDenyAction},
	{HCBT_DESTROYWND,	KEY_TITLE, DESTROY DENY TITLE,		GhFuncStdDenyAction},
	{HCBT_MINMAX,		KEY_TITLE, MINMAX DENY TITLE,		GhFuncStdDenyAction},
	{HCBT_MOVESIZE,		KEY_TITLE, MOVESIZE DENY TITLE,		GhFuncStdDenyAction},
	{HCBT_SETFOCUS,		KEY_TITLE, SETFOCUS DENY TITLE,		GhFuncStdDenyAction},

	//{HCBT_CREATEWND,	KEY_CLASS, CREATE ZORDER TITLE,		GhFuncInsertAfter},

	{HCBT_ACTIVATE,		KEY_BLANK, ACTIVATE RUN ALL,		GhFuncStdGoTime},
	{HCBT_CREATEWND,	KEY_BLANK, CREATE RUN ALL,			GhFuncStdGoTime},
	{HCBT_DESTROYWND,	KEY_BLANK, DESTROY RUN ALL,			GhFuncStdGoTime},
	{HCBT_MINMAX,		KEY_BLANK, MINMAX RUN ALL,			GhFuncStdGoTime},
	{HCBT_MOVESIZE,		KEY_BLANK, MOVESIZE RUN ALL,		GhFuncStdGoTime},
	{HCBT_SETFOCUS,		KEY_BLANK, SETFOCUS RUN ALL,		GhFuncStdGoTime},

	{HCBT_ACTIVATE,		KEY_BLANK, ACTIVATE DENY ALL,		GhFuncStdDenyAction},
	{HCBT_CREATEWND,	KEY_BLANK, CREATE DENY ALL,			GhFuncStdDenyAction},
	{HCBT_DESTROYWND,	KEY_BLANK, DESTROY DENY ALL,		GhFuncStdDenyAction},
	{HCBT_MINMAX,		KEY_BLANK, MINMAX DENY ALL,			GhFuncStdDenyAction},
	{HCBT_MOVESIZE,		KEY_BLANK, MOVESIZE DENY ALL,		GhFuncStdDenyAction},
	{HCBT_SETFOCUS,		KEY_BLANK, SETFOCUS DENY ALL,		GhFuncStdDenyAction},

	{-1, 0, NULL, NULL}
};

int GhGetKey (int keytype, int code, WPARAM wParam, LPARAM lParam, char *key, int keylen)
{
	HWND hwnd = NULL;
	DERR st;

	switch (code)
	{
	case HCBT_ACTIVATE:
	case HCBT_CREATEWND:
	case HCBT_DESTROYWND:
	case HCBT_MINMAX:
	case HCBT_MOVESIZE:
	case HCBT_SETFOCUS:
		hwnd = (HWND)wParam;
		break;
	}

	switch (keytype)
	{
	case KEY_CLASS:
		fail_if (!hwnd, 0, DP (NOISE, "Can't get classname of a NULL window\n"));
		st = GetClassName (hwnd, key, keylen - 1);
		fail_if (!st, 0, DP (WARN, "Couldn't get classname of window %X, %i\n", hwnd, GetLastError ()));
		key[keylen - 1] = '\0';
		break;

	case KEY_TITLE:
		if (code == HCBT_CREATEWND)
		{
			fail_if (!((CBT_CREATEWND  *)lParam)->lpcs->lpszName, 0, DP (NOISE, "HCBT_CREATE: NULL title\n"));
			/* Create is special--WM_GETTEXT doesn't go through because the window
			 * hasn't gotten WM_CREATE yet, among other things. */
			strncpy (key, ((CBT_CREATEWND  *)lParam)->lpcs->lpszName, keylen - 1);
			key[keylen - 1] = '\0';
		}
		else
		{
			fail_if (!hwnd, 0, DP (NOISE, "Can't get classname of a NULL window\n"));
			st = SendMessage (hwnd, WM_GETTEXT, keylen - 1, (LPARAM)key);
			fail_if (!st, 0, DP (WARN, "Couldn't WM_GETTEXT window %x, %i\n", hwnd, GetLastError ()));
			key[keylen - 1] = '\0';
		}
		break;

	default: /* just in case some other value gets in here */
	case KEY_BLANK:
		*key = '\0';
		break;
	}

	GovDprintf (NOISE, "GhGetKey: Response: %X == %s\n", hwnd, key);

	return 1;
failure:
	return 0;
}

void GhPrintPotentialHit (int code, WPARAM wParam, LPARAM lParam)
{
	char class[256];
	char title[256];
	char *type;

	class[0] = '\0';
	title[0] = '\0';

	GhGetKey (KEY_TITLE, code, wParam, lParam, title, 256);
	GhGetKey (KEY_CLASS, code, wParam, lParam, class, 256);

	class[255] = '\0';
	title[255] = '\0';

	switch (code)
	{
	case HCBT_ACTIVATE:		type = "ACTIVATE"; break;
	case HCBT_CREATEWND:	type = "CREATE"; break;
	case HCBT_DESTROYWND:	type = "DESTROY"; break;
	case HCBT_MINMAX:		type = "MINMAX"; break;
	case HCBT_MOVESIZE:		type = "MOVESIZE"; break;
	case HCBT_SETFOCUS:		type = "SETFOCUS"; break;
	default:				type = "UNKNOWN"; break;
	}

	/* Always print this; it's controlled by a different var */
	GovDprintf (CRIT, "Potential gov: %s (class:%s) (title:%s)\n", type, class, title);
}

#define KEY_MAXSIZE 256
LRESULT CALLBACK GhHookProc (int code, WPARAM wParam, LPARAM lParam)
{
	void *buffer = NULL;
	int block = 0;
	int retval = 0;
	DERR st;
	sectionobj_t *o;

	if (code < 0)
		return CallNextHookEx (NULL, code, wParam, lParam);

	if (GovIsPlugin ())
		return CallNextHookEx (NULL, code, wParam, lParam);

	if (code != HCBT_ACTIVATE && code != HCBT_MOVESIZE &&
		code != HCBT_CREATEWND && code != HCBT_DESTROYWND &&
		code != HCBT_MINMAX && code != HCBT_SETFOCUS)
		return CallNextHookEx (NULL, code, wParam, lParam);

	if (GovPrintGoverns ())
		GhPrintPotentialHit (code, wParam, lParam);

	st = GbGetBuffer (NULL, &buffer);
	fail_if (!st, 0, DP (NOISE, "Skipping hooks because of error\n"));
	
	__try
	{
		char key[KEY_MAXSIZE];
		int keytype;
		int keyerror;
		int x;

		keytype = KEY_NONE;
		*key = '\0';
		keyerror = 0;
		for (x=0;actions[x].code != -1;x++)
		{
			if (code == actions[x].code)
			{
				if (keytype != actions[x].keytype)
				{
					st = GhGetKey (actions[x].keytype, code, wParam, lParam, key, KEY_MAXSIZE);
					keytype = actions[x].keytype;
					keyerror = !st ? 1 : 0;
				}

				/* If, for example, GetClassName has failed, then continue to skip
				 * all of the class based hooks */
				if (keyerror)
					continue;

				st = GbGetEntry (buffer, actions[x].type, key, &o);
				if (!st)
					continue;
				if (o->flags & OBJFLAG_DISABLED)
					continue;

				st = actions[x].func (o, &retval, wParam, lParam);
				if (st > 0)
					block = 1;
				fail_if (st == -1, 0, DP (WARN, "Function %i told us to fail.\n", x));
			}
		}
				

				


		
		/*st = GetClassName ((HWND)wParam, classname, 255);
		fail_if (!st, 0, DP (WARN, "Could not get class name to test (%i)\n", GetLastError ()));
		classname[255] = '\0';

		if (code == HCBT_ACTIVATE)
		{
			st = GbGetEntry (buffer, "ACTIVATE_CLASS", classname, &o);
			if (st && !(o->flags & OBJFLAG_DISABLED))
			{
				GhSendCommand1 ((HWND)wParam, o->strings + strlen (o->strings) + 1);
			}
		}

		if (code == HCBT_MOVESIZE && strcmp (classname, "Notepad") == 0)
		{
			GovDprintf (NOISE, "Trying to block up in here\n");
			block = 1;
			retval = 1;
		}*/
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		fail_if (1, 0, DP (ERROR, "Exception 0x%.8X while accessing buffer!\n", GetExceptionCode ()));
	}

	GbReleaseBuffer (buffer);

	if (block)
		return retval;
	return CallNextHookEx (NULL, code, wParam, lParam);

failure: /* fall through */
	if (buffer)
		GbReleaseBuffer (buffer);

	return CallNextHookEx (NULL, code, wParam, lParam);
}
