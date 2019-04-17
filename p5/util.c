/* Copyright 2013 wParam, licensed under the GPL */

#include <windows.h>
#include "p5.h"
#include <stdio.h>
#include "..\common\p5lib.h"

#define UTIL_LINE_TOO_LONG				DERRCODE (UTIL_DERR_BASE, 0x01)
#define UTIL_INVALID_PARAM				DERRCODE (UTIL_DERR_BASE, 0x02)
#define UTIL_OUT_OF_MEMORY				DERRCODE (UTIL_DERR_BASE, 0x03)
#define UTIL_NOTHING_TO_DO				DERRCODE (UTIL_DERR_BASE, 0x04)
#define UTIL_UNMATCHED_QUOTE			DERRCODE (UTIL_DERR_BASE, 0x05)
#define UTIL_SHELLEXECUTE_FAILED		DERRCODE (UTIL_DERR_BASE, 0x06)
#define UTIL_REGOPENKEY_FAILED			DERRCODE (UTIL_DERR_BASE, 0x07)
#define UTIL_REGSETVALUE_FAILED			DERRCODE (UTIL_DERR_BASE, 0x08)
#define UTIL_P3WINDOW_NOT_FOUND			DERRCODE (UTIL_DERR_BASE, 0x09)
char *UtilErrors[] = 
{
	NULL,
	"UTIL_LINE_TOO_LONG",
	"UTIL_INVALID_PARAM",	
	"UTIL_OUT_OF_MEMORY",	
	"UTIL_NOTHING_TO_DO",	
	"UTIL_UNMATCHED_QUOTE",
	"UTIL_SHELLEXECUTE_FAILED",
	"UTIL_REGOPENKEY_FAILED",
	"UTIL_REGSETVALUE_FAILED",
	"UTIL_P3WINDOW_NOT_FOUND",
};

char *UtilGetErrorString (int errindex)
{
	int numerrors = sizeof (UtilErrors) / sizeof (char *);

	if (errindex >= numerrors || !UtilErrors[errindex])
		return "UTIL_UNKNOWN_ERROR_CODE";

	return UtilErrors[errindex];
}



DERR UtilProperShell (char *line)
{
	char *buffer = NULL;
	char *dir = NULL;
	int len;
	char *walk;
	char *app;
	char *params;
	DERR st;
	SHELLEXECUTEINFO sex = {0};

	fail_if (!line, UTIL_INVALID_PARAM, DI (1));

	len = strlen (line) + 1;
	
	//copy the string so that we can edit it.
	buffer = PfMalloc (len);
	fail_if (!buffer, UTIL_OUT_OF_MEMORY, DI (len));
	strcpy (buffer, line);

	//first identify the app and the params.
	walk = buffer;
	while (*walk && (*walk == '\t' || *walk == ' ' || *walk == '\r' || *walk == '\n'))
		walk++;
	fail_if (!*walk, UTIL_NOTHING_TO_DO, 0);

	if (*walk == '\"') //if they quoted the exe, then we're searching for a quote, and we need to skip the first quote
	{
		walk++;
		app = walk;
		while (*walk && *walk != '\"')
			walk++;
		fail_if (!*walk, UTIL_UNMATCHED_QUOTE, 0);

		//null out the quote to make app a proper string, and pass whitespace to get to the params
		*walk = '\0';
		walk++;
		while (*walk && (*walk == '\t' || *walk == ' ' || *walk == '\r' || *walk == '\n')) //skip any whitespace after the quote
			walk++;
		
		params = walk; //params may be an empty string
	}
	else
	{
		app = walk;
		while (*walk && (*walk != '\t' && *walk != ' ' && *walk != '\r' && *walk != '\n'))
			walk++;

		if (*walk) //we have params
		{
			*walk = '\0';
			walk++;
			while (*walk && (*walk == '\t' || *walk == ' ' || *walk == '\r' || *walk == '\n')) //skip any whitespace after the quote
				walk++;
			params = walk;
		}
		else
		{
			//no params, the string is just the exe file.  Nothing to do, set params to walk, which is ""
			params = walk;
		}
	}

	//we have app and params.  Now, split off the directory.
	len = strlen (app) + 1;
	dir = PfMalloc (len);
	fail_if (!dir, UTIL_OUT_OF_MEMORY, DI (len));
	strcpy (dir, app);

	walk = dir + strlen (dir) - 1;
	while (walk >= dir)
	{
		if (*walk == '\\')
		{
			*walk = '\0';
			break;
		}
		walk--;
	}
	if (walk < dir) //no \\ found
	{
		PfFree (dir);
		dir = NULL;
	}

	//ok, we have our info.  RUn it.

	sex.cbSize = sizeof (SHELLEXECUTEINFO);
	sex.fMask = SEE_MASK_FLAG_NO_UI;
	sex.lpFile = app;
	sex.lpDirectory = dir;
	sex.lpParameters = params;
	sex.nShow = SW_SHOW;

	st = ShellExecuteEx (&sex);
	fail_if (!st, UTIL_SHELLEXECUTE_FAILED, DIGLE);

	if (dir)
		PfFree (dir);

	PfFree (buffer);

	return PF_SUCCESS;

failure:
	if (dir)
		PfFree (dir);

	if (buffer)
		PfFree (buffer);

	return st;
}

#define WM_PARSEINDIRECT (WM_USER + 11)
DERR UtilPixshellThreeRun (char *command, int async)
{
	HKEY key = NULL;
	HWND hwnd;
	DERR st;
	int res;

	fail_if (!command, UTIL_INVALID_PARAM, DI (1));

	res = RegOpenKeyEx (HKEY_CURRENT_USER, "software\\argus\\pixshell", 0, KEY_ALL_ACCESS, &key);
	fail_if (res != ERROR_SUCCESS, UTIL_REGOPENKEY_FAILED, DI (res));

	res = RegSetValueEx (key, "ParseString", 0, REG_SZ, command, strlen (command) + 1);
	fail_if (res != ERROR_SUCCESS, UTIL_REGSETVALUE_FAILED, DI (res));

	RegCloseKey (key);
	key = NULL;

	hwnd = FindWindow ("Pixshell_Parser", NULL);
	fail_if (!hwnd, UTIL_P3WINDOW_NOT_FOUND, DIGLE);

	if (async)
		PostMessage (hwnd, WM_PARSEINDIRECT, 0, 0);
	else
		SendMessage (hwnd, WM_PARSEINDIRECT, 0, 0);

	return PF_SUCCESS;
failure:

	if (key)
		RegCloseKey (key);

	return st;
}
	


///////////////////////// TYPE manipulation functions /////////////////////////
/// Called from script without actually /being/ script

char *UtilLongIntToType (parerror_t *pe, __int64 in)
{
	char *out = NULL;
	int len;
	DERR st;

	//assume that we'll need the maximum characters--4 + 16 + 1 (i64:123456789ABCDEF0)
	//and add a few just in case.

	len = 22;

	out = ParMalloc (len + 1);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (len + 1));

	_snprintf (out, len, "i64:%I64X", in);
	out[len] = '\0'; //safety null

	return out;
failure:

	if (out)
		ParFree (out);

	return NULL;
}

int UtilTypeToLongInt (parerror_t *pe, char *in, __int64 *output)
{
	DERR st;
	__int64 out;
	int val;

	//check the type;
	fail_if (strncmp (in, "i64:", 4) != 0, 0, PE ("Type mismatch: Not an int64", 0, 0, 0, 0));

	out = 0;

	in += 4;
	while (*in)
	{
		if (*in >= '0' && *in <= '9')
			val = *in - '0';
		else if (*in >= 'A' && *in <= 'F')
			val = *in - 'A' + 10;
		else if (*in >= 'a' && *in <= 'f')
			val = *in - 'a' + 10;
		else
			break;

		out <<= 4;
		out += val;

		in++;
	}

	*output = out;

	return 1;
failure:
	return 0;
}


static char UtilBinaryChars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

char *UtilBinaryToType (parerror_t *pe, void *voidin, int inlen, char *typename)
{
	unsigned char *walk;
	unsigned char *in = voidin;
	char *out = NULL;
	int len;
	DERR st;
	int x;

	len = strlen (typename) + 1 + inlen * 2 + 1; //[typename] : [data] \0

	out = ParMalloc (len);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (len));

	strcpy (out, typename);
	strcat (out, ":");

	walk = out + strlen (out);
	fail_if (*walk != '\0', 0, PE ("Sanity failure in ConvertBinaryToType", 0, 0, 0, 0));

	for (x=0;x<inlen;x++)
	{
		*(walk++) = UtilBinaryChars[((*in) >> 4) & 0xF];
		*(walk++) = UtilBinaryChars[(*in) & 0xF];

		in++;
	}

	*walk = '\0';

	fail_if (walk - out != len - 1, 0, PE ("Sanity failure 2 in ConvertBinaryToType", 0, 0, 0, 0));

	return out;
failure:
	if (out)
		ParFree (out);
	return NULL;

}


static int UtilTypeCheck (char *type, char *expected)
{
	while (*type && *expected)
	{
		if (*(type++) != *(expected++))
			return 0;
	}

	//one of them has run out of characters.  The only case where it is a type match
	//is when *type is the colon separating name from data and *expected is \0.

	if (*type == ':' && *expected == '\0')
		return 1;

	return 0;
}

static int UtilCharValue (char in)
{
	if (in >= '0' && in <= '9')
		return in - '0';
	if (in >= 'A' && in <= 'F')
		return in - 'A' + 10;
	if (in >= 'a' && in <= 'f')
		return in - 'a' + 10;

	return -1; //error
}

int UtilTypeToBinary (parerror_t *pe, char *type, char *typeexpected, int *lenout, void *voiddata, int datalen)
{
	DERR st;
	int len;
	int x;
	int val1, val2;
	unsigned char *data = voiddata;

	fail_if (!UtilTypeCheck (type, typeexpected), 0, PE_STR ("Type mismatch: Passed object is not of type %s", COPYSTRING (typeexpected), 0, 0, 0));

	//advance type past the typename to the data.
	//we know this works because we know that the beginning of type matches
	//typestring followed by a colon.  (Otherwise UtilTypeCheck would have
	//failed.)
	type += strlen (typeexpected) + 1;

	len = strlen (type);
	//fail if len is odd
	fail_if (len & 1, 0, PE ("Type string is invalid: Odd number of data characters (%i)", len, 0, 0, 0));

	len /= 2; //now len will be the length of the output data.

	//if they didn't ask for the data, just return them the length and exit.
	//This can be used to fail before data is allocated if the expected size
	//of the data is known in advance.
	if (!data)
	{
		*lenout = len;
		return 1;
	}

	//make sure we have buffer space
	fail_if (len > datalen, 0, PE ("Output buffer for UtilTypeToBinary too small (%i, should be >= %i", datalen, len, 0, 0));

	//ok, so do the conversion.
	for (x=0;x<len;x++)
	{
		val1 = UtilCharValue (type[x * 2]);
		val2 = UtilCharValue (type[(x * 2) + 1]);
		fail_if (val1 == -1, 0, PE ("Invalid characted %c detected in string", type[x * 2], 0, 0, 0));
		fail_if (val2 == -1, 0, PE ("Invalid characted %c detected in string", type[(x * 2) + 1], 0, 0, 0));

		data[x] = ((val1 << 4) & 0xF0) | (val2 & 0xF);
	}

	//success.
	*lenout = len;

	return 1;
failure:


	return 0;
}





/////////////////////////////// script functions //////////////////////////////

void UtilScriptProperShell (parerror_t *pe, char *line)
{
	DERR st;

	fail_if (!line, 0, PE_BAD_PARAM (1));

	st = UtilProperShell (line);
	fail_if (!DERR_OK (st), st, PE ("UtilProperShell failed, {%s,%i}", PfDerrString (st), GETDERRINFO (st), 0, 0));

failure:
	return;
}





void UtilScriptPixshellThreeRun (parerror_t *pe, char *command)
{
	DERR st;

	fail_if (!command, 0, PE_BAD_PARAM (1));

	st = UtilPixshellThreeRun (command, 0);
	fail_if (!DERR_OK (st), st, PE ("Could not complete p3 run, {%s,%i}", PfDerrString (st), GETDERRINFO (st), 0, 0));

failure:
	return;

}

void UtilScriptPixshellThreeRunAsync (parerror_t *pe, char *command)
{
	DERR st;

	fail_if (!command, 0, PE_BAD_PARAM (1));

	st = UtilPixshellThreeRun (command, 1);
	fail_if (!DERR_OK (st), st, PE ("Could not complete p3 run, {%s,%i}", PfDerrString (st), GETDERRINFO (st), 0, 0));

failure:
	return;
}

char *UtilStrcat (parerror_t *pe, char *p1, char *p2)
{
	DERR st;
	char *out = NULL;

	//strings are allowed to be null, because I feel like it.
	if (!p1)
		p1 = "";
	if (!p2)
		p2 = "";

	st = strlen (p1) + strlen (p2) + 1;

	out = ParMalloc (st);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (st));

	strcpy (out, p1);
	strcat (out, p2);

	return out;

failure:

	if (out)
		ParFree (out);

	return NULL;
}


char *UtilStrcatV (parerror_t *pe, int count, ...)
{
	va_list va;
	int inva = 0;
	int numparams = count - 2;
	int len;
	int x;
	char *param;
	DERR st;
	char *out = NULL;


	//first step is to make sure all the params form valid strings.
	__try
	{
		va_start (va, count);
		inva = 1;
		for (x=0;x<numparams;x++)
		{
			param = va_arg (va, char *);
			if (!param)
				param = "";

			fail_if ((((int)param) & 0xFFFF0000) == 0, 0, PE ("Parameter %i invalid. (not a string)", x + 1, 0, 0, 0));
			strlen (param); //should throw an exception if it's invalid
		}
		va_end (va);
		inva = 0;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		fail_if (TRUE, 0, PE ("Parameter %i to (strcatv) invalid. (exception on access)", x + 1, 0, 0, 0));
	}

	//now get the total length
	va_start (va, count);
	inva = 1;

	len = 0;
	for (x=0;x<numparams;x++)
	{
		param = va_arg (va, char *);
		if (!param)
			param = "";

		len += strlen (param);
	}

	va_end (va);
	inva = 0;


	len++; //add 1 for the null terminator

	out = ParMalloc (len);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (len));


	//copy the strings in
	va_start (va, count);
	inva = 1;

	*out = '\0';
	for (x=0;x<numparams;x++)
	{
		param = va_arg (va, char *);
		if (!param)
			param = "";

		strcat (out, param);
	}

	va_end (va);
	inva = 0;

	return out;

failure:
	if (inva)
		va_end (va);

	if (out)
		ParFree (out);

	return NULL;
}


int UtilGetTid (char *in)
{
	if (!in)
		return 0;

	while (*in && *in != ' ')
		in++;
	if (*in)
		in++;

	return atoi (in);
}

int UtilGetPid (char *in)
{
	if (!in)
		return 0;
	return atoi (in);
}

char *UtilCreateProcess (parerror_t *pe, char *app, char *cmdline, int inherit, int suspended, char *dir)
{
	char *out = NULL;
	PROCESS_INFORMATION pi;
	STARTUPINFO si = {sizeof (si)};
	int res;
	char buffer[40];
	DERR st;

	res = CreateProcess (app, cmdline, NULL, NULL, inherit, suspended ? CREATE_SUSPENDED : 0, NULL, dir, &si, &pi);
	fail_if (!res, 0, PE ("CreateProcess failed, %i", GetLastError (), 0, 0, 0));
	
	CloseHandle (pi.hProcess);
	CloseHandle (pi.hThread);

	_snprintf (buffer, 39, "%i %i", pi.dwProcessId, pi.dwThreadId);
	buffer[39] = '\0';

	out = ParMalloc (strlen (buffer) + 1);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (strlen (buffer) + 1));
	strcpy (out, buffer);

	return out;
failure:
	return NULL;
}

char *UtilCreateProcessDesktop (parerror_t *pe, char *app, char *cmdline, int inherit, int suspended, char *dir, char *desktop)
{
	char *out = NULL;
	PROCESS_INFORMATION pi;
	STARTUPINFO si = {sizeof (si)};
	int res;
	char buffer[40];
	DERR st;

	si.lpDesktop = desktop;

	res = CreateProcess (app, cmdline, NULL, NULL, inherit, suspended ? CREATE_SUSPENDED : 0, NULL, dir, &si, &pi);
	fail_if (!res, 0, PE ("CreateProcess failed, %i", GetLastError (), 0, 0, 0));
	
	CloseHandle (pi.hProcess);
	CloseHandle (pi.hThread);

	_snprintf (buffer, 39, "%i %i", pi.dwProcessId, pi.dwThreadId);
	buffer[39] = '\0';

	out = ParMalloc (strlen (buffer) + 1);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (strlen (buffer) + 1));
	strcpy (out, buffer);

	return out;
failure:
	return NULL;
}

void UtilTerminateProcess (parerror_t *pe, int pid, int exitcode)
{
	DERR st;
	HANDLE proc = NULL;

	proc = OpenProcess (PROCESS_TERMINATE, FALSE, pid);
	fail_if (!proc, 0, PE ("OpenProcess failed, %i", GetLastError(), 0, 0, 0));

	st = TerminateProcess (proc, exitcode);
	fail_if (!st, 0, PE ("TerminateProcess failed, %i", GetLastError(), 0, 0, 0));

	CloseHandle (proc);

	return;
failure:
	if (proc)
		CloseHandle (proc);
	return;
}

//Sends a command to a given p5 instance
void UtilSendMailCommand (parerror_t *pe, char *pipename, char *line)
{
	DERR st;

	fail_if (!pipename, 0, PE_BAD_PARAM (1));

	st = PfSendMailCommand (pipename, line);
	fail_if (!DERR_OK (st), st, PE ("PfSendMailCommand returned {%s,%i}", PfDerrString (st), GETDERRINFO (st), 0, 0));

	return;
failure:
	return;
}


//desktop can be a NULL string to mean the current one.
char *UtilGetDefaultPipeName (parerror_t *pe, char *desktop)
{
	DERR st;
	char *pipename = NULL;
	int pipelen;

	st = PfGetDefaultPipeName (desktop ? NULL : GetThreadDesktop (GetCurrentThreadId ()), desktop, NULL, &pipelen);
	fail_if (!DERR_OK (st), st, PE ("PfGetDefaultPipeName returned failure, {%s,%i}", PfDerrString (st), GETDERRINFO (st), 0, 0));

	//allocate it on the parser heap so we can just return it.
	pipename = ParMalloc (pipelen);
	fail_if (!pipename, 0, PE_OUT_OF_MEMORY (pipelen));

	st = PfGetDefaultPipeName (desktop ? NULL : GetThreadDesktop (GetCurrentThreadId ()), desktop, pipename, &pipelen);
	fail_if (!DERR_OK (st), st, PE ("PfGetDefaultPipeName returned failure, {%s,%i}", PfDerrString (st), GETDERRINFO (st), 0, 0));

	//ok, should be done.
	return pipename;

failure:
	if (pipename)
		ParFree (pipename);

	return NULL;
}


char *UtilSubstring (parerror_t *pe, char *string, int start, int len)
{
	char *out = NULL;
	DERR st;

	fail_if (!string, 0, PE_BAD_PARAM (1));
	fail_if (start < 0, 0, PE_BAD_PARAM (2));
	fail_if (len < -1, 0, PE_BAD_PARAM (3));

	if ((unsigned)start > strlen (string))
		start = strlen (string);

	if (len == -1)
		len = strlen (string + start) + 2;

	out = ParMalloc (len + 2);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (len + 2));

	strncpy (out, string + start, len);
	out[len] = '\0';

	return out;
failure:
	if (out)
		ParFree (out);
	return NULL;
}

char *UtilUppercase (parerror_t *pe, char *string)
{
	char *out = NULL;
	DERR st;

	if (!string)
		return NULL; //this string was easy to convert.

	st = strlen (string);
	out = ParMalloc (st + 1);
	fail_if (!out, st, PE_OUT_OF_MEMORY (st + 1));
	strcpy (out, string);

	CharUpper (out);

	return out;
failure:
	if (out)
		ParFree (out);
	return NULL;
}

char *UtilLowercase (parerror_t *pe, char *string)
{
	char *out = NULL;
	DERR st;

	if (!string)
		return NULL; //this string was easy to convert.

	st = strlen (string);
	out = ParMalloc (st + 1);
	fail_if (!out, st, PE_OUT_OF_MEMORY (st + 1));
	strcpy (out, string);

	CharLower (out);

	return out;
failure:
	if (out)
		ParFree (out);
	return NULL;
}




DERR UtilAddGlobalCommands (parser_t *root)
{
	DERR st;

	st = ParAddFunction (root, "p5.util.p3runsync", "ves", UtilScriptPixshellThreeRun, "Runs a command in old pixshell, does not return until the command completes.", "(p5.util.p3runsync [s:command]) = [v]", "The string passed in is send directly to old pixshell for processing, it is parsed just as if it was typed in the old runbox.  This command doesn't return until the parse has finished, meaning that p5 will be blocked if, for example, the command brings up a message box.  This command should not be run in the main thread.  p5.util.p3run should be used whenever possible.  Old pixshell does not return information about success or failure anyway, so waiting will not necessarily be useful.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.util.p3run", "ves", UtilScriptPixshellThreeRunAsync, "Runs a command in old pixshell, always returns immediately.", "(p5.util.p3run [s:command]) = [v]", "The string passed in is send directly to old pixshell for processing, it is parsed just as if it was typed in the old runbox.  This command uses PostMessage and returns immediately.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "p5.util.strcat", "sess", UtilStrcat, "Concatenates two strings", "(p5.util.strcat [s:string 1] [s:string 2]) = [s:string 1 + string 2]", "(p5.util.strcat \"foo\" \"bar\") returns \"foobar\"");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.util.strcatv", "sec.", UtilStrcatV, "Concatenates any number of strings", "(p5.util.strcatv [s:string 1] [s:string 2] ...) = [s:string 1 + string 2 + ...]", "This function can be called with any number of strings.  (p5.util.strcatv) returns an empty string.  This function also accepts atoms as parameters (quoted or otherwise).");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.util.substr", "sesii", UtilSubstring, "Returns a substring of a given string", "(p5.util.substr [s:string] [i:start position] [i:length]) = [s:sub string]", "The start position is a zero based index into the string, and length is in characters.  To return the entire string after the start position, specify -1 for the length.  For example, (p5.util.substr \"Random Example\" 2 3) returns \"ndo\".  If the starting position is beyond the end of the string, an empty string will be returned.  Neither start nor len may be negative, except for the special case of -1 mentioned previously for len.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.util.toupper", "ses", UtilUppercase, "Converts a string to uppercase", "(p5.util.toupper [s:sTriNg]) = [s:STRING]", "The returned string is identical to the input string with all characters converted to uppercase.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.util.tolower", "ses", UtilLowercase, "Converts a string to lowercase", "(p5.util.tolower [s:sTriNg]) = [s:string]", "The returned string is identical to the input string with all characters converted to lowercase.");
	fail_if (!DERR_OK (st), st, 0);


	st = ParAddFunction (root, "p5.util.propershell", "ves", UtilScriptProperShell, "Runs a program", "(p5.util.propershell [s:command line]) = [v]", "The program is run with the working directory set to whatever directory the exe file is in.  If you don't give a path to the exe, it is run in p5's current directory (whatever that is).  If you need more control over how the program is run, use one of the p5.util.cp functions.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.util.cp", "sessiis", UtilCreateProcess, "Calls CreateProcess()", "(p5.util.cp [s:app] [s:command line] [i:inherit handles] [i:create suspended] [s:directory]) = [s:pidtid]", "Use (pid) and (tid) to split the returned pidtid into its component parts.  See MSDN for documentation about the parameters to CreateProcess ()");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.util.cpdesk", "sessiiss", UtilCreateProcessDesktop, "Calls CreateProcess() with a desktop set in STARTUPINFO", "(p5.util.cpdesk [s:app] [s:command line] [i:inherit handles] [i:create suspended] [s:directory] [s:desktop]) = [s:pidtid]", "Use (pid) and (tid) to split the returned pidtid into its component parts.  See MSDN for documentation about the parameters to CreateProcess ()");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.util.terminate", "veii", UtilTerminateProcess, "Terminates a running program", "(p5.util.terminate [i:pid] [i:exitcode]) = [v]", "Calls TerminateProcess on the given process id.  The exit code is passed to the API; a good default to use is 1 if you don't care.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "p5.util.pid", "is", UtilGetPid, "Gets the pid from a pid/tid pair", "(p5.util.pid [s:pidtid string]) = [i:pid]", "pidtid's are returned by the create process functions.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.util.tid", "is", UtilGetTid, "Gets the tid from a pid/tid pair", "(p5.util.tid [s:pidtid string]) = [i:tid]", "pidtid's are returned by the create process functions.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "p5.util.p5run", "vesa", UtilSendMailCommand, "Sends a p5 expression to another instance of p5 for execution.", "(p5.util.p5run [s:mailslot name] [a:expression]) = [v]", "Use p5.util.slotname to get the default mailslot name used by a p5 running on a given desktop.  This command is intended to be used to get hotkeys to always run on the desktop the user is working with.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.util.slotname", "ses", UtilGetDefaultPipeName, "Gets the default pipename for a given desktop name.", "(p5.util.slotname [s:desktop name]) = [s:result]", "The result of this function can be used in a call to p5.util.p5run.  Note that if you've changed the mailslot name with the --mailslotname command line parameter, this command will be completely useless.  Intended to work with multiple desktop setups.  The desktop name can be NULL if you want it to return the slot name for the current desktop.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddAlias (root, "p3", "ves", "p5.util.p3run");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "ss", "sess", "p5.util.strcat");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "ss.", "sec.", "p5.util.strcatv");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "substr", "sesii", "p5.util.substr");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddAlias (root, "pid", "is", "p5.util.pid");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "tid", "is", "p5.util.tid");
	fail_if (!DERR_OK (st), st, 0);


	return PF_SUCCESS;

failure:
	return st;
}
