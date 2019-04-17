/* Copyright 2008 wParam, licensed under the GPL */

#include <windows.h>
#include "p5.h"
#include <stdio.h>
#include "resource.h"

#include "..\common\wp_parse.h"
#include "..\common\wp_mainwin.h"

#include "..\common\p5lib.h"

//#define PF_SUCCESS				DERRCODE (PF_DERR_BASE, 0x00)
#define PF_OUT_OF_MEMORY			DERRCODE (PF_DERR_BASE, 0x01)
#define PF_INVALID_COMMAND_LINE		DERRCODE (PF_DERR_BASE, 0x02)
#define PF_MAINWINDOW_FAILURE		DERRCODE (PF_DERR_BASE, 0x03)
#define PF_CREATE_PIPE_FAILURE		DERRCODE (PF_DERR_BASE, 0x04)
#define PF_CREATE_EVENT_FAILURE		DERRCODE (PF_DERR_BASE, 0x05)
#define PF_READ_FILE_FAILED			DERRCODE (PF_DERR_BASE, 0x06)
#define PF_GET_DESKTOP_NAME_ERROR	DERRCODE (PF_DERR_BASE, 0x07)
#define PF_WHAT_ARE_YOU_THINKING	DERRCODE (PF_DERR_BASE, 0x08)
#define PF_BUFFER_TOO_SMALL			DERRCODE (PF_DERR_BASE, 0x09)
#define PF_DEFAULT_NAME_ERROR		DERRCODE (PF_DERR_BASE, 0x0A)
#define PF_OBJECT_WAIT_FAILED		DERRCODE (PF_DERR_BASE, 0x0B)
#define PF_WRITE_FILE_FAILED		DERRCODE (PF_DERR_BASE, 0x0C)
#define PF_FILE_NOT_FOUND			DERRCODE (PF_DERR_BASE, 0x0D)
#define PF_GET_OVERLAPPED_FAILED	DERRCODE (PF_DERR_BASE, 0x0E)
#define PF_NOTHING_TO_DO			DERRCODE (PF_DERR_BASE, 0x0F)
#define PF_HEAP_CREATE_FAILED		DERRCODE (PF_DERR_BASE, 0x10)
#define PF_NOT_IN_MAIN_THREAD		DERRCODE (PF_DERR_BASE, 0x11)
#define PF_INVALID_PARAMETER		DERRCODE (PF_DERR_BASE, 0x12)
#define PF_SET_EVENT_FAILED			DERRCODE (PF_DERR_BASE, 0x13)
#define PF_SANITY_FAILURE			DERRCODE (PF_DERR_BASE, 0x14)
#define PF_GET_MODULE_FAILED		DERRCODE (PF_DERR_BASE, 0x15)
#define PF_SET_DIRECTORY_FAILED		DERRCODE (PF_DERR_BASE, 0x16)
#define PF_MESSAGEREGISTER_FAILURE	DERRCODE (PF_DERR_BASE, 0x17)
#define PF_ALREADY_IN_MAIN_THREAD	DERRCODE (PF_DERR_BASE, 0x18)




char *PfErrors[] = 
{
	"PF_SUCCESS",
	"PF_OUT_OF_MEMORY",
	"PF_INVALID_COMMAND_LINE",
	"PF_MAINWINDOW_FAILURE",
	"PF_CREATE_PIPE_FAILURE",
	"PF_CREATE_EVENT_FAILURE",
	"PF_READ_FILE_FAILED",
	"PF_GET_DESKTOP_NAME_ERROR",
	"PF_WHAT_ARE_YOU_THINKING",
	"PF_BUFFER_TOO_SMALL",
	"PF_DEFAULT_NAME_ERROR",
	"PF_OBJECT_WAIT_FAILED",
	"PF_WRITE_FILE_FAILED",
	"PF_FILE_NOT_FOUND",
	"PF_GET_OVERLAPPED_FAILED",
	"PF_NOTHING_TO_DO",
	"PF_HEAP_CREATE_FAILED",
	"PF_NOT_IN_MAIN_THREAD",
	"PF_INVALID_PARAMETER",
	"PF_SET_EVENT_FAILED",
	"PF_SANITY_FAILURE",
	"PF_GET_MODULE_FAILED",
	"PF_SET_DIRECTORY_FAILED",
	"PF_MESSAGEREGISTER_FAILURE",
	"PF_ALREADY_IN_MAIN_THREAD",
};

char *PfGetErrorString (int errindex)
{
	int numerrors = sizeof (PfErrors) / sizeof (char *);

	if (errindex >= numerrors || !PfErrors[errindex])
		return "PF_UNKNOWN_ERROR_CODE";

	return PfErrors[errindex];
}





int PfArgc = 0;
char **PfArgv = NULL;
HMAINWND PfMainWindow = NULL;
DWORD PfMainThreadId = 0;
HANDLE PfMainHeap = NULL;
HINSTANCE PfInstance = NULL;

//The main parser.  ALL ACCESS MUST HAPPEN ONLY IN THE MAIN THREAD!  This is how
//we keep it sane.  You MUST use ParRunCurrent when you call it.  (Always assume
//there's already someone on the call stack who is in the main parser.)  This
//means we need to call ParFormatError ourselves; this is an ok price to pay.
parser_t *PfMainParser = NULL;

// The main thread message structure.  This is used by the (mtparse) command and,
//more generally, by the PlParseCommand function.  PlParseCommand runs a command
//in the main parser, which means it needs to happen in the main thread or else
//we risk concurrency issues with the other user of the main thread parser, the
//mailslot (from PfRemote).  A thread wishing to have code run in the main parser
//allocates one of these structures, fills it with values, and posts the message
//to the main thread's queue.  The structure should be allocated in the main heap
//and will be freed by the main thread once it is used.  The line must be
//complete; messages that don't parse out are dropped and an error is returned.
typedef struct mtmessage_t
{
	HANDLE event; //will be set when parsing is done.
				  //must be freed BY THE THREAD THAT CREATED THE mtmessage_t

	int type;

	DERR (*func)(void *param);
	void *param;

	char *line;
	char *output;
	DERR status;
	parerror_t pe;

	LONG control; //see the comment for the field of the same name in bgrun_t; TODO

} mtmessage_t
#define MTMESSAGE_PARSE 1
#define MTMESSAGE_FUNC 2

;
LPARAM PfMessageKey = 0;
UINT PfParseMessage = 0;
UINT PfAbortMessage = 0;


#define PF_BUFFER_SIZE 1024
struct
{
	HANDLE mailslot;
	HANDLE event;
	OVERLAPPED over;
	DWORD read;
	char buffer[PF_BUFFER_SIZE];

	int debugout; //if 1, print info on console

} PfRemote = {0};


HWND PfGetMainWindow (void)
{
	return PfMainWindow;
}

void PfUsageMessage (void)
{
	char buffer[1024];

	buffer[0] = '\0';
	buffer[1023] = (char)0x8E;

	strcat (buffer, "p5 command line parameters:\r\n");
	strcat (buffer, "\r\n");
	strcat (buffer, "--help\r\n");
	strcat (buffer, "    This message\r\n");
	strcat (buffer, "--runconsole pro,in,out,event\r\n");
	strcat (buffer, "    Used to start p5 as a console front.\r\n");
	strcat (buffer, "--debugconsole\r\n");
	strcat (buffer, "    Opens the in-process console at startup.\r\n");
	strcat (buffer, "--stressparser\r\n");
	strcat (buffer, "    Runs a stress test on the parser, looking for memory leaks.\r\n");
	strcat (buffer, "--nomailslot\r\n");
	strcat (buffer, "    Does not attempt to open the remote command mailslot\r\n");
	strcat (buffer, "--mailslotname [name]\r\n");
	strcat (buffer, "    Overrides the default remote control mailslot name.\r\n");
	strcat (buffer, "--parse [string]\r\n");
	strcat (buffer, "    Passes [string] to the remote parse mailslot\r\n");
	strcat (buffer, "    (This must be the last paramater on the command line.)\r\n");
	strcat (buffer, "--directory [string]\r\n");
	strcat (buffer, "    Sets the current directory of p5 to [string]\r\n");
	strcat (buffer, "--startupscript [string]\r\n");
	strcat (buffer, "    Sets the name of the file p5 will run on startup\r\n");
	strcat (buffer, "--shutdownscript [string]\r\n");
	strcat (buffer, "    Sets the name of the file p5 will run on shutdown\r\n");
	strcat (buffer, "--mainwindowtitle [string]\r\n");
    strcat (buffer, "    Sets the title of the p5 main window\r\n");
	

	if (buffer[1023] != (char)0x8E)
	{
		MessageBox (NULL, "Message buffer overflow--fix this when you recompile", "HOLY BEJEZZUS BATMAN!", MB_ICONERROR);
		MessageBox (NULL, buffer, "p5 usage", MB_ICONINFORMATION);
		ExitProcess (0);
	}

	MessageBox (NULL, buffer, "p5 usage", MB_ICONINFORMATION);

}

int PfIsMainThread (void)
{
	return GetCurrentThreadId () == PfMainThreadId;
}

void *PfMalloc (int size)
{
	return HeapAlloc (PfMainHeap, 0, size);
}

void PfFree (void *block)
{
	HeapFree (PfMainHeap, 0, block);
}

void *PfRealloc (void *block, int size)
{
	return HeapReAlloc (PfMainHeap, 0, block, size);
}

DERR PfInit (void)
{
	int st;

	srand (GetTickCount ());
	PfMessageKey = ((rand () & 0xFFFF) << 16) | (rand () & 0xFFFF);
	PfParseMessage = 0x8000 + (rand () % (0xBFF0 - 0x8000));
	PfAbortMessage = PfParseMessage + 1;

	PfMainHeap = HeapCreate (0, 256 * 1024, 0);
	fail_if (!PfMainHeap, PF_HEAP_CREATE_FAILED, DIGLE);

	return PF_SUCCESS;
failure:
	return st;
}

void PfDenit (void)
{

	HeapDestroy (PfMainHeap);
}

int PfHeapCount (void)
{
	PROCESS_HEAP_ENTRY phe = {0};
	int count = 0;
	int busycount = 0;

	while (HeapWalk (PfMainHeap, &phe))
	{
		if (phe.wFlags & PROCESS_HEAP_ENTRY_BUSY)
			busycount++;

		count++;
	}

	return busycount;
}

char *ConGetErrorString (int errindex);
char *ParGetErrorString (int errindex);
char *ScrGetErrorString (int errindex);
char *HwGetErrorString (int errindex);
char *InfGetErrorString (int errindex);
char *UtilGetErrorString (int errindex);
char *PfDerrString (DERR in)
{
	int subsystem = ((in & 0xFF000000) >> 24);
	int code = ((in & 0xFF0000) >> 16);

	switch (subsystem)
	{
	case CONSOLE_DERR_BASE:
		return ConGetErrorString (code);

	case PARSER_DERR_BASE:
		return ParGetErrorString (code);

	case PF_DERR_BASE:
		return PfGetErrorString (code);

	case SCRIPT_DERR_BASE:
		return ScrGetErrorString (code);

	case HELPWIN_DERR_BASE:
		return HwGetErrorString (code);

	case INF_DERR_BASE:
		return InfGetErrorString (code);

	case UTIL_DERR_BASE:
		return UtilGetErrorString (code);

	case WATCHER_DERR_BASE:
		return WatGetErrorString (code);
	}

	return "PF_UNKNOWN_SUBSYSTEM_CODE";
}


DERR PfSaneInitCriticalSection (CRITICAL_SECTION *cs)
{
	DERR st;
	st = PF_SUCCESS;
	__try
	{
		InitializeCriticalSection (cs);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		st = PF_OUT_OF_MEMORY;
	}
	return st;
}


char *PfGetArgParam (char *key)
{
	int x;

	for (x=0;x<PfArgc - 1;x++) //-1 because we need for there to be something to return.
	{
		if (strcmp (PfArgv[x], key) == 0)
			return PfArgv[x+1];
	}

	return NULL;
}

int PfIsArgumentPresent (char *arg)
{
	int x;

	for (x=0;x<PfArgc;x++)
	{
		if (strcmp (PfArgv[x], arg) == 0)
			return 1;
	}

	return 0;
}

int PfMessageBoxf (HWND hwnd, char *fmt, char *title, int flags, ...)
{
	char buffer[1024];
	va_list marker;

	va_start (marker, flags);
	_vsnprintf(buffer, 1023, fmt, marker);
	buffer[1023] = '\0';
	va_end (marker);

	return MessageBox (hwnd, buffer, title, flags);
}







DERR PfInitializeRemote (char *name)
{
	int st;
	char *pipename;
	int len;

	if (!name) //get the default name
	{
		//note: GetThreadDesktop returns a handle you don't need to close.
		//(it actually returns the value of a handle win32 keeps open all the time)
		st = PfGetDefaultPipeName (GetThreadDesktop (GetCurrentThreadId ()), NULL, NULL, &len);
		fail_if (!DERR_OK (st), st, 0);

		pipename = _alloca (len);

		st = PfGetDefaultPipeName (GetThreadDesktop (GetCurrentThreadId ()), NULL, pipename, &len);
		fail_if (!DERR_OK (st), st, 0);
	}
	else
	{
		pipename = name;
	}

	//st = RopInherit (&PfRemote.par);
	//fail_if (!DERR_OK (st), st, 0);

	PfRemote.mailslot = CreateMailslot (pipename, PF_BUFFER_SIZE - 1, MAILSLOT_WAIT_FOREVER, NULL);
	fail_if (PfRemote.mailslot == INVALID_HANDLE_VALUE, PF_CREATE_PIPE_FAILURE, PfRemote.mailslot = NULL; DIGLE);

	PfRemote.event = CreateEvent (NULL, TRUE, TRUE, NULL);
	fail_if (!PfRemote.event, PF_CREATE_EVENT_FAILURE, DIGLE);

	PfRemote.over.hEvent = PfRemote.event;

	ResetEvent (PfRemote.event);
	st = ReadFile (PfRemote.mailslot, PfRemote.buffer, PF_BUFFER_SIZE, &PfRemote.read, &PfRemote.over);
	fail_if (!st && GetLastError () != ERROR_IO_PENDING, PF_READ_FILE_FAILED, DIGLE);

	return PF_SUCCESS;

failure:

	//if (PfRemote.par)
	//	ParDestroy (PfRemote.par);

	if (PfRemote.event)
		CloseHandle (PfRemote.event);

	if (PfRemote.mailslot)
		CloseHandle (PfRemote.mailslot);
		
	memset (&PfRemote, 0, sizeof (PfRemote));

	return st;
}

DERR PfProcessIncomingMail (void)
{
	DWORD read;
	int st;
	char *output;
	char error[1024];
	int nesting;
	parerror_t pe = {0};

	//sanity check
	fail_if (!PfIsMainThread (), PF_NOT_IN_MAIN_THREAD, 0);

	st = GetOverlappedResult (PfRemote.mailslot, &PfRemote.over, &read, FALSE);
	fail_if (!st, PF_GET_OVERLAPPED_FAILED, DIGLE);

	PfRemote.buffer[read >= 1024 ? 1023 : read] = '\0';

	if (PfRemote.debugout)
		ConWriteStringf ("Mailslot in: %s\n", PfRemote.buffer);

	//parse it
	*error = '\0';
	output = NULL;
	nesting = 0;
	st = ParRunLineCurrent (PfMainParser, PfRemote.buffer, &output, &nesting, &pe);
	if (!DERR_OK (st))
		ParProcessErrorType (st, error, 1024, &pe); //always do this.

	if (PfRemote.debugout)
		ConWriteStringf ("Mailslot out: 0x%.8X,%i,%s\n", st, nesting, output ? output : error);

	if (output)
		ParFree (output);

	//no need to reset anymore, ParRunLineCurrent always does this.
	
	//issue the read again
	ResetEvent (PfRemote.event);
	st = ReadFile (PfRemote.mailslot, PfRemote.buffer, PF_BUFFER_SIZE, &PfRemote.read, &PfRemote.over);
	fail_if (!st && GetLastError () != ERROR_IO_PENDING, PF_READ_FILE_FAILED, DIGLE);

	return PF_SUCCESS;

failure:
	if (PfRemote.debugout)
		ConWriteStringf ("Mailslot failure: 0x%.8X\n", st);

	return st;
}



//side note: this function is stupid
DERR PfMainParserRun (char *line, char **output, parerror_t *pe)
{
	DERR st;

	st = ParRunLineCurrent (PfMainParser, line, output, NULL, pe);
	
	return st;
}


void PfFreeMtm (mtmessage_t *mtm)
{
	if (mtm->line)
		PfFree (mtm->line);

	if (mtm->output)
		ParFree (mtm->output);

	ParCleanupError (&mtm->pe);

	if (mtm->event)
		CloseHandle (mtm->event);

	PfFree (mtm);
}

//output and/or pe can be NULL
static DERR PfMainThreadMessage (void *func, void *param, char *line, char **output, parerror_t *pe)
{
	mtmessage_t *mtm = NULL;
	DERR st;
	LONG val;
	int messagesent = 0;

	fail_if (PfIsMainThread (), PF_ALREADY_IN_MAIN_THREAD, 0);

	mtm = PfMalloc (sizeof (mtmessage_t));
	fail_if (!mtm, PF_OUT_OF_MEMORY, PE_OUT_OF_MEMORY (sizeof (mtmessage_t)));
	memset (mtm, 0, sizeof (mtmessage_t));

	if (line)
	{
		//need to copy this in case we fail, we can't be sending around stack
		//pointers to things that don't exist.
		mtm->line = PfMalloc (strlen (line) + 1);
		fail_if (!mtm->line, PF_OUT_OF_MEMORY, DI (strlen (line) + 1) ; PE_OUT_OF_MEMORY (strlen (line) + 1));
		strcpy (mtm->line, line);

		mtm->type = MTMESSAGE_PARSE;
	}
	else
	{
		mtm->func = func;
		mtm->param = param;

		mtm->type = MTMESSAGE_FUNC;
	}

	mtm->event = CreateEvent (NULL, FALSE, FALSE, NULL);
	fail_if (!mtm->event, PF_CREATE_EVENT_FAILURE, DIGLE ; PE ("CreateEvent failed, %i", GetLastError (), 0, 0, 0));

	st = PostThreadMessage (PfMainThreadId, PfParseMessage, (WPARAM)mtm, PfMessageKey);
	fail_if (!st, 0, PE ("PostThreadMessage failed, %i", GetLastError (), 0, 0, 0));
	messagesent = 1;

	st = WaitForSingleObject (mtm->event, INFINITE);
	fail_if (st == WAIT_FAILED, PF_OBJECT_WAIT_FAILED, DIGLE ; PE ("WaitForSingleObject failed, %i", GetLastError (), 0, 0, 0));

	//no failures below this point!

	//ok.  So the main thread has signaled completion.  We know that the data will
	//not be freed yet because we haven't marked the mtm->control as 1 yet.
	//So, grab what we're going to return, and remove it from the struct so it won't
	//be freed by PfFreeMtm
	if (output)
	{
		*output = mtm->output;
		mtm->output = NULL;
	}
	if (pe)
	{
		memcpy (pe, &mtm->pe, sizeof (parerror_t));
		memset (&mtm->pe, 0, sizeof (parerror_t));
	}
	st = mtm->status;

	val = InterlockedExchange (&mtm->control, 1);
	if (val == 2)
		PfFreeMtm (mtm);

	return st;

failure:

	if (messagesent)
	{
		//see if we need to free it or not.
		val = InterlockedExchange (&mtm->control, 1);
		if (val != 2) //second thread still running, don't free. (checking != 2 instead of ==0 because some of the
			mtm = NULL; //failure code above may set it to 1, and in that case we don't want it to free.
	}

	if (mtm)
		PfFreeMtm (mtm);

	return st;


}

DERR PfMainThreadParse (char *line, char **output, parerror_t *pe)
{

	if (PfIsMainThread ())
		return PfMainParserRun (line, output, pe);

	return PfMainThreadMessage (NULL, NULL, line, output, pe);
}

DERR PfRunInMainThread (DERR (*func)(void *), void *param)
{
	if (PfIsMainThread ())
		return func (param);

	return PfMainThreadMessage (func, param, NULL, NULL, NULL);
}

	

DERR PfRunMainThreadMessage (mtmessage_t *mtm)
{
	DERR st;
	LONG val;
	//very little actually needs to be done.

	if (mtm->type == MTMESSAGE_PARSE)
		mtm->status = PfMainParserRun (mtm->line, &mtm->output, &mtm->pe);
	else
		mtm->status = mtm->func (mtm->param);

	st = SetEvent (mtm->event);
	fail_if (!st, PF_SET_EVENT_FAILED, DIGLE);
		

	val = InterlockedExchange (&mtm->control, 2);
	if (val == 1) //first thread has given up; free result
		PfFreeMtm (mtm);

	return PF_SUCCESS;
failure:

	val = InterlockedExchange (&mtm->control, 2);
	if (val == 1) //first thread not waiting, so free it.
		PfFreeMtm (mtm);

	//although, really, if setting this event failed, the first thread
	//is probably not going to be going anywhere.

	return st;
}





//Return values and meanings:
//
// PF_SUCCESS with *extrasignaled equal to FALSE
//   A WM_QUIT message was received, callers should signal errors on whatever
//   they were parsing and abort.
//
// PF_SUCCESS with *extrasignaled equal to TRUE
//   The handle passed in was signaled.
//
// A failure DERR code
//   Something failed.  Caller should abort with the DERR returned.
//
// extrasignaled may only be NULL if extra is also NULL
DERR PfMessageLoop (HANDLE extra, int *extrasignaled)
{
	HANDLE waits[2] = {PfRemote.event, extra};
	DWORD res;
	MSG msg;
	int st;
	DWORD remotecode, extracode, messagecode;
	int count = 0;

	if (GetCurrentThreadId () != PfMainThreadId)
	{
		//called from another thread.  We only do the wait, and the implied message loop,
		//if we're running in the main thread.  In all other threads, it is considered ok
		//to stall the thread entirely for a (bg.sync) command.  Only the GUI thread needs
		//to be kept running in this manner.  Therefore, if you call this from another
		//thread without specifying something else you want to wait on (generally it's
		//a handle to a thread) it's an error
		if (!extra)
			return PF_NOTHING_TO_DO;
		
		res = WaitForSingleObject (extra, INFINITE);
		fail_if (res == WAIT_FAILED, PF_OBJECT_WAIT_FAILED, DIGLE);

		if (extrasignaled)
			*extrasignaled = 1;
		return PF_SUCCESS;
	}


	if (PfRemote.mailslot) //could also test .event, they should be NULL or not together
	{
		waits[count] = PfRemote.event;
		remotecode = WAIT_OBJECT_0 + count;
		count++;
	}

	if (extra)
	{
		waits[count] = extra;
		extracode = WAIT_OBJECT_0 + count;
		count++;
	}

	messagecode = WAIT_OBJECT_0 + count;


	while (1)
	{
		res = MsgWaitForMultipleObjects (count, waits, FALSE, INFINITE, QS_ALLEVENTS | QS_ALLINPUT | QS_ALLPOSTMESSAGE | QS_HOTKEY | QS_INPUT | QS_KEY | QS_MOUSE | QS_MOUSEBUTTON | QS_MOUSEMOVE | QS_PAINT | QS_POSTMESSAGE | QS_SENDMESSAGE | QS_TIMER);

		if (res == messagecode)
		{
			//messages await, process them
			while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
				{
					//post another one so that the next higher message
					//loop will see it
					PostQuitMessage (msg.wParam);

					//then return
					if (extrasignaled)
						*extrasignaled = FALSE;
					return PF_SUCCESS;
				}
				else if (msg.message == PfAbortMessage && msg.hwnd == NULL && msg.lParam == PfMessageKey)
				{
					//mainly intended for debugging purposes, this acts like WM_QUIT except that
					//it does not break out of the root message loop.  (Only breaks if extra is
					//not null)
					if (extra) //throw it out again
					{
						PostThreadMessage (GetCurrentThreadId (), PfAbortMessage, 0, PfMessageKey);
						if (extrasignaled)
							*extrasignaled = FALSE;
						return PF_SUCCESS;
					}
				}
				else if (msg.message == PfParseMessage && msg.hwnd == NULL && msg.lParam == PfMessageKey)
				{
					st = PfRunMainThreadMessage ((mtmessage_t *)msg.wParam);
					if (!DERR_OK (st)) //just report it, there's not much that can be done.
						ConWriteStringf ("PfRunMainThreadMessage failed, {%s,%i}\n", PfDerrString (st), GETDERRINFO (st));
				}
				else
				{
					TranslateMessage (&msg);
					DispatchMessage (&msg);
				}
			}
		}
		else if (res == remotecode) //the pipe
		{
			st = PfProcessIncomingMail ();
			//fail_if (!DERR_OK (st), st, 0);
			//not worth aborting the process over
		}
		else if (res == extracode) //the handle they passed in
		{
			if (extrasignaled)
				*extrasignaled = TRUE;
			return PF_SUCCESS;
		}
		else
		{
			fail_if (TRUE, PF_OBJECT_WAIT_FAILED, DIGLE);
		}

	}

failure:
	return st;
}


DERR PfParseInRemoteP5 (char *name, char *line)
{
	char *slotname;
	int st, len;

	if (!line || !*line)
		return PF_SUCCESS; //if we have nothing to parse, we've succeeded

	if (!name) //get the default name
	{
		//noteL GetThreadDesktop returns a handle you don't need to close.
		//(it actually returns the value of a handle win32 keeps open all the time)
		st = PfGetDefaultPipeName (GetThreadDesktop (GetCurrentThreadId ()), NULL, NULL, &len);
		fail_if (!DERR_OK (st), st, 0);

		slotname = _alloca (len);

		st = PfGetDefaultPipeName (GetThreadDesktop (GetCurrentThreadId ()), NULL, slotname, &len);
		fail_if (!DERR_OK (st), st, 0);
	}
	else
	{
		slotname = name;
	}

	st = PfSendMailCommand (slotname, line);
	fail_if (!DERR_OK (st), st, 0);

	return PF_SUCCESS;

failure:

	{
		char buffer[1024];
		sprintf (buffer, "failure: 0x%.8X\n", st);
		OutputDebugString (buffer);
	}

	return st;
}

DERR PfSetProcessDirectory (void)
{
	DERR st;
	char buffer[MAX_PATH];
	char *start, *walk;
	char delim;

	st = GetModuleFileName (NULL, buffer, MAX_PATH - 1);
	fail_if (!st, PF_GET_MODULE_FAILED, DIGLE);
	buffer[MAX_PATH - 1] = '\0';

	//sometimes, the thing comes in quotes.... apparently
	if (buffer[0] == '\"')
	{
		start = buffer + 1;
		delim = '\"';
	}
	else
	{
		start = buffer;
		delim = ' ';
	}

	walk = start;
	while (*walk && *walk != delim)
		walk++;

	//whether it's \0 or \" or ' ', walk backwards looking for '\\'
	while (walk >= start && *walk != '\\')
		walk--;

	//shouldn't happen, GetModuleFileName is supposed to return the full path, 
	//which MUST have at least one \ in it.
	fail_if (walk == start - 1, PF_SANITY_FAILURE, 0);

	*walk = '\0';

	st = SetCurrentDirectory (start);
	fail_if (!st, PF_SET_DIRECTORY_FAILED, DIGLE);

	return PF_SUCCESS;
failure:
	return st;
}


DERR PfRunStartupScript (char *filename, int msgbox)
{
	char error[512];
	DERR st;
	int line;

	*error = '\0';

	st = ScrExecute (filename, NULL, &line, error, 512, 0, NULL);
	if (!DERR_OK (st))
	{
		if (msgbox)
			PfMessageBoxf (NULL, "Startup script failure:\r\nline %i\r\nstatus {%s,%i}\r\nerror: %s", "p5 Startup error", 0, line, PfDerrString (st), GETDERRINFO (st), error);
	}

	return st;
}


LRESULT CALLBACK PfMessagesOfInterest (UINT message, WPARAM wParam, LPARAM lParam)
{
	char *arg;
	DERR st;

	switch (message)
	{
	case WM_QUERYENDSESSION:
		return TRUE;

	case WM_ENDSESSION:
		//some shutting down needs to happen.
		arg = PfGetArgParam ("--shutdownscript");
		if (!arg)
			arg = "shutdown.p5";
		st = PfRunStartupScript (arg, 0); //will not put up any message box
		
		//do the plugin shutdown mambo
		PlugShutdown ();

		ExitProcess (st);

		return 23834535;
	}

	return 0;

}


int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE crap, char *line, int show)
{
	char *arg;
	DERR st;
	char *fmt = NULL;
	int res;
	int pfin = 0, parin = 0, ropin = 0, mwin = 0, plugin = 0, infin = 0, watin = 0;

	PfInstance = hInstance;

	st = PfInit ();
	fail_if (!DERR_OK (st), st, fmt = "Main subsystem init failed: 0x%.8X");
	pfin = 1;

	//hack: --parse is supposed to put the whole rest of the command line into the remote
	//parser mailslot.  If we modify StrTokenize to handle embedded quotes in quotes, then
	//we will have to translate the string before passing it here, and I don't want to
	//do that, especially since this is expected to be used as the main interface for a 
	//p5:// protocol.  So, we check for --parse before we tokenize, and if it's there,
	//we null off the rest of the string, and save a pointer to it.  This allows us to have
	//a command line like:
	//--mailslotname Foodeebar --parse (func1 (func2 "lalala" 4) (func3 "lala\"la\"lala\"\""))
	//work correctly, which wouldn't be possible without this hack.
	//NOTE: No other param can begin with "--parse" or shit will go bad.

	arg = strstr (line, "--parse");
	if (arg)
	{
		arg += strlen ("--parse");
		//null out and pass the next character (if it isn't '\0')
		if (*arg)
			*(arg++) = '\0';
		//skip whitespace
		while (*arg && (*arg == ' ' || *arg == '\t' || *arg == '\r' || *arg == '\n'))
			arg++;
		//if arg == "", that's ok.
	}

	//note: don't set arg to anything before checking it for the call to PfParseInRemoteP5

	res = StrTokenize (line, &PfArgc, &PfArgv);
	fail_if (!res, PF_INVALID_COMMAND_LINE, fmt = "p5: Invalid command line");

	if (PfIsArgumentPresent ("--help"))
	{
		PfUsageMessage ();
		return 0;
	}

	//arg = PfGetArgParam ("--parse"); //arg has already been set to this.
	if (arg)
	{
		//object is to do nothing but pass the given string on to the p5 shell that
		//should already exist on the current desktop.

		return PfParseInRemoteP5 (PfGetArgParam ("--mailslotname"), arg);
	}
	
	//If we're in slave console mode, do that now before we take up any more ram.
	arg = PfGetArgParam ("--runconsole");
	if (arg)
		ConRunDummyConsole (arg); //this function never returns; errors come through process exit status

	arg = PfGetArgParam ("--directory");
	if (arg)
	{
		st = SetCurrentDirectory (arg);
		fail_if (!st, PF_SET_DIRECTORY_FAILED, DIGLE ; fmt = "Could not set proper directory: 0x%.8X");
	}
	else
	{
		st = PfSetProcessDirectory ();
		fail_if (!DERR_OK (st), st, fmt = "Could not set proper directory: 0x%.8X");
	}

	st = ParInit ();
	fail_if (!DERR_OK (st), st, fmt = "Parser subsystem init failed: 0x%.8X");
	parin = 1;

	st = RopInit ();
	fail_if (!DERR_OK (st), st, fmt = "Stdlib [rop] subsystem init failed: 0x%.8X");
	ropin = 1;

	res = MwInit (hInstance);
	fail_if (!res, PF_MAINWINDOW_FAILURE, fmt = "MwInit returned failure");
	mwin = 1;

	st = WatInit ();
	fail_if (!DERR_OK (st), st, fmt = "Watched subsystem init failed: 0x%.8X");
	watin = 1;

	st = InfInit ();
	fail_if (!DERR_OK (st), st, fmt = "Info subsystem init failed, 0x%.8X");
	infin = 1;

	st = PlugInit ();
	fail_if (!DERR_OK (st), st, fmt = "Plugin subsystem init failed: 0x%.8X");
	plugin = 1;

	//this needs to go after RopInit
	st = RopInherit (&PfMainParser);
	fail_if (!DERR_OK (st), st, fmt = "Main Parser creation failed: 0x%.8X");

	if (!PfIsArgumentPresent ("--nomailslot"))
	{
		arg = PfGetArgParam ("--mailslotname");
		st = PfInitializeRemote (arg); //OK if arg is null
		fail_if (!DERR_OK (st), st, fmt = "Remote pipe subsystem init failed: 0x%.8X");
	}

	arg = PfGetArgParam ("--mainwindowtitle");
	if (!arg)
		arg = "p5 main window";
	PfMainWindow = MwCreate (hInstance, arg);
	fail_if (!PfMainWindow, PF_MAINWINDOW_FAILURE, fmt = "MwCreate returned failure");

	res = MwAddMessageFullCall (PfMainWindow, WM_QUERYENDSESSION, 0, 0, 0, 0, PfMessagesOfInterest, FLAG_MESSAGE | FLAG_WPARAM | FLAG_LPARAM, 0);
	fail_if (!res, PF_MESSAGEREGISTER_FAILURE, fmt = "MwAddMessage returned failure");
	res = MwAddMessageFullCall (PfMainWindow, WM_ENDSESSION, 0, 0, 0, 0, PfMessagesOfInterest, FLAG_MESSAGE | FLAG_WPARAM | FLAG_LPARAM, 0);
	fail_if (!res, PF_MESSAGEREGISTER_FAILURE, fmt = "MwAddMessage returned failure");

	st = InfAddInfoBlock ("script", hInstance, IDR_SCRIPT);
	fail_if (!DERR_OK (st), st, fmt = "Could not add info topic on scripts, 0x%.8X");
	st = InfAddInfoBlock ("watcher", hInstance, IDR_WATCHER);
	fail_if (!DERR_OK (st), st, fmt = "Could not add info topic on watchers, 0x%.8X");
	st = InfAddInfoBlock ("script hash", hInstance, IDR_SCRIPTHASH);
	fail_if (!DERR_OK (st), st, fmt = "Could not add info topic on the script # command, 0x%.8X");
	st = InfAddInfoBlock ("command line", hInstance, IDR_COMMANDLINE);
	fail_if (!DERR_OK (st), st, fmt = "Could not add info topic on command line params, 0x%.8X");



	if (PfIsArgumentPresent ("--debugconsole"))
	{
		st = ConOpenDebugConsole ();
		fail_if (!DERR_OK (st), st, fmt = "Failed to open console: 0x%.8X");
	}

	if (PfIsArgumentPresent ("--stressparser"))
	{
		RopParserStressTest ();
	}

	PfMainThreadId = GetCurrentThreadId ();

	//ok, ready to go, now, before we enter the main loop, we need to run the
	//startup script.
	arg = PfGetArgParam ("--startupscript");
	if (!arg)
		arg = "startup.p5";
	st = PfRunStartupScript (arg, 1); //will put up its own message box if it fails
	fail_if (!DERR_OK (st), st, fmt = NULL);

	PfMessageLoop (NULL, NULL);

	arg = PfGetArgParam ("--shutdownscript");
	if (!arg)
		arg = "shutdown.p5";
	st = PfRunStartupScript (arg, 0); //will not put up any message box
	fail_if (!DERR_OK (st), st, fmt = NULL); //but go ahead and exit the process with a failure code.

	PlugDenit ();
	ParDestroy (PfMainParser);
	InfDenit ();
	WatDenit ();
	MwDenit (hInstance);
	RopDenit ();
	ParDenit ();
	PfDenit ();

	return 0;

failure:

	if (plugin)
		PlugDenit ();

	if (PfMainParser)
		ParDestroy (PfMainParser);

	if (infin)
		InfDenit ();

	if (watin)
		WatDenit ();

	if (mwin)
		MwDenit (hInstance);

	if (ropin)
		RopDenit ();

	if (parin)
		ParDenit ();

	if (pfin)
		PfDenit ();

	if (fmt)
		PfMessageBoxf (NULL, fmt, "p5 startup error", MB_ICONERROR, st);

	return st;
}




/////////////////////  Script Functions  /////////////////////////

int PfSetRemoteDebugOut (int count, int value)
{
	if (count == 1) //means value is complete garbage, they just want to query
		return PfRemote.debugout;

	count = PfRemote.debugout;
	PfRemote.debugout = value;
	return count;
}

void PfSleep (int seconds)
{
	Sleep (seconds * 1000);
}

void PfQuit (parerror_t *pe)
{
	int st;

	if (MessageBox (NULL, "Are you sure you want to exit?", "Exit p5?", MB_ICONQUESTION | MB_YESNO) == IDYES)
	{
		st = PostThreadMessage (PfMainThreadId, WM_QUIT, 0, 0);
		fail_if (!st, 0, PE ("PostThreadMessage returned %i, you might want to consider terminating p5.exe", GetLastError (), 0, 0, 0));
	}
	else
	{
		fail_if (TRUE, 0, PE ("User reject quit request", 0, 0, 0, 0));
	}
failure:
	return;
}

void PfForceQuit (parerror_t *pe)
{
	int st;
	
	st = PostThreadMessage (PfMainThreadId, WM_QUIT, 0, 0);
	fail_if (!st, 0, PE ("PostThreadMessage returned %i, you might want to consider terminating p5.exe", GetLastError (), 0, 0, 0));
failure:
	return;
}

void PfAbort (parerror_t *pe)
{
	DERR st;

	st = PostThreadMessage (PfMainThreadId, PfAbortMessage, 0, PfMessageKey);
	fail_if (!st, 0, PE ("PostThreadMessage returned %i; not aborting anything.", GetLastError (), 0, 0, 0));
failure:
	return;
}

char *PfMainParse (parerror_t *pe, char *what)
{
	DERR st;
	char *output;

	st = PfMainThreadParse (what, &output, pe);

	if (!DERR_OK (st))
	{
		if (output)
			ParFree (output);

		return NULL;
	}

	return output;
}


typedef struct
{
	//the control value.  When the struct is created, it is set to zero.  When the slave
	//thread finishes putting values in it, it InterlockedExchanges the value to 2.  If
	//the previous value was zero, then all is well, and the master thread now owns the
	//object and is responsible for freeing it.  If the previous value was 1, then the
	//master thread has aborted for some reason, and the slave thread is expected to free
	//the values and throw them away.  When the master thread drops out of the message
	//loop, it InterlockedExchanges the value to 1.  If the previous value was zero, then
	//the slave thread is still running, and responsibility for freeing it passes to the
	//slave thread.  If the previous value was 2, then the master thread frees the values
	//before returning failure.
	//
	//Basically, 0 means both threads are still looking at it, they set it to nonzero
	//when they're done.  The first one to be done does not free the memory, the second
	//one does.  First or second is determined by the old value of control.  
	LONG control;


	char *line;

	DERR status;	//if PF_SUCCESS, output is valid, otherwise error is valid.
					//In low memory situations, output and error may still be
					//null despite the status, so test them before you use them.
	char *output;	//will be NULL if error is non null
	char *error;	//will be NULL if output is non null.
} bgrun_t;

static void PfFreeBgrun (bgrun_t *bg)
{
	if (bg->output)
		PfFree (bg->output);
	if (bg->error)
		PfFree (bg->error);
	if (bg->line)
		PfFree (bg->line);
	PfFree (bg);
}

DWORD CALLBACK PfBackgroundRunThread (void *param)
{
	bgrun_t *bg = param;
	char *output = NULL;
	DERR st;
	parser_t *par = NULL;
	char error[1024];
	int nesting;
	LONG val;

	*error = '\0'; //failure will check for this and fill it based on st if it's still empty
	
	st = RopInherit (&par);
	fail_if (!DERR_OK (st), st, 0);

	st = ParRunLine (par, bg->line, &output, &nesting, error, 1024);
	fail_if (!DERR_OK (st), st, 0);
	fail_if (nesting, 0, strcpy (error, "Syntax error in bg.* command atom.  (Incomplete s-expression)"));

	ParReset (par);
	ParDestroy (par);
	par = NULL;

	//ok, so we're successful.
	if (output)
	{
		bg->output = PfMalloc (strlen (output) + 1);
		fail_if (!bg->output, PF_OUT_OF_MEMORY, DI (strlen (output) + 1));
		strcpy (bg->output, output);

		ParFree (output);
		output = NULL;
	}
	//else do nothing, because bg->output is already NULL

	bg->status = PF_SUCCESS;

	val = InterlockedExchange (&bg->control, 2);
	if (val != 0) //if main thread has released it
	{
		ConWriteStringf ("bg %i output: %s\n", GetCurrentThreadId (), bg->output);
		PfFreeBgrun (bg);
	}

	return PF_SUCCESS;

failure:

	if (output)
		ParFree (output);

	if (par)
	{
		ParReset (par);
		ParDestroy (par);
	}

	//ok, now the error thing.
	if (*error)
	{
		bg->error = PfMalloc (strlen (error) + 1);
		if (bg->error)
			strcpy (bg->error, error);
		//else leave it NULL, it will be output as "out of memory"
	}
	else
	{
		bg->error = PfMalloc (500);
		if (bg->error)
		{
			_snprintf (bg->error, 499, "{%s,%i}", PfDerrString (st), GETDERRINFO (st));
			bg->error[499] = '\0';
		}
	}

	bg->status = st;

	val = InterlockedExchange (&bg->control, 2);
	if (val != 0) //main thread has released error
	{
		ConWriteStringf ("bg %i error: %s\n", GetCurrentThreadId (), bg->error);
		PfFreeBgrun (bg);
	}

	return st;
}

	


char *PfBgSync (parerror_t *pe, char *line)
{
	bgrun_t *bg = NULL;
	char *output;
	DWORD tid;
	HANDLE thread = NULL;
	DERR st;
	int extrasignaled = 0;
	LONG val;

	bg = PfMalloc (sizeof (bgrun_t));
	fail_if (!bg, 0, PE_OUT_OF_MEMORY (sizeof (bgrun_t)));
	memset (bg, 0, sizeof (bgrun_t));

	bg->line = PfMalloc (strlen (line) + 1);
	fail_if (!bg->line, 0, PE_OUT_OF_MEMORY (strlen (line) + 1));
	strcpy (bg->line, line);


	thread = CreateThread (NULL, 0, PfBackgroundRunThread, bg, 0, &tid);
	fail_if (!thread, 0, PE ("Could not create background thread, %i", GetLastError (), 0, 0, 0)); 

	st = PfMessageLoop (thread, &extrasignaled);
	fail_if (!DERR_OK (st), st, PE ("Message loop encountered error {%s,%i}, aborting.", PfDerrString (st), GETDERRINFO (st), 0, 0));
	//also "fail" if it's WM_QUIT.  NOTE:  sets st to PF_SUCCESS
	fail_if (!extrasignaled, PF_SUCCESS, PE ("p5 is shutting down; aborting bg.sync command", 0, 0, 0, 0));

	st = GetExitCodeThread (thread, &tid);
	fail_if (!st, 0, PE ("Unable to retrieve thread exit status, %i", GetLastError (), 0, 0, 0));
	fail_if (tid == STILL_ACTIVE, 0, PE ("Consistency failure: thread object signaled, but STILL_ACTIVE is exit status", 0, 0, 0, 0));

	//it is important to do this now, above the line below that fails if val != 2.
	//In the failure code, bg will not be freed if thread != NULL and val != 0, which
	//is what will happen.  (Val will be 1, because it's set to that just below these
	//lines.  It's ok to set this to NULL now, because we know the thread has finished.
	CloseHandle (thread);
	thread = NULL;

	//ok, the thread finished processing, figure out what to do.  First, test the control
	//value.  If it isn't 2, then the slave thread must have crashed or otherwise exited
	//uncleanly.  This is mostly a sanity test, as the only way this could really happen
	//is if the thread called ExitThread(), which shouldn't ever be done.
	val = InterlockedExchange (&bg->control, 1);
	
	//according to the control value, the slave thread has not yet finished, but
	//we know that's not true, because the thread object was signaled.  So
	//something went pretty badly wrong somewhere.  We will free this memory, 
	fail_if (val != 2, 0, PE ("Consistency failure in thread exit procedure", 0, 0, 0, 0));

	//Ok.  The thread ran.  The thread finished.  The thread set failure info for us
	//like a good little thread.  We are the owner of bg.  Everything has gone as it
	//is supposed to.  Yay life.

	//do the failure thing if the thread didn't succeed.
	fail_if (!DERR_OK (bg->status), 0, PE_STR ("Background thread failed: %s", COPYSTRING (bg->error ? bg->error : "[Out of memory allocating error string]"), 0, 0, 0));

	//bg->status is PF_SUCCESS.  Copy the output to a parser allocated string.  We
	//should consider allocating the output string from the parser heap.  I'm not
	//doing that because I think it will introduce a greater likelyhood of mis-freeing.

	if (bg->output)
	{
		output = ParMalloc (strlen (bg->output) + 1);
		fail_if (!output, 0, PE ("Out of memory attempting to copy return buffer back from bg.sync thread.", 0, 0, 0, 0));
		strcpy (output, bg->output);
	}
	else
	{
		output = NULL; //return NULL just like the func did.
	}

	PfFreeBgrun (bg);


	return output;
failure:

	//On shutdown, plugins are denitted but NOT removed from RAM.  (We let the OS handle
	//that for us.)  This is done so that the threads that get away from us in situations
	//such as this don't suddenly wake up after their plugin has been unloaded.  So, whether
	//this was an error in PfMessageLoop or a WM_QUIT received, we let the thread continue.
	//No ram will be leaked, see the comments about bgrun_t.control.
	if (thread)
	{
		//we need to test for thread first, because the process of freeing bg is
		//more complicated if the second thread has been run.  Getting here means
		//the thread has started, so even if it hasn't yet gotten to run, we're
		//still ok, because ->control will be set to 1, signaling the target thread
		//to free bg when it is finished.  If the second thread has released control
		//of bg, val will be 2 when tested, bg will stay non-null, and will be
		//freed by the cleanup code.
		val = InterlockedExchange (&bg->control, 1);
		if (val == 0) //second thread still running, don't free bg
			bg = NULL;

		CloseHandle (thread);
	}

	if (bg)
		PfFreeBgrun (bg);

	return NULL;
}


int PfBgAsync (parerror_t *pe, char *line)
{
	bgrun_t *bg = NULL;
	DWORD tid;
	HANDLE thread = NULL;
	DERR st;

	//christ this one is easy...

	bg = PfMalloc (sizeof (bgrun_t));
	fail_if (!bg, 0, PE_OUT_OF_MEMORY (sizeof (bgrun_t)));
	memset (bg, 0, sizeof (bgrun_t));

	bg->line = PfMalloc (strlen (line) + 1);
	fail_if (!bg->line, 0, PE_OUT_OF_MEMORY (strlen (line) + 1));
	strcpy (bg->line, line);

	//set control to 1 now, we want the thread to free its own stuff.
	bg->control = 1;

	thread = CreateThread (NULL, 0, PfBackgroundRunThread, bg, 0, &tid);
	fail_if (!thread, 0, PE ("Could not create background thread, %i", GetLastError (), 0, 0, 0)); 

	CloseHandle (thread);

	return tid;

failure:

	if (thread)
		CloseHandle (thread);

	if (bg)
		PfFreeBgrun (bg);

	return 0;
}


void PfReport (void)
{
	ConWriteString ("Heap report:\n");
	ConWriteStringf ("Main:     %i\n", PfHeapCount ());
	ConWriteStringf ("Parser:   %i\n", ParHeapCount ());
}

void PfMainThreadRequire (parerror_t *pe)
{
	if (!PfIsMainThread ())
	{
		PE ("Not running in main thread", 0, 0, 0, 0);
	}
}

void PfMainThreadForbid (parerror_t *pe)
{
	if (PfIsMainThread ())
	{
		PE ("Running in main thread", 0, 0, 0, 0);
	}
}

int PfScriptIsMainThread (void)
{
	return PfIsMainThread ();
}


DERR PfAddGlobalCommands (parser_t *root)
{
	int st;

	st = ParAddFunction (root, "p5.dbg.mail", "ici", PfSetRemoteDebugOut, "Set remote mailslot debug output value", "(mailslotdebug [i:bool value]) = [i:old value]", "Sets the flag that determines whether debug information about mailslot activity is output to the console (if one is open).  Returns the previous value of the flag.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.dbg.mail", "ic", PfSetRemoteDebugOut, "Get remote mailslot debug output value", "(mailslotdebug [i:bool value]) = [i:current value]", "Gets the flag that determines whether debug information about mailslot activity is output to the console (if one is open).");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "p5.dbg.sleep", "vi", PfSleep, "Delay execution", "(sleep [i:Seconds]) = [v]", "This function is probably only useful for development.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.dbg.report", "v", PfReport, "Report non-plugin heap usage", "(report) = [v]", "Counts and reports on the number of blocks in the main and parser heaps.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "p5.quit", "ve", PfQuit, "Exit p5", "(quit) = [v]", "A message box will confirm the user's intent.  If you don't want the user to be asked, use (quit.force)");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.forcequit", "ve", PfForceQuit, "Exit p5", "(quit.force) = [v]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.dbg.abort", "ve", PfAbort, "Abandon waiting for bg.sync commands to complete in the main thread.", "(abort) = [v]", "This function posts a WM_ABORT message to the main thread.  Its main use is for debugging, so I can break out of those things at odd times to make sure everything is cleaned up properly.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "p5.exec.fg.run", "uea", PfMainParse, "Run an s-expression in the main parser (in the main thread)", "(main.parse [a:s-expression]) = [u:expr. result]", "Use this for those commands that MUST run in the main thread.  Intended mainly for scripts and defines, its purpose is to keep things working as expected if the user uses bg.sync or bg.async to run your command.  Note that you will be running in a different parsing context, i.e. only variables defined globally will be in scope.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.exec.fg.require", "ve", PfMainThreadRequire, "Fails if not running in the main thread.", "(fg.require) = [v]", "This function does nothing except cause a failure if it is not run in the main thread.  Use it at the beginning of a script or (begin) that must run in the main thread.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.exec.fg.forbid", "ve", PfMainThreadForbid, "Fails if running in the main thread.", "(fg.forbid) = [v]", "This function does nothing except cause a failure if it is run in the main thread.  Use it at the beginning of a script or (begin) that must NOT run in the main thread.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.exec.fg.check", "i", PfScriptIsMainThread, "Checks whether code is running in the main thread.", "(fg.check) = [i:ismainthread]", "This function checks to see whether the main thread is running it.  If it is running in the main thread, it returns 1, otherwise it returns 0.  In no case will an error be generated.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.exec.bg.sync", "uea", PfBgSync, "Run an s-expression in a background thread synchronously", "(bg.sync [a:s-expression]) = [u:expr. result]", "This command runs the given s-expression in a background thread, allowing GUI operation to continue in the main thread.  Use it for operations that take a long time that you don't want blocking the entire program.  The bg.sync command will not return until the command has completed.  Note that bg.sync runs in a different scope; only global variables will be visible.  Bg.sync returns what the command returns.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.exec.bg.async", "iea", PfBgAsync, "Run an s-expression in a background thread asynchronously", "(bg.async [a:s-expression]) = [i:s/f]", "This command runs the given s-expression in a background thread, allowing GUI operation to continue in the main thread.  Bg.async returns immediately.  The result of the command will be output to the console, if one is open.  Note that bg.async runs in a different scope; only global variables will be visible.  Bg.async returns the thread identifier of the thread running the command.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddAlias (root, "fg",  "uea", "p5.exec.fg.run");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "bg.sync", "uea", "p5.exec.bg.sync");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "bg.async", "iea", "p5.exec.bg.async");	fail_if (!DERR_OK (st), st, 0);


	return PF_SUCCESS;
failure:
	return st;
}


/*
TODO:
+proper main failure handling
+startup.p5 script running
+WM_PARSEINMAIN for console to run in main thread
+bg.sync and bg.async

  */
