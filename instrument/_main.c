/* Copyright 2006 wParam, licensed under the GPL */

#include "instrument.h"

HINSTANCE instance;
static DWORD dll_mode = MODE_UNKNOWN;
int SafeToUnload = 0;


int CheckMode (DWORD desiredmode)
{
	if (dll_mode != desiredmode)
		return 0;

	return 1;
}

int SetMode (DWORD mode)
{
	if (dll_mode != MODE_UNKNOWN)
		return 0;

	dll_mode = mode;
	return 1;
}

void SetDbLevel (int level)
{
	dblevel = level;
}

void CancelAgent (void)
{
	HANDLE event;

	instrumenting = 0;

	event = OpenEventf ("Instrument-pid-%i", GetCurrentProcessId ());
	fail_if (!event, (ERROR, "Could not get agent event (%i)\n", GetLastError ()));

	SetEvent (event);
	CloseHandle (event);

	SafeToUnload = 1;

	return;
failure:
	return;
}

int SmartIntConvert (char *string)
{
	if (strncmp (string, "0x", 2) == 0)
	{
		return strtoul (string + 2, NULL, 16);
	}
	else
	{
		return atoi (string);
	}
}

//////////required (duh)
BOOL __stdcall DllMain (HINSTANCE hInstance, DWORD reason, void *crap)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		HANDLE thread;
		DWORD tid;

		instance = hInstance;

		manager_mutex = CreateMutex (NULL, FALSE, "Instrument-manager-mutex");
		fail_if (!manager_mutex, (ERROR, "Failed to create manager mutex (%i)\n", GetLastError ()));
		msgbuf_mutex = CreateMutex (NULL, FALSE, "Instrument-msgbuf-mutex");
		fail_if (!msgbuf_mutex, (ERROR, "Failed to create msgbuf mutex (%i)\n", GetLastError ()));

		thread = CreateThread (NULL, 0, Instrument, NULL, 0, &tid);
		fail_if (!thread, (ERROR, "Could not create instrumentation thread, %i\n", GetLastError ()));
		CloseHandle (thread);
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		if (!SafeToUnload)
			return FALSE;

		CloseHandle (msgbuf_mutex);
		CloseHandle (manager_mutex);

	}

	return TRUE;
failure:
	return FALSE;
}



HANDLE manager_mutex = NULL;
HANDLE msgbuf_mutex = NULL;


/* NOTHING BUT SHARED VARIABLES BELOW THIS POINT! */
#ifdef SEVEN
#pragma section("shfoo", read, write, shared)
#else
#pragma data_seg("shfoo")
#endif

__declspec (allocate ("shfoo"))	int dblevel = NOISE;
__declspec (allocate ("shfoo")) int responsenumber = 12;
__declspec (allocate ("shfoo")) BYTE msgbuf[MAX_INSTRUMENT_SIZE] = {0};


/*

  TODO:

  Figure out how hooking of a process should work.  Does a 'hook' message type make sense?  
  (So it would be !instrument pid foo hook)

  More actions.  Custom dll calling actions.  Pixshell command actions.  Function call actions. Much goodness

  */