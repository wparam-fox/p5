/* Copyright 2006 wParam, licensed under the GPL */

#include <windows.h>
#include "instrument.h"

#include <stdio.h>


//This variable controls whether we are running as a plugin (i.e. don't allow any
//hooking, and use PlMalloc/PlFree), or running wild in some hostile process (i.e.
//hook away, don't allow unload, use the process heap for allocation.
static int IsmIsPlugin = 0;

static DWORD IsmDllMode = MODE_UNKNOWN;

int IsmCheckIsPlugin (void)
{
	return IsmIsPlugin;
}

int IsmCheckMode (DWORD desiredmode)
{
	if (IsmDllMode != desiredmode)
		return 0;

	return 1;
}

int IsmSetMode (DWORD mode)
{
	if (IsmDllMode != MODE_UNKNOWN)
		return 0;

	IsmDllMode = mode;
	return 1;
}

void IsmSetPluginMode (void)
{
	IsmIsPlugin = 1;
}


extern int IsmDebugLevel; //not really external; it has to be at the bottom of this file
void IsmSetDbLevel (int level)
{
	IsmDebugLevel = level;
}

void *IsmMalloc (int size)
{
	if (IsmIsPlugin)
		return PlMalloc (size);
	else
		return HeapAlloc (GetProcessHeap (), 0, size);
}

void IsmFree (void *block)
{
	if (IsmIsPlugin)
		PlFree (block);
	else
		HeapFree (GetProcessHeap (), 0, block);
}

void *IsmRealloc (void *block, int size)
{
	if (IsmIsPlugin)
		return PlRealloc (block, size);
	else
		return HeapReAlloc (GetProcessHeap (), 0, block, size);
}

extern int IsmDebugLevel;

void IsmDprintf (int level, char *format, ...)
{
	char buffer[1024];

	if (level <= IsmDebugLevel)
	{
		
		va_list marker;
		va_start (marker, format);
		
		_vsnprintf (buffer, 1024, format, marker);
		buffer[1023] = '\0';


		OutputDebugString (buffer);

		va_end (marker);
	}


}


HANDLE IsmMessageMutex = NULL;
HANDLE IsmManagerMutex = NULL;

/* NOTHING BUT SHARED VARIABLES BELOW THIS POINT! */
#ifdef SEVEN
#pragma section("shfoo", read, write, shared)
#else
#pragma data_seg("shfoo")
#endif

__declspec (allocate ("shfoo"))	int IsmDebugLevel = NOISE;
__declspec (allocate ("shfoo")) int IsmResponseNumber = 12;
__declspec (allocate ("shfoo")) BYTE IsmMessageBuffer[MAX_INSTRUMENT_SIZE] = {0};


/*

  TODO:

  Figure out how hooking of a process should work.  Does a 'hook' message type make sense?  
  (So it would be !instrument pid foo hook)

  More actions.  Custom dll calling actions.  Pixshell command actions.  Function call actions. Much goodness

  */


