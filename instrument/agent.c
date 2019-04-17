/* Copyright 2006 wParam, licensed under the GPL */

#include <windows.h>
#include "instrument.h"

static BYTE AgnMessageBuffer[MAX_INSTRUMENT_SIZE] = {0};

int AgnAgentRunning = 1;



int AgnSendMessage (DWORD pid, message_t *message)
{
	HANDLE event = NULL, resp = NULL;
	int st;
	int havemutex = 0;
	int ack = 1; //whether we got a successful ack
	int managercreated = 0;

	fail_if (!IsmCheckMode (MODE_AGENT), 0, DP (ERROR, "MnSendMessage is for managers only\n"));

	//some sanity checks
	fail_if (message->size > MAX_INSTRUMENT_SIZE || message->magic != MAGIC_NUMBER,
		0, DP (ERROR, "Refusing to send malformed message (%X %i)\n", message->magic, message->size));

	IsmManagerMutex = CreateMutex (NULL, FALSE, "Instrument-manager-mutex");
	fail_if (!IsmManagerMutex, 0, DP (ERROR, "Could not create manager mutex, %i\n", GetLastError ()));
	managercreated = 1;

	message->pid = pid;

	//create the event to signal process
	//if it's not there, no sense going any farther
	event = UtilOpenEventf ("Instrument-pid-%i", pid);
	fail_if (!event, 0, DP (ERROR, "Could not find target event (%i)\n", GetLastError ()));

	//get manager mutex
	st = WaitForSingleObject (IsmManagerMutex, INFINITE);
	fail_if (st == WAIT_FAILED, 0, DP (ERROR, "Wait failure (%i)\n", GetLastError ()));
	havemutex = 1;

	message->event = IsmResponseNumber++;
	resp = UtilCreateEventf (NULL, FALSE, FALSE, "Instrument-event-%i", message->event);
	fail_if (!resp, 0, DP (ERROR, "Could not create response event (%i)\n", GetLastError ()));

	//grab the message buffer mutex and copy the data to shared memory
	st = WaitForSingleObject (IsmMessageMutex, INFINITE);
	fail_if (st == WAIT_FAILED, 0, DP (ERROR, "Wait failure (%i)\n", GetLastError ()));

	memcpy (IsmMessageBuffer, message, message->size);

	st = ReleaseMutex (IsmMessageMutex);
	fail_if (!st, 0, DP (ERROR, "ReleaseMutex failed (%i) (but the command was sent)\n", GetLastError (), 0, 0, 0));

	st = SetEvent (event);
	fail_if (!st, 0, DP (ERROR, "SetEvent failed (%i) (but command was sent)\n", GetLastError (), 0, 0, 0));

	//wait up to 2 seconds for the agent to respond
	st = WaitForSingleObject (resp, 2000);
	fail_if (st == WAIT_FAILED, 0, DP (ERROR, "Wait failure (%i) waiting for ack event\n", GetLastError (), 0, 0, 0));
	if (st == WAIT_TIMEOUT)
		ack = 0;

	//grab message buffer again and clear it for good measure.
	st = WaitForSingleObject (IsmMessageMutex, INFINITE);
	fail_if (st == WAIT_FAILED, 0, DP (ERROR, "Wait failure (%i)\n", GetLastError (), 0, 0, 0));

	memset (IsmMessageBuffer, 0, MAX_INSTRUMENT_SIZE);

	st = ReleaseMutex (IsmMessageMutex);
	fail_if (!st, 0, DP (ERROR, "ReleaseMutex failed (%i) after message was sent\n", GetLastError (), 0, 0, 0));

	//ok, we're done.
	st = ReleaseMutex (IsmManagerMutex);
	fail_if (!st, 0, DP (ERROR, "ReleaseMutex failed (%i) after message was sent\n", GetLastError (), 0, 0, 0));
	havemutex = 0;

	CloseHandle (resp);
	CloseHandle (event);

	CloseHandle (IsmManagerMutex);
	IsmManagerMutex = NULL;

	if (ack)
		return 1;

	return 2;

failure:

	if (resp)
		CloseHandle (resp);

	if (havemutex)
	{
		st = ReleaseMutex (IsmManagerMutex);
		log_if (!st, 0, DP (ERROR, "ReleaseMutex failed (%i)\n", GetLastError ()));
	}

	if (event)
		CloseHandle (event);

	if (managercreated)
	{
		CloseHandle (IsmManagerMutex);
		IsmManagerMutex = NULL;
	}

	return 0;
}








void AgnProcessMessage (message_t *message)
{
	//int res;

	switch (message->command)
	{
	case COMMAND_INSTRUMENT:
		InSetInstrument ((instrument_t *)(message->data + message->instrumentofs),
					   (action_t *)(message->data + message->actionsofs));
		break;

	case COMMAND_UNINSTRUMENT:
		InUnsetInstrument ((instrument_t *)(message->data + message->instrumentofs));
		break;

	case COMMAND_PRINT:
		AgnMessageBuffer[MAX_INSTRUMENT_SIZE - 1] = '\0';
		IsmDprintf (NOISE, "Print: %s\n", message->data);
		break;
	}

}


void AgnInfectNewProcess (HANDLE process)
{


}



 
DETOUR_TRAMPOLINE
(
	BOOL CreateProcessATramp(
	  LPSTR lpApplicationName,
	  LPSTR lpCommandLine,
	  LPSECURITY_ATTRIBUTES lpProcessAttributes,
	  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	  BOOL bInheritHandles,
	  DWORD dwCreationFlags,
	  LPVOID lpEnvironment,
	  LPSTR lpCurrentDirectory,
	  LPSTARTUPINFO lpStartupInfo,
	  LPPROCESS_INFORMATION lpProcessInformation),
	CreateProcessA
);

BOOL CreateProcessADetour(
	  LPSTR lpApplicationName,
	  LPSTR lpCommandLine,
	  LPSECURITY_ATTRIBUTES lpProcessAttributes,
	  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	  BOOL bInheritHandles,
	  DWORD dwCreationFlags,
	  LPVOID lpEnvironment,
	  LPSTR lpCurrentDirectory,
	  LPSTARTUPINFO lpStartupInfo,
	  LPPROCESS_INFORMATION lpProcessInformation)
{

	int doresume;
	int res, gle;

	if (dwCreationFlags & CREATE_SUSPENDED)
		doresume = 0;
	else
		doresume = 1;

	dwCreationFlags |= CREATE_SUSPENDED;

	res = CreateProcessATramp (lpApplicationName, lpCommandLine, lpProcessAttributes, 
		lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment,
		lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	gle = GetLastError ();

	if (res)
	{
		AgnInfectNewProcess (lpProcessInformation->hProcess);

		if (doresume)
			ResumeThread (lpProcessInformation->hThread);
	}

	SetLastError (gle);

	return res;
}



DETOUR_TRAMPOLINE
(
	BOOL CreateProcessWTramp(
	  LPWSTR lpApplicationName,
	  LPWSTR lpCommandLine,
	  LPSECURITY_ATTRIBUTES lpProcessAttributes,
	  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	  BOOL bInheritHandles,
	  DWORD dwCreationFlags,
	  LPVOID lpEnvironment,
	  LPWSTR lpCurrentDirectory,
	  LPSTARTUPINFO lpStartupInfo,
	  LPPROCESS_INFORMATION lpProcessInformation),
	CreateProcessW
);

BOOL CreateProcessWDetour(
	  LPWSTR lpApplicationName,
	  LPWSTR lpCommandLine,
	  LPSECURITY_ATTRIBUTES lpProcessAttributes,
	  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	  BOOL bInheritHandles,
	  DWORD dwCreationFlags,
	  LPVOID lpEnvironment,
	  LPWSTR lpCurrentDirectory,
	  LPSTARTUPINFO lpStartupInfo,
	  LPPROCESS_INFORMATION lpProcessInformation)
{
	int doresume;
	int res, gle;

	if (dwCreationFlags & CREATE_SUSPENDED)
		doresume = 0;
	else
		doresume = 1;

	dwCreationFlags |= CREATE_SUSPENDED;

	res = CreateProcessWTramp (lpApplicationName, lpCommandLine, lpProcessAttributes, 
		lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment,
		lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	gle = GetLastError ();

	if (res)
	{
		AgnInfectNewProcess (lpProcessInformation->hProcess);

		if (doresume)
			ResumeThread (lpProcessInformation->hThread);
	}

	SetLastError (gle);

	return res;

}

int AgnSetCpHooks (void)
{
	int st;
	int adone = 0;

	st = DetourFunctionWithTrampoline ((char *)CreateProcessATramp, (char *)CreateProcessADetour);
	fail_if (!st, 0, DP (ERROR, "Detour on CreateProcessA failed\n"));
	adone = 1;

	st = DetourFunctionWithTrampoline ((char *)CreateProcessWTramp, (char *)CreateProcessWDetour);
	fail_if (!st, 0, DP (ERROR, "Detour on CreateProcessW failed\n"));

	IsmDprintf (ERROR, "CreateProcess detours set\n");

	return 1;
failure:
	if (adone)
		DetourRemove ((char *)CreateProcessATramp, (char *)CreateProcessATramp);

	IsmDprintf (ERROR, "CreateProcess detours failed; infection will not occur\n");

	return 0;
}

//this is the DLL entry point for agent mode.  The rest of the DLL is
//un-initialized when it is called.
DWORD CALLBACK AgnInstrumentationListener (void *infection)
{
	HANDLE event = NULL;
	HANDLE resp;
	int res;
	int st;
	message_t *message = (message_t *)AgnMessageBuffer;

	fail_if (!IsmSetMode (MODE_AGENT), 0, DP (ERROR, "Agent thread could not set DLL to agent mode\n"));
	fail_if (!IsmCheckMode (MODE_AGENT), 0, DP (ERROR, "Agent thread cannot run if DLL is not in agent mode.\n"));

	IsmMessageMutex = CreateMutex (NULL, FALSE, "Instrument-msgbuf-mutex");
	fail_if (!IsmMessageMutex, 0, DP (ERROR, "Could not create/open message buffer mutex, %i\n", GetLastError ()));

	event = UtilCreateEventf (NULL, FALSE, FALSE, "Instrument-pid-%i", GetCurrentProcessId ());
	fail_if (!event, 0, DP (ERROR, "Count not create instrument event (%i); quitting\n", GetLastError ()));

	if (infection)
		AgnSetCpHooks ();


	while (AgnAgentRunning)
	{
		res = WaitForSingleObject (event, INFINITE);
		fail_if (res == WAIT_FAILED, 0, DP (ERROR, "Wait failure (%i)\n", GetLastError ()));

		if (!AgnAgentRunning)
			break;

		//ok.  manager has work for us
		res = WaitForSingleObject (IsmMessageMutex, INFINITE);
		fail_if (res == WAIT_FAILED, 0, DP (ERROR, "Wait failure (%i)\n", GetLastError ()));

		memcpy (message, IsmMessageBuffer, MAX_INSTRUMENT_SIZE);

		res = ReleaseMutex (IsmMessageMutex);
		log_if (!res, 0, DP (ERROR, "ReleaseMutex failure (%i)\n", GetLastError ()));

		//basic checks
		if (message->magic != MAGIC_NUMBER || 
			message->pid != GetCurrentProcessId () ||
			message->size > MAX_INSTRUMENT_SIZE)
		{
			IsmDprintf (WARN, "Invalid message received (%X %i %i)\n", message->magic, message->pid, message->size);
			continue;
		}

		//signal the manager that we've got the message
		resp = UtilCreateEventf (NULL, FALSE, FALSE, "Instrument-event-%i", message->event);
		if (resp)
		{
			SetEvent (resp);
			CloseHandle (resp);
		}
		else
		{
			IsmDprintf (WARN, "Could not get response event %i (%i)\n", message->event, GetLastError ());
		}

		//ok.  We've done all we need to do with respect to the manager

		AgnProcessMessage (message);

	}

	CloseHandle (event);
	return 0;

failure:
	if (event)
		CloseHandle (event);

	if (IsmMessageMutex)
	{
		CloseHandle (IsmMessageMutex);
		IsmMessageMutex = NULL;
	}

	return 1;

}