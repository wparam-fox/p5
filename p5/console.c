/* Copyright 2006 wParam, licensed under the GPL */

#include <windows.h>
#include <stdio.h>

#include "p5.h"


#define CONSOLE_PARENT_EXITED			DERRCODE (CONSOLE_DERR_BASE, 0x01)
#define CONSOLE_ARGUMENT_ERROR			DERRCODE (CONSOLE_DERR_BASE, 0x02)
#define CONSOLE_OPEN_STDOUT_FAILED		DERRCODE (CONSOLE_DERR_BASE, 0x03)
#define CONSOLE_READER_READ_FAILED		DERRCODE (CONSOLE_DERR_BASE, 0x04)
#define CONSOLE_READER_WRITE_FAILED		DERRCODE (CONSOLE_DERR_BASE, 0x05)
#define CONSOLE_OPEN_STDIN_FAILED		DERRCODE (CONSOLE_DERR_BASE, 0x06)
#define CONSOLE_WRITER_READ_FAILED		DERRCODE (CONSOLE_DERR_BASE, 0x07)
#define CONSOLE_WRITER_WRITE_FAILED		DERRCODE (CONSOLE_DERR_BASE, 0x08)
#define CONSOLE_CONSOLE_ERROR			DERRCODE (CONSOLE_DERR_BASE, 0x09)
#define CONSOLE_WATCHER_THREAD_ERROR	DERRCODE (CONSOLE_DERR_BASE, 0x0A)
#define CONSOLE_READER_THREAD_ERROR		DERRCODE (CONSOLE_DERR_BASE, 0x0B)
#define CONSOLE_WRITER_THREAD_ERROR		DERRCODE (CONSOLE_DERR_BASE, 0x0C)
#define CONSOLE_OPEN_SELF_ERROR			DERRCODE (CONSOLE_DERR_BASE, 0x0D)
#define CONSOLE_CREATE_EVENT_FAILED		DERRCODE (CONSOLE_DERR_BASE, 0x0E)
#define CONSOLE_CREATE_PIPE_ERROR		DERRCODE (CONSOLE_DERR_BASE, 0x0F)
#define CONSOLE_GETFILENAME_ERROR		DERRCODE (CONSOLE_DERR_BASE, 0x10)
#define CONSOLE_CHILD_CREATE_ERROR		DERRCODE (CONSOLE_DERR_BASE, 0x11)
#define CONSOLE_ALREADY_OPEN			DERRCODE (CONSOLE_DERR_BASE, 0x12)
#define CONSOLE_UNKNOWN_STATUS			DERRCODE (CONSOLE_DERR_BASE, 0x13)
#define CONSOLE_NOT_OPEN				DERRCODE (CONSOLE_DERR_BASE, 0x14)
#define CONSOLE_WRITE_FAILED			DERRCODE (CONSOLE_DERR_BASE, 0x15)
//#define CONSOLE_SUCCESS				DERRCODE (CONSOLE_DERR_BASE, 0x16)
#define CONSOLE_READ_FAILED				DERRCODE (CONSOLE_DERR_BASE, 0x17)
#define CONSOLE_CHILD_EXITED			DERRCODE (CONSOLE_DERR_BASE, 0x18)
#define CONSOLE_EXIT_FORCED				DERRCODE (CONSOLE_DERR_BASE, 0x19)
#define CONSOLE_STARTUP_FAILED			DERRCODE (CONSOLE_DERR_BASE, 0x1A)
#define CONSOLE_CREATE_ACTION_FAILED	DERRCODE (CONSOLE_DERR_BASE, 0x1B)
#define CONSOLE_CREATE_WATCHER_FAILED	DERRCODE (CONSOLE_DERR_BASE, 0x1C)
#define CONSOLE_FORCE_CLOSED			DERRCODE (CONSOLE_DERR_BASE, 0x1D)
#define CONSOLE_FREOPEN_ERROR			DERRCODE (CONSOLE_DERR_BASE, 0x1E)


char *ConErrors[] = 
{
	NULL,
	"CONSOLE_PARENT_EXITED",
	"CONSOLE_ARGUMENT_ERROR",
	"CONSOLE_OPEN_STDOUT_FAILED",
	"CONSOLE_READER_READ_FAILED",
	"CONSOLE_READER_WRITE_FAILED",
	"CONSOLE_OPEN_STDIN_FAILED",
	"CONSOLE_WRITER_READ_FAILED",
	"CONSOLE_WRITER_WRITE_FAILED",
	"CONSOLE_CONSOLE_ERROR",
	"CONSOLE_WATCHER_THREAD_ERROR",
	"CONSOLE_READER_THREAD_ERROR",
	"CONSOLE_WRITER_THREAD_ERROR",
	"CONSOLE_OPEN_SELF_ERROR",
	"CONSOLE_CREATE_EVENT_FAILED",
	"CONSOLE_CREATE_PIPE_ERROR",
	"CONSOLE_GETFILENAME_ERROR",
	"CONSOLE_CHILD_CREATE_ERROR",
	"CONSOLE_ALREADY_OPEN",
	"CONSOLE_UNKNOWN_STATUS",
	"CONSOLE_NOT_OPEN",
	"CONSOLE_WRITE_FAILED",
	"CONSOLE_SUCCESS",
	"CONSOLE_READ_FAILED",
	"CONSOLE_CHILD_EXITED",
	"CONSOLE_EXIT_FORCED",
	"CONSOLE_STARTUP_FAILED",
	"CONSOLE_CREATE_ACTION_FAILED",
	"CONSOLE_CREATE_WATCHER_FAILED",
	"CONSOLE_FORCE_CLOSED",
	"CONSOLE_FREOPEN_ERROR",

};

char *ConGetErrorString (int errindex)
{
	int numerrors = sizeof (ConErrors) / sizeof (char *);

	if (errindex >= numerrors || !ConErrors[errindex])
		return "CONSOLE_UNKNOWN_ERROR_CODE";

	return ConErrors[errindex];
}


//note that this func returns 1 on success; I don't want to change that now,
//but we're using 0 to mean success in this program.
int ConExtractParams (char *params, HANDLE *pro, HANDLE *in, HANDLE *out, HANDLE *event)
{
	int temp;
	int st;

	temp = atoi (params);
	if (!temp)
		return 0;

	*pro = (HANDLE)temp;

	while (*params && *params != ',')
		params++;
	fail_if (!*params, 0, 0);
	params++;

	temp = atoi (params);
	fail_if (!temp, 0, 0);

	*in = (HANDLE)temp;

	while (*params && *params != ',')
		params++;
	fail_if (!*params, 0, 0);
	params++;

	temp = atoi (params);
	fail_if (!temp, 0, 0);

	*out = (HANDLE)temp;

	while (*params && *params != ',')
		params++;
	fail_if (!*params, 0, 0);
	params++;

	temp = atoi (params);
	fail_if (!temp, 0, 0);

	*event = (HANDLE)temp;

	return 1;
failure:
	return st;
}


DWORD CALLBACK ConRunConsoleWatcher (void *param)
{
	HANDLE *handles = (HANDLE *)param;
	int res;

	res = WaitForMultipleObjects (2, handles, FALSE, INFINITE);

	ExitProcess (res == WAIT_OBJECT_0 ? CONSOLE_PARENT_EXITED : CONSOLE_EXIT_FORCED);

	return 0;
}

DWORD CALLBACK ConRunConsoleReader (void *param)
{
	HANDLE input = (HANDLE)param;
	HANDLE conout;
	char buffer[1024];
	DWORD read, writ;
	int res, st;

	conout = CreateFile ("CONOUT$", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	fail_if (conout == (HANDLE)-1, CONSOLE_OPEN_STDOUT_FAILED, DI (GetLastError ()));

	while (1)
	{
		res = ReadFile (input, buffer, 1024, &read, NULL);
		fail_if (!res, CONSOLE_READER_READ_FAILED, DI (GetLastError ()));

		res = WriteFile (conout, buffer, read, &writ, NULL);
		fail_if (!res || writ != read, CONSOLE_READER_WRITE_FAILED, DI (GetLastError ()));
	}

	//fall through, if execution ever gets here somehow

failure:
	ExitProcess (st);
	return 0;
}

DWORD CALLBACK ConRunConsoleWriter (void *param)
{
	HANDLE output = (HANDLE)param;
	//HANDLE conin;
	char buffer[1024];
	DWORD read, writ;
	int res, st;
	void *temp;

	temp = freopen ("CONIN$", "r", stdin);
	fail_if (!temp, CONSOLE_OPEN_STDIN_FAILED, 0);

	while (1)
	{
		temp = fgets (buffer, 1024, stdin);
		fail_if (!temp, CONSOLE_WRITER_READ_FAILED, 0);

		buffer[1023] = '\0';
		read = strlen (buffer);

		res = WriteFile (output, buffer, read, &writ, NULL);
		fail_if (!res || writ != read, CONSOLE_WRITER_WRITE_FAILED, DI (GetLastError ()));
	}


failure:
	ExitProcess (st);
	return 0;
}

//--runconsole process,input,output,event
//input is the one we'll read from, i.e. is is the OUTPUT pipe
//returns 1 if something fails; doesn't return otherwise
DERR ConRunDummyConsole (char *params)
{
	HANDLE process, in, out, thread, event;
	DWORD tid;
	int res, st;
	HANDLE blah[2];

	res = ConExtractParams (params, &process, &in, &out, &event);
	fail_if (!res, CONSOLE_ARGUMENT_ERROR, 0);

	res = AllocConsole ();
	fail_if (!res, CONSOLE_CONSOLE_ERROR, DI (GetLastError ()));

	blah[0] = process;
	blah[1] = event;

	thread = CreateThread (NULL, 0, ConRunConsoleWatcher, blah, 0, &tid);
	fail_if (!thread, CONSOLE_WATCHER_THREAD_ERROR, DI (GetLastError ()));
	CloseHandle (thread);

	thread = CreateThread (NULL, 0, ConRunConsoleReader, in, 0, &tid);
	fail_if (!thread, CONSOLE_READER_THREAD_ERROR, DI (GetLastError ()));
	CloseHandle (thread);

	//this thread becomes the writer
	res = ConRunConsoleWriter (out);

	//the function should never return, but just in case...
	st = CONSOLE_WRITER_THREAD_ERROR;

	//fall through
failure:
	ExitProcess (st);
	return 0;
}


/********************************************************************************************************

										Code that runs in the parent p5 process

********************************************************************************************************/


HANDLE ConChildProcess = NULL;
HANDLE ConInputPipe = NULL;
HANDLE ConOutputPipe = NULL;
HANDLE ConCancelPipe = NULL; //this is the write end of the input pipe, used to cancel IO
HANDLE ConStopEvent = NULL;
DWORD ConChildExitCode = 0;
DWORD ConConsoleIsInternal = 0;
int ConRunInMainThread = 1;


DWORD CALLBACK ConChildWatcherThread (void *param)
{
	int res;
	DWORD read;
	HANDLE temp;

	WaitForSingleObject (ConChildProcess, INFINITE);

	res = GetExitCodeProcess (ConChildProcess, &ConChildExitCode);
	if (!res)
		ConChildExitCode = CONSOLE_UNKNOWN_STATUS;

	//the child exit code needs to be nonzero before we write to the cancel pipe; that's
	//what the action thread looks for to decide when to exit

	read = 0;
	res = WriteFile (ConCancelPipe, &read, sizeof (DWORD), &read, NULL);

	//close the handles
	temp = ConInputPipe;
	ConInputPipe = NULL;
	CloseHandle (temp);

	temp = ConOutputPipe;
	ConOutputPipe = NULL;
	CloseHandle (temp);

	temp = ConCancelPipe;
	ConCancelPipe = NULL;
	CloseHandle (temp);

	temp = ConStopEvent;
	ConStopEvent = NULL;
	CloseHandle (temp);

	temp = ConChildProcess;
	ConChildProcess = NULL;
	CloseHandle (temp);

	return 0;
}

DERR ConWriteString (char *string)
{
	DWORD read;
	int res;
	int st;

	if (ConConsoleIsInternal)
	{
		printf ("%s", string);
	}
	else
	{
		fail_if (!ConOutputPipe, CONSOLE_NOT_OPEN, 0);
		res = WriteFile (ConOutputPipe, string, strlen (string), &read, NULL);
		fail_if (!res || read != strlen (string), CONSOLE_WRITE_FAILED, 0);
	}

	return PF_SUCCESS;
failure:
	return st;
}

DERR ConWriteStringf (char *format, ...)
{
	char buffer[1024];
	va_list marker;

	va_start (marker, format);
	_vsnprintf(buffer, 1023, format, marker);
	buffer[1023] = '\0';
	va_end (marker);

	return ConWriteString (buffer);
}


void ConStrip (char *a)
{
	while (*a && *a != '\r' && *a != '\n')
		a++;
	*a = '\0';
}

typedef struct
{
	int received;
	parser_t *par;
	char *buffer;
	char **output;
	int *nesting;
	char *error;
	int errlen;
} conmtrun_t;

DERR ConMainThreadRunParser (void *param)
{
	conmtrun_t *cmt = param;
	DERR st;

	st = ParRunLine (cmt->par, cmt->buffer, cmt->output, cmt->nesting, cmt->error, cmt->errlen);

	//set this so ConRUnParser can tell the difference betweeen a failure in 
	//ParRunLine and PfRunInMainThread.
	cmt->received = 1;
	/*st = InterlockedExchange (&cmt->received, 1);
	if (st == 2) //the main thread has aborted
	{
		if (cmt->output)
			ParFree (cmt->output);*/

	return st;
}

DERR ConRunParser (parser_t *par, char *buffer, char **output, int *nesting, char *error, int errlen)
{
	DERR st;
	conmtrun_t cmt;

	if (!ConRunInMainThread)
		return ParRunLine (par, buffer, output, nesting, error, errlen);

	cmt.received = 0;
	cmt.par = par;
	cmt.buffer = buffer;
	cmt.output = output;
	cmt.nesting = nesting;
	cmt.error = error;
	cmt.errlen = errlen;

	st = PfRunInMainThread (ConMainThreadRunParser, &cmt);

	if (!DERR_OK (st) && !cmt.received)
	{
		//an error occured in PfRunMainThread.  Simulate a normal parser failure.
		_snprintf (error, errlen - 1, "PfRunInMainThread error: {%s,%i}", PfDerrString (st), GETDERRINFO (st));
		error[errlen - 1] = '\0';

		*output = NULL; //this needs to be set
		*nesting = 0;
	}

	return st;
}
		



DWORD CALLBACK ConActionThread (void *param)
{
	char buffer[1024];
	int st, res;
	DWORD read;
	parser_t *par = NULL;
	int nesting = 0;
	char error[1024], *output;

	st = RopInherit (&par);
	fail_if (!DERR_OK (st), st, 0);

	while (1)
	{
		if (nesting)
			ConWriteStringf ("%i>> ", nesting);
		else
			ConWriteStringf ("%.4X%.4X> ", ParHeapCount (), PfHeapCount ());

		res = ReadFile (ConInputPipe, buffer, 1023, &read, NULL);
		fail_if (!res, CONSOLE_READ_FAILED, DI (GetLastError ()));
		buffer[read] = '\0';

		if (ConChildExitCode)
			break;

		ConStrip (buffer);

		if (!*buffer)
			continue;

		if (buffer[0] == '~' && buffer[1] == '~')
		{
			//signifies that the parser should be reset
			ParReset (par);
			nesting = 0;
			continue;
		}

		st = ConRunParser (par, buffer, &output, &nesting, error, 1024);
		if (!DERR_OK (st))
		{
			nesting = 0;
			ConWriteStringf ("ERROR: %s\n", error);
			if (output)
			{
				ConWriteString ("EXTRA ERROR!  Output set when !DERR_OK()  !");
				ParFree (output);
			}
			continue;
		}

		if (!nesting) //print "null" if 
		{
			ConWriteStringf ("result: %s\n", output);
			if (output)
				ParFree (output);
		}
		
	}

	ParDestroy (par);

	return PF_SUCCESS;

failure:

	if (par)
		ParDestroy (par);

	SetEvent (ConStopEvent);

	return st;
}
		

DERR ConOpenConsole (char *title, int color)
{
	SECURITY_ATTRIBUTES sa;
	HANDLE proself = NULL, proother = NULL, inputread = NULL;
	HANDLE inputwrite = NULL, outputread = NULL, outputwrite = NULL;
	char exename[MAX_PATH];
	char params[100];
	int st, res;
	STARTUPINFO si = {sizeof (si)};
	PROCESS_INFORMATION pi = {0};
	DWORD read;
	HANDLE event = NULL;
	HANDLE watcherthread = NULL, actionthread = NULL;

	fail_if (ConChildProcess, CONSOLE_ALREADY_OPEN, 0);
	fail_if (ConConsoleIsInternal, CONSOLE_ALREADY_OPEN, 0);

	sa.nLength = sizeof (sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE; //this is the important part

	//open this just so that the child can inherit it
	proself = OpenProcess (SYNCHRONIZE, TRUE, GetCurrentProcessId ());
	fail_if (!proself, CONSOLE_OPEN_SELF_ERROR, DIGLE);

	event = CreateEvent (&sa, TRUE, FALSE, NULL);
	fail_if (!event, CONSOLE_CREATE_EVENT_FAILED, DIGLE);

	res = CreatePipe (&inputread, &inputwrite, &sa, 0);
	fail_if (!res || !inputread || !inputwrite, CONSOLE_CREATE_PIPE_ERROR, DIGLE);

	res = CreatePipe (&outputread, &outputwrite, &sa, 0);
	fail_if (!res || !outputread || !outputwrite, CONSOLE_CREATE_PIPE_ERROR, DIGLE);

	res = GetModuleFileName (NULL, exename, MAX_PATH - 2);
	fail_if (!res, CONSOLE_GETFILENAME_ERROR, DIGLE);
	exename[MAX_PATH - 1] = '\0';

	_snprintf (params, 98, "mefoo.exe --runconsole %i,%i,%i,%i", proself, outputread, inputwrite, event);
	params[99] = '\0';

	si.lpTitle = title; //title can be NULL
	if (color)
		si.dwFillAttribute = color;

	res = CreateProcess (exename, params, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	fail_if (!res, CONSOLE_CHILD_CREATE_ERROR, DIGLE);

	//ok, the child is open, close some handles we don't need
	CloseHandle (proself);
	proself = NULL;
	CloseHandle (outputread);
	outputread = NULL;

	//set our globals
	ConChildProcess = pi.hProcess;
	ConInputPipe = inputread;
	ConOutputPipe = outputwrite;
	ConCancelPipe = inputwrite;
	ConStopEvent = event;
	ConChildExitCode = 0;

	//create the watcher and the action thread.
	watcherthread = CreateThread (NULL, 0, ConChildWatcherThread, NULL, 0, &read);
	fail_if (!watcherthread, CONSOLE_CREATE_WATCHER_FAILED, DIGLE);

	actionthread = CreateThread (NULL, 0, ConActionThread, NULL, 0, &read);
	fail_if (!actionthread, CONSOLE_CREATE_ACTION_FAILED, DIGLE);

	//ok. Jesus christ, I think we're done.
	CloseHandle (pi.hThread);
	CloseHandle (watcherthread);
	CloseHandle (actionthread);

	return PF_SUCCESS;
	
failure:

	if (proself)
		CloseHandle (proself);

	if (event)
		CloseHandle (event);

	if (inputread)
		CloseHandle (inputread);

	if (inputwrite)
		CloseHandle (inputwrite);

	if (outputread)
		CloseHandle (outputread);

	if (outputwrite)
		CloseHandle (outputwrite);

	if (pi.hProcess)
	{
		TerminateProcess (pi.hProcess, CONSOLE_STARTUP_FAILED);
		CloseHandle (pi.hProcess);
	}

	if (pi.hThread)
		CloseHandle (pi.hThread);

	if (watcherthread)
		CloseHandle (watcherthread); //it will exit because the process was terminated

	if (actionthread)
	{
		//better to leak a stack than leak a thread
		//this is the last thing to be done, so this should only
		//occur if the thread wasn't created anyway.
		TerminateThread (actionthread, 0);
		CloseHandle (actionthread);
	}

	//clear these out.
	ConChildProcess = NULL;
	ConInputPipe = NULL;
	ConOutputPipe = NULL;
	ConCancelPipe = NULL;
	ConStopEvent = NULL;
	ConChildExitCode = 0;

	return st;
}


DWORD CALLBACK ConDebugActionThread (void *param)
{
	char buffer[1024];
	int st;
	parser_t *par;
	int nesting;
	char error[1024];
	char *output;

	//debug console runs in own thread by default
	ConRunInMainThread = 0;


	st = RopInherit (&par);
	fail_if (!DERR_OK (st), st, 0);

	printf ("%.4X%.4X > ", ParHeapCount (), PfHeapCount ());

	while (fgets (buffer, 1024, stdin))
	{
		if (buffer[0] == '~' && buffer[1] == '~')
		{
			ParReset (par);
			printf ("%.4X%.4X> ", ParHeapCount (), PfHeapCount ());
			continue;
		}

		st = ConRunParser (par, buffer, &output, &nesting, error, 1024);
		if (!DERR_OK (st))
		{
			printf ("ERROR: %s\n%.4X%.4X> ", error, ParHeapCount (), PfHeapCount ());
			continue;
		}

		if (output)
		{
			printf ("res: %s\n", output);
			ParFree (output);
		}

		if (nesting)
			printf ("%i>>", nesting);
		else
			printf ("%.4X%.4X > ", ParHeapCount (), PfHeapCount ());
	}

	return PF_SUCCESS;
failure:
	printf ("Console error: 0x%.8X\n", st);
	return st;

}

//open console attached to this process
DERR ConOpenDebugConsole (void)
{
	int res;
	DERR st;
	HANDLE actionthread = NULL;
	DWORD tid;

	res = AllocConsole ();
	fail_if (!res, CONSOLE_CONSOLE_ERROR, DIGLE);

	fail_if (!freopen ("CONOUT$", "w", stdout), CONSOLE_FREOPEN_ERROR, 0);
	fail_if (!freopen ("CONOUT$", "w", stderr), CONSOLE_FREOPEN_ERROR, 0);
	fail_if (!freopen ("CONIN$", "r", stdin), CONSOLE_FREOPEN_ERROR, 0);

	actionthread = CreateThread (NULL, 0, ConDebugActionThread, NULL, 0, &tid);
	fail_if (!actionthread, CONSOLE_CREATE_ACTION_FAILED, DIGLE);

	ConConsoleIsInternal = 1;

	return PF_SUCCESS;

failure:
	if (actionthread) //this will have to change if more stuff is added above
		CloseHandle (actionthread);

	return st;
}

DERR ConCloseConsole (void)
{
	//this relies on the watcher thread to clean up.
	SetEvent (ConStopEvent);

	return PF_SUCCESS;
}

DERR ConForceCloseConsole (void)
{
	HANDLE temp;
	//this function will stop the console, or at minimum it will make it possible 

	SetEvent (ConStopEvent);

	TerminateProcess (ConChildProcess, CONSOLE_FORCE_CLOSED);

	//close the handles
	temp = ConInputPipe;
	ConInputPipe = NULL;
	CloseHandle (temp);

	temp = ConOutputPipe;
	ConOutputPipe = NULL;
	CloseHandle (temp);

	temp = ConCancelPipe;
	ConCancelPipe = NULL;
	CloseHandle (temp);

	temp = ConStopEvent;
	ConStopEvent = NULL;
	CloseHandle (temp);

	temp = ConChildProcess;
	ConChildProcess = NULL;
	CloseHandle (temp);

	ConChildExitCode = CONSOLE_FORCE_CLOSED;

	return PF_SUCCESS;
}
	


///////////////////////////  Script Commands /////////////////////////


void ConScriptOpen (parerror_t *pe)
{
	DERR st;

	st = ConOpenConsole ("p5 Console", 0);
	fail_if (!DERR_OK (st), st, PE ("ConOpenConsole returned {%s,%i}", PfDerrString (st), GETDERRINFO (st), 0, 0));
failure:
	return;
}


void ConScriptClose (parerror_t *pe)
{
	DERR st;

	st = ConCloseConsole ();
	fail_if (!DERR_OK (st), st, PE ("ConCloseConsole returned {%s,%i}", PfDerrString (st), GETDERRINFO (st), 0, 0));
failure:
	return;
}

void ConScriptForceClose (parerror_t *pe)
{
	DERR st;

	st = ConForceCloseConsole ();
	fail_if (!DERR_OK (st), st, PE ("ConForceCloseConsole returned {%s,%i}", PfDerrString (st), GETDERRINFO (st), 0, 0));
failure:
	return;
}

int ConScriptGetExitCode (void)
{
	return ConChildExitCode;
}

void ConScriptPrint (parerror_t *pe, char *line)
{
	DERR st;

	fail_if (!line, 0, PE_BAD_PARAM (1));

	st = ConWriteString (line);
	fail_if (!DERR_OK (st), st, PE ("ConWriteString returned {%s,%i}", PfDerrString (st), GETDERRINFO (st), 0, 0));
failure:
	return;
}

int ConSetMainThreadRun (int count, int newval)
{
	int out;

	if (count == 1)
		return ConRunInMainThread;

	out = ConRunInMainThread;
	ConRunInMainThread = newval;

	return out;
}



DERR ConAddGlobalCommands (parser_t *root)
{
	DERR st;

	st = ParAddFunction (root, "p5.con.open", "ve", ConScriptOpen, "Opens the p5 console", "(p5.con.open) = [v]", "Due to windows being incredibly stupid about its console handling, this will spawn a new process which allocates the console.  The only way to get the current process to open a console is to pass the --debugconsole parameter to the process when you start it.  Note that this will mean you have to always have the console open; if you click its close button, p5 will exit.  (This is the aforementioned stupidity that led to the external console in the first place.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "p5.con.close", "ve", ConScriptClose, "Closes the p5 console", "(p5.con.close) = [v]", "You should always use p5.con.close instead of p5.con.forceclose.  p5.con.forceclose may cause a resource leak and only exists in case a bug causes p5.con.close to not work.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "p5.con.forceclose", "ve", ConScriptForceClose, "Forcibly closes the p5 console", "(p5.con.forceclose) = [v]", "This function will terminate the child process and reset all state variables to their initial conditions.  After calling p5.con.forceclose, you should always be able to use p5.con.open to get the console back, however, calling p5.con.forceclose may have caused some resources to leak.  Use it only as a last resort.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "p5.con.exitcode", "i", ConScriptGetExitCode, "Gets the exit code of the p5 console process", "(p5.con.exitcode) = [i:exit code]", "The exit status of the console child process is saved and can be retrieved with this command.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "p5.con.print", "ves", ConScriptPrint, "Prints a string to the p5 console", "(p5.con.print [s:string]) = [v]", "Use (va) to get the string if you're looking for printf like semantics");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "p5.con.usemain", "ic", ConSetMainThreadRun, "Returns whether the console runs commands in the main thread.", "(p5.con.usemain) = [i:current value]", "By default, a normal console runs all of its commands in the main thread.  The debugging console defaults to running in its own thread.  The current behavior can be retrieved with this function.  Nonzero means the main thread is used, zero means a secondary thread is used.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.con.usemain", "ici", ConSetMainThreadRun, "Sets whether the console runs commands in the main thread.", "(p5.con.usemain [i:new value]) = [i:old value]", "By default, a normal console runs all of its commands in the main thread.  The debugging console defaults to running in its own thread.  The current behavior can be controlled with this function.  Nonzero means the main thread is used, zero means a secondary thread is used.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddAlias (root, "cprint", "ves", "p5.con.print");
	fail_if (!DERR_OK (st), st, 0);

	return PF_SUCCESS;
failure:
	return st;
}