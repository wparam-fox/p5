/* Copyright 2008 wParam, licensed under the GPL */

#include <windows.h>
#include "govern.h"
#include <stdio.h>

HHOOK GsHook = NULL;

HANDLE GsMailslot = NULL;
HANDLE GsMailslotEvent = NULL;
OVERLAPPED GsMailslotOver = {0};

HANDLE GsThread = NULL;
HANDLE GsStopEvent = NULL;
int GsRunning = 0;
parser_t *GsParser = NULL;

int GsDebugOut = 0;

static DWORD CALLBACK GsListeningThread (void *param)
{
	HANDLE waits[2];
	DERR st;
	char buffer[GOV_MAX_COMMAND_LENGTH];
	char *walk;
	char temp1[30], temp2[30];
	int x;
	DWORD read;

	char error[1024];
	char *output;
	int nesting;

	while (GsRunning)
	{
		GsMailslotOver.hEvent = GsMailslotEvent;
		ResetEvent (GsMailslotEvent);
		st = ReadFile (GsMailslot, buffer, GOV_MAX_COMMAND_LENGTH, &read, &GsMailslotOver);
		fail_if (!st && GetLastError () != ERROR_IO_PENDING, 0, DP (ERROR, "Wait thread aborted due to read error %i\n", GetLastError ()));

		waits[0] = GsMailslotEvent;
		waits[1] = GsStopEvent;

		st = WaitForMultipleObjects (2, waits, FALSE, INFINITE);
		GovDprintf (NOISE, "GsListenThread trigger: %i\n", st);
		fail_if (st == WAIT_FAILED, 0, DP (ERROR, "Wait thread aborted due to wait error %i\n", GetLastError ()));

		if (st == WAIT_OBJECT_0 + 1) /* the stop event */
			return 0;
		if (!GsRunning)
			return 0;

		st = GetOverlappedResult (GsMailslot, &GsMailslotOver, &read, FALSE);
		if (!st)
			continue;

		buffer[GOV_MAX_COMMAND_LENGTH - 1] = '\0';
		walk = buffer;

		/* First, Set the G* variables as appropriate. */
		for (x=0;x<GOV_MAX_COMMAND_PARAMS;x++, walk+=sizeof (int))
		{
			_snprintf (temp1, 29, "G%i", x + 1);
			temp1[29] = '\0';
			_snprintf (temp2, 29, "%i", *((int *)walk));
			temp2[29] = '\0';

			st = PlParserAddAtom (GsParser, temp1, temp2, 0);
			fail_if (!DERR_OK (st), st, DP (ERROR, "Wait thread aborted due to atom set error %X\n", st));
		}

		/* now parse */
		output = NULL;
		*error = '\0';
		nesting = 0;
		st = PlRunParser (GsParser, walk, &output, &nesting, error, 1023);
		error[1023] = '\0';

		if (GsDebugOut)
			PlPrintf ("Governor: Command %s resulted in nesting %i, code %i, output \"%s\" error \"%s\"\n", walk, nesting, st, output, error);

		if (output)
			ParFree (output);

		st = PlResetParser (GsParser);
		fail_if (!DERR_OK (st), st, DP (ERROR, "Wait thread aborted due to parser reset error %X\n", st));
	}

	GovDprintf (NOISE, "Thread exiting, success\n");
	return 0;
failure:
	GovDprintf (NOISE, "Thread exiting, failure\n");
	return 1;
}


static void GsCleanupThreadGlobals (void)
{
	if (GsThread)
	{
		CloseHandle (GsThread);
		GsThread = NULL;
	}

	GsRunning = 0;

	if (GsStopEvent)
	{
		CloseHandle (GsStopEvent);
		GsStopEvent = NULL;
	}

	if (GsParser)
	{
		PlDestroyParser (GsParser);
		GsParser = NULL;
	}

	if (GsMailslotEvent)
	{
		CloseHandle (GsMailslotEvent);
		GsMailslotEvent = NULL;
	}

	if (GsMailslot)
	{
		CloseHandle (GsMailslot);
		GsMailslot = NULL;
	}

	/* Nothing gets allocated in here, but clear it out for good measure */
	memset (&GsMailslotOver, 0, sizeof (OVERLAPPED));
}	

static int GsStartGovernorThread (parerror_t *pe)
{
	DERR st;
	char name[MAX_PATH];
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;

	fail_if (GsThread, 0, PE ("Governor thread already running", 0, 0, 0, 0));

	st = GovGetName (name, MAX_PATH, "mailslot");
	fail_if (!st, 0, PE ("GovGetName for mailslot failed, %i\n", GetLastError (), 0, 0, 0));

	st = InitializeSecurityDescriptor (&sd, SECURITY_DESCRIPTOR_REVISION);
	fail_if (!st, 0, PE ("Couldn't init security descriptor", GetLastError (), 0, 0, 0));
	st = SetSecurityDescriptorDacl (&sd, TRUE, NULL, FALSE);
	fail_if (!st, 0, PE ("Couldn't set wide-open ACL", GetLastError (), 0, 0, 0));
	st = IsValidSecurityDescriptor (&sd);
	fail_if (!st, 0, PE ("SD seems to not be valid", 0, 0, 0, 0));

	sa.nLength = sizeof (SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = &sd;

	GsMailslot = CreateMailslot (name, GOV_MAX_COMMAND_LENGTH, MAILSLOT_WAIT_FOREVER, &sa);
	fail_if (GsMailslot == INVALID_HANDLE_VALUE, 0, PE ("Could not create mailslot, %i", GetLastError (), 0, 0, 0));

	GsMailslotEvent = CreateEvent (NULL, TRUE, TRUE, NULL);
	fail_if (!GsMailslotEvent, 0, PE ("Could not create mailslot read event, %i", GetLastError (), 0, 0, 0));

	st = PlCreateParser (&GsParser);
	fail_if (!DERR_OK (st), st, PE ("Could not create parser: %X", st, 0, 0, 0));

	GsStopEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
	fail_if (!GsStopEvent, 0, PE ("Could not create stop event, %i", GetLastError (), 0, 0, 0));

	GsRunning = 1;

	GsThread = CreateThread (NULL, 0, GsListeningThread, NULL, 0, NULL);
	fail_if (!GsThread, 0, PE ("Could not create listen thread, %i", GetLastError (), 0, 0, 0));

	/* Do not fail below here.  GsCleanupThreadGlobals doesn't kill the thread. */
	return 1;


failure:
	GsCleanupThreadGlobals ();
	return 0;
}


static int GsStopGovernorThread (parerror_t *pe)
{
	DERR st;

	fail_if (!GsThread, 0, PE ("Governor thread doesn't seem to be running", 0, 0, 0, 0));

	/* Attempt a graceful stop of the thread.  Don't block too long. */
	GsRunning = 0;

	SetEvent (GsStopEvent);
	/* ignore it if it failed--we should timeout below. */

	st = WaitForSingleObject (GsThread, 750);
	fail_if (st == WAIT_FAILED, 0, PE ("Couldn't stop the thread--wait failed (%i)", GetLastError (), 0, 0, 0));
	fail_if (st == WAIT_TIMEOUT, 0, PE ("The listen thread refused to stop", 0, 0, 0, 0));

	/* Sweet, thread exited, all is good. */
	GsCleanupThreadGlobals ();

	return 1;
failure:
	/* Leave GsRunning 0 and the stop event set. */
	return 0;
}

static int GsKillGovernorThread (void)
{
	if (!GsThread)
		return 1;

	GsRunning = 0;
	SetEvent (GsStopEvent);

	/* Quick context switch just in case it can exit. */
	Sleep (50);

	TerminateThread (GsThread, 555);

	GsCleanupThreadGlobals ();

	return 1;
}


static int GsStartGovernor (int exclusive, int forceslot, parerror_t *pe)
{
	DERR st;
	int handlesmade = 0;

	st = GbCreateHandles (pe, exclusive);
	fail_if (!st, 0, 0);
	handlesmade = 1;

	GsHook = SetWindowsHookEx (WH_CBT, GhHookProc, instance, 0);
	fail_if (!GsHook, 0, PE ("Set hook failed (%i)", GetLastError (), 0, 0, 0));

	if (exclusive || forceslot)
	{
		st = GsStartGovernorThread (pe);
		fail_if (!st, 0, 0);
	}

	/* Don't fail below the thread start */

	return 1;
failure:
	if (GsHook)
	{
		UnhookWindowsHookEx (GsHook);
		GsHook = NULL;
	}

	if (handlesmade)
		GbCloseHandles ();

	return 0;
}

static int GsStopGovernor (int useforce, parerror_t *pe)
{
	DERR st;

	if (GsThread)
	{
		if (useforce)
			st = GsKillGovernorThread ();
		else
			st = GsStopGovernorThread (pe);
		fail_if (!st, 0, 0);
	}

	UnhookWindowsHookEx (GsHook);
	GsHook = NULL;

	GbCloseHandles ();

	return 1;
failure:
	return 0;
}

int GsShutdownGovernor (int useforce, parerror_t *pe)
{
	if (!GsHook) /* Not running */
		return 1;

	return GsStopGovernor (useforce, pe);
}

int GsGovernorRunning (void)
{
	return GsHook ? 1 : 0;
}

void GsFlushGovernors (parerror_t *pe)
{
	void *buffer = NULL;
	DERR st;

	st = GbGetBuffer (pe, &buffer);
	fail_if (!st, 0, 0);

	__try
	{
		GbClearBuffer (buffer);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		fail_if (TRUE, 0, PE ("Exception 0x%.8X while formatting buffer", GetExceptionCode (), 0, 0, 0));
	}

	GbReleaseBuffer (buffer);

	return;

failure:
	if (buffer)
		GbReleaseBuffer (buffer);
}



static int GsAddGovernor (parerror_t *pe, char *type, char *key, char *parse)
{
	sectionobj_t *o;
	void *buffer = NULL;
	DERR st;

	st = GbGetBuffer (pe, &buffer);
	fail_if (!st, 0, 0);

	__try
	{
		/* See if there's a previously deleted element that we can just
		 * re-enable.  Saves on memory waste; I expect if this really
		 * gets used there will be some things that toggle off/on a lot*/
		st = GbGetEntry (buffer, type, key, &o);

		if (st)
		{
			/* check to see if it has the same parse string */
			if (strcmp (parse, o->strings + strlen (o->strings) + 1) == 0)
			{
				/* Cool, re-enable it, in case it was disabled, and we're done. */
				fail_if (strlen (key) != strlen (o->strings), 0, PE ("Sanity failure on key length", 0, 0, 0, 0));
				o->flags &= ~OBJFLAG_DISABLED;
			}
			else
			{
				/* Can't use it.  Set st to 0, so that we'll trigger below */
				st = 0;
			}
		}
		if (!st) /* sorry for the odd flow, but no, this cannot be replaced with "else"*/
		{
			st = GbSetEntry (buffer, type, key, parse, 0);
			fail_if (!st, 0, PE ("Could not set entry; out of governor space?", 0, 0, 0, 0));
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		fail_if (1, 0, PE ("Exception 0x%.8X while accessing buffer", GetExceptionCode (), 0, 0, 0));
	}

	GbReleaseBuffer (buffer);

	return 1;
failure:
	if (buffer)
		GbReleaseBuffer (buffer);

	return 0;
}

char *GsGetGovernorValue (parerror_t *pe, char *type, char *key)
{
	void *buffer = NULL;
	DERR st;
	sectionobj_t *o;
	char *out = NULL;
	int len;
	char *temp;

	st = GbGetBuffer (pe, &buffer);
	fail_if (!st, 0, 0);

	__try
	{
		st = GbGetEntry (buffer, type, key, &o);
		fail_if (!st, 0, PE ("Object not found", 0, 0, 0, 0));

		temp = o->strings + strlen (o->strings) + 1;

		len = strlen (temp) + 1;
		out = ParMalloc (len);
		fail_if (!out, 0, PE_OUT_OF_MEMORY (len));

		strcpy (out, temp);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		fail_if (1, 0, PE ("Exception 0x%.8X while reading", GetExceptionCode (), 0, 0, 0));
	}

	GbReleaseBuffer (buffer);

	return out;
failure:
	if (out)
		ParFree (out);

	if (buffer)
		GbReleaseBuffer (buffer);
	return NULL;
}
	
int GsRemoveGovernor (parerror_t *pe, char *type, char *key)
{
	void *buffer = NULL;
	sectionobj_t *o;
	DERR st;

	st = GbGetBuffer (pe, &buffer);
	fail_if (!st, 0, 0);

	__try
	{
		st = GbGetEntry (buffer, type, key, &o);
		fail_if (!st, 0, PE ("Governor not found", 0, 0, 0, 0));

		o->flags |= OBJFLAG_DISABLED;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		fail_if (1, 0, PE ("Exception 0x%.8X while accessing buffer", GetExceptionCode (), 0, 0, 0));
	}

	GbReleaseBuffer (buffer);

	return 1;
failure:
	if (buffer)
		GbReleaseBuffer (buffer);

	return 0;
}

/******************** script functions *********************/

void GsScriptStartGovernor (parerror_t *pe)
{
	DERR st;
	fail_if (GsHook, 0, PE ("Governor is already running", 0, 0, 0, 0));
	GsStartGovernor (1, 0, pe);
failure:
	return;
}

void GsScriptStartShared (parerror_t *pe)
{
	DERR st;
	fail_if (GsHook, 0, PE ("Governor is already running", 0, 0, 0, 0));
	GsStartGovernor (0, 0, pe);
failure:
	return;
}

void GsScriptContinueGovernor (parerror_t *pe)
{
	DERR st;
	fail_if (GsHook, 0, PE ("Governor is already running", 0, 0, 0, 0));
	GsStartGovernor (0, 1, pe);
failure:
	return;
}

void GsScriptStopGovernor (parerror_t *pe)
{
	DERR st;
	fail_if (!GsHook, 0, PE ("Governor is not running", 0, 0, 0, 0));
	GsStopGovernor (0, pe);
failure:
	return;
}

void GsScriptKillGovernor (parerror_t *pe)
{
	DERR st;
	fail_if (!GsHook, 0, PE ("Governor is not running", 0, 0, 0, 0));
	GsStopGovernor (1, pe);
failure:
	return;
}

int GsScriptGetPrint (void)
{
	return GsDebugOut;
}

void GsScriptSetPrint (int val)
{
	GsDebugOut = val;
}

void GsScriptThreadStatus (parerror_t *pe)
{
	DERR st;
	DWORD code;
	fail_if (!GsHook, 0, PE ("Governor is not running", 0, 0, 0, 0));
	fail_if (!GsThread, 0, PE ("Governor thread was not started", 0, 0, 0, 0));
	st = GetExitCodeThread (GsThread, &code);
	fail_if (!st, 0, PE ("Could not query governor thread status, %i", GetLastError (), 0, 0, 0));
	fail_if (code != STILL_ACTIVE, 0, PE ("Governor thread appears to have terminated, status %i", code, 0, 0, 0));
failure:
	return;
}

void GsScriptDumpBuffer (parerror_t *pe)
{
	void *buffer = NULL;
	DERR st;

	st = GbGetBuffer (pe, &buffer);
	fail_if (!st, 0, 0);

	__try
	{
		GbDumpBuffer (buffer);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		fail_if (TRUE, 0, PE ("Exception 0x%.8X in dumping buffer\n", GetExceptionCode (), 0, 0, 0));
	}

	GbReleaseBuffer (buffer);

	return;
failure:
	if (buffer)
		GbReleaseBuffer (buffer);

	return;
}


#define STD_FUNCS(nicename, typestring) \
void GsScriptAdd##nicename (parerror_t *pe, int count, char *key, char *action)\
{\
	DERR st;\
	if (count < 3) key = "";\
	if (count < 4) action = "";\
	fail_if (!key, 0, PE_BAD_PARAM (1));\
	fail_if (!action, 0, PE_BAD_PARAM (2));\
	st = GsAddGovernor (pe, typestring, key, action);\
	fail_if (!st, 0, 0);\
failure:\
	return;\
}\
\
char *GsScriptGet##nicename (parerror_t *pe, int count, char *key)\
{\
	DERR st;\
	if (count < 3) key = "";\
	fail_if (!key, 0, PE_BAD_PARAM (1));\
	return GsGetGovernorValue (pe, typestring, key);\
failure:\
	return NULL;\
}\
\
void GsScriptDelete##nicename (parerror_t *pe, int count, char *key)\
{\
	DERR st;\
	if (count < 3) key = "";\
	fail_if (!key, 0, PE_BAD_PARAM (1));\
	GsRemoveGovernor (pe, typestring, key);\
failure:\
	return;\
}



STD_FUNCS (ActivateClass,		ACTIVATE RUN CLASS)
STD_FUNCS (ActivateTitle,		ACTIVATE RUN TITLE)
STD_FUNCS (ActivateAll,			ACTIVATE RUN ALL)

STD_FUNCS (ActivateOneClass,	ACTIVATE_ONE RUN CLASS)
STD_FUNCS (ActivateOneTitle,	ACTIVATE_ONE RUN TITLE)

STD_FUNCS (CreateClass,			CREATE RUN CLASS);
STD_FUNCS (CreateTitle,			CREATE RUN TITLE);
STD_FUNCS (CreateAll,			CREATE RUN ALL);

STD_FUNCS (DestroyClass,		DESTROY RUN CLASS);
STD_FUNCS (DestroyTitle,		DESTROY RUN TITLE);
STD_FUNCS (DestroyAll,			DESTROY RUN ALL);

STD_FUNCS (MoveSizeClass,		MOVESIZE RUN CLASS);
STD_FUNCS (MoveSizeTitle,		MOVESIZE RUN TITLE);
STD_FUNCS (MoveSizeAll,			MOVESIZE RUN ALL);

STD_FUNCS (MinMaxClass,			MINMAX RUN CLASS);
STD_FUNCS (MinMaxTitle,			MINMAX RUN TITLE);
STD_FUNCS (MinMaxAll,			MINMAX RUN ALL);

STD_FUNCS (SetFocusClass,		SETFOCUS RUN CLASS);
STD_FUNCS (SetFocusTitle,		SETFOCUS RUN TITLE);
STD_FUNCS (SetFocusAll,			SETFOCUS RUN ALL);


STD_FUNCS (ActivateClassDeny,	ACTIVATE DENY CLASS)
STD_FUNCS (ActivateTitleDeny,	ACTIVATE DENY TITLE)
STD_FUNCS (ActivateAllDeny,		ACTIVATE DENY ALL)

STD_FUNCS (CreateClassDeny,		CREATE DENY CLASS);
STD_FUNCS (CreateTitleDeny,		CREATE DENY TITLE);
STD_FUNCS (CreateAllDeny,		CREATE DENY ALL);

STD_FUNCS (DestroyClassDeny,	DESTROY DENY CLASS);
STD_FUNCS (DestroyTitleDeny,	DESTROY DENY TITLE);
STD_FUNCS (DestroyAllDeny,		DESTROY DENY ALL);

STD_FUNCS (MoveSizeClassDeny,	MOVESIZE DENY CLASS);
STD_FUNCS (MoveSizeTitleDeny,	MOVESIZE DENY TITLE);
STD_FUNCS (MoveSizeAllDeny,		MOVESIZE DENY ALL);

STD_FUNCS (MinMaxClassDeny,		MINMAX DENY CLASS);
STD_FUNCS (MinMaxTitleDeny,		MINMAX DENY TITLE);
STD_FUNCS (MinMaxAllDeny,		MINMAX DENY ALL);

STD_FUNCS (SetFocusClassDeny,	SETFOCUS DENY CLASS);
STD_FUNCS (SetFocusTitleDeny,	SETFOCUS DENY TITLE);
STD_FUNCS (SetFocusAllDeny,		SETFOCUS DENY ALL);


STD_FUNCS (ZorderClass,			CREATE ZORDER CLASS);
STD_FUNCS (ZorderTitle,			CREATE ZORDER TITLE);
