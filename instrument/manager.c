/* Copyright 2006 wParam, licensed under the GPL */

#include <windows.h>
#include "instrument.h"

#define __ASM __asm



int MnSendMessage (DWORD pid, message_t *message, parerror_t *pe)
{
	HANDLE event = NULL, resp = NULL;
	int st;
	int havemutex = 0;
	int ack = 1; //whether we got a successful ack

	fail_if (!IsmCheckMode (MODE_MANAGER), 0, PE ("MnSendMessage is for managers only", 0, 0, 0, 0));

	//some sanity checks
	fail_if (message->size > MAX_INSTRUMENT_SIZE || message->magic != MAGIC_NUMBER,
		0, PE ("Refusing to send malformed message (%X %i)", message->magic, message->size, 0, 0));

	message->pid = pid;

	//create the event to signal process
	//if it's not there, no sense going any farther
	event = UtilOpenEventf ("Instrument-pid-%i", pid);
	fail_if (!event, 0, PE ("Could not find target event (%i)", GetLastError (), 0, 0, 0));

	//get manager mutex
	st = WaitForSingleObject (IsmManagerMutex, INFINITE);
	fail_if (st == WAIT_FAILED, 0, PE ("Wait failure (%i)", GetLastError (), 0, 0, 0));
	havemutex = 1;

	message->event = IsmResponseNumber++;
	resp = UtilCreateEventf (NULL, FALSE, FALSE, "Instrument-event-%i", message->event);
	fail_if (!resp, 0, PE ("Could not create response event (%i)", GetLastError (), 0, 0, 0));

	//grab the message buffer mutex and copy the data to shared memory
	st = WaitForSingleObject (IsmMessageMutex, INFINITE);
	fail_if (st == WAIT_FAILED, 0, PE ("Wait failure (%i)", GetLastError (), 0, 0, 0));

	memcpy (IsmMessageBuffer, message, message->size);

	st = ReleaseMutex (IsmMessageMutex);
	fail_if (!st, 0, PE ("ReleaseMutex failed (%i) (but the command was sent)", GetLastError (), 0, 0, 0));

	st = SetEvent (event);
	fail_if (!st, 0, PE ("SetEvent failed (%i) (but command was sent)", GetLastError (), 0, 0, 0));

	//wait up to 2 seconds for the agent to respond
	st = WaitForSingleObject (resp, 2000);
	fail_if (st == WAIT_FAILED, 0, PE ("Wait failure (%i) waiting for ack event", GetLastError (), 0, 0, 0));
	if (st == WAIT_TIMEOUT)
		ack = 0;

	//grab message buffer again and clear it for good measure.
	st = WaitForSingleObject (IsmMessageMutex, INFINITE);
	fail_if (st == WAIT_FAILED, 0, PE ("Wait failure (%i)", GetLastError (), 0, 0, 0));

	memset (IsmMessageBuffer, 0, MAX_INSTRUMENT_SIZE);

	st = ReleaseMutex (IsmMessageMutex);
	fail_if (!st, 0, PE ("ReleaseMutex failed (%i) after message was sent", GetLastError (), 0, 0, 0));

	//ok, we're done.
	st = ReleaseMutex (IsmManagerMutex);
	fail_if (!st, 0, PE ("ReleaseMutex failed (%i) after message was sent", GetLastError (), 0, 0, 0));
	havemutex = 0;

	CloseHandle (resp);
	CloseHandle (event);

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

	return 0;
}


int MnIsProcessHooked (DWORD pid)
{
	HANDLE event;

	event = UtilOpenEventf ("Instrument-pid-%i", pid);
	if (event)
	{
		CloseHandle (event);
		return 1;
	}

	return 0;
}

/*
DWORD CALLBACK targ (void *param)
{

	return 0;
}

VOID WINAPI ethr (DWORD code)
{
	ExitThread (code);
}

BOOL WINAPI vfex (HANDLE pro, void *addy, int len, int type)
{
	return VirtualFreeEx (pro, addy, len, type);
}
*/

typedef struct
{
	char *libname;
	char *funcname;
	void *gmh; //GetModuleHandleA
	void *gpa; //GetProcAddressA
	void *et; //ExitThread
	void *vfe; //VirtualFreeEx
	void *param;
} hookiedata_t;
#define HOOKIE_VALUE_OFS 13
#define HOOKIE_FUNC_SIZE 100


__declspec (naked) DWORD CALLBACK MnForeignStub (void *param)
{
	
	__ASM //screw you msdev 6
	{
		nop
		nop
		nop
		nop
		nop
		nop

		call getEIP
		jmp past

		//the important data goes here
		_emit 0x00
		_emit 0x00
		_emit 0x00
		_emit 0x00

getEIP:
		mov eax, [esp - 0h]
		ret

past:
		add eax, 2
		mov ecx, [eax]
		//eax has pointer to important data; copy it to the stack

		push ecx
		mov eax, [ecx]hookiedata_t.libname
		push eax
		call [ecx]hookiedata_t.gmh

		pop ecx //get it out now; we need it in error_out (plus we don't want to jump to error_out with the stack off)

		cmp eax, 0
		je error_out

		
		push ecx
		push [ecx]hookiedata_t.funcname
		push eax
		call [ecx]hookiedata_t.gpa

		pop ecx

		cmp eax, 0
		je error_out

		//eax has the target address
		//prepare for a call to VirtualFreeEx
		push MEM_RELEASE
		push 0
		push ecx
		push 0xFFFFFFFF

		//return address is eax
		push eax

		jmp [ecx]hookiedata_t.vfe

error_out:
		//prepare the stack such that when VirtualFreeEx returns, it returns
		//directly to ExitThread.  We need to do this because the memory that
		//holds this code will be gone when vfex returns.

		//parameter for ExitThread
		push 0xbad
		//the address to which ExitThread would return (if it was a function
		//that could return, which obviously it isn't, but the value is expected)
		//anyhow, to properly align the parameters.)
		push 0xEEEE0BAD
		
		
		//parameters for VirtualFreeEx
		push MEM_RELEASE
		push 0
		push ecx
		push 0xFFFFFFFF //GetCurrentProcess().  Todo: actually call this function?

		//return address.
		push [ecx]hookiedata_t.et
		

		jmp [ecx]hookiedata_t.vfe
		

	}
}

int MnStartAgentThread (HANDLE process, int infectious, parerror_t *pe, char *lib, char *function)
{
	void *addy = NULL;
	hookiedata_t hd;
	DWORD len;
	char *pos;
	HMODULE kern;
	char *funcmod, *temp;
	DERR st;
	DWORD writ;
	HANDLE thread;

	len = sizeof (hookiedata_t) + strlen (lib) + strlen (function) + 2 + HOOKIE_FUNC_SIZE;

	addy = VirtualAllocEx (process, NULL, len, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	fail_if (!addy, 0, PE ("VirtualAllocEx failed, %i\n", GetLastError (), 0, 0, 0));

	kern = GetModuleHandle ("kernel32.dll");
	fail_if (!kern, 0, PE ("GetModuleHandle of kernel32.dll failed, %i\n", GetLastError (), 0, 0, 0));

	pos = addy;

	hd.gmh = GetProcAddress (kern, "GetModuleHandleA");
	fail_if (!hd.gmh, 0, PE ("GetProcAddress for GetModuleHandleA failed, %i", GetLastError (), 0, 0, 0));
	hd.gpa = GetProcAddress (kern, "GetProcAddress");
	fail_if (!hd.gpa, 0, PE ("GetProcAddress for GetProcAddress failed, %i", GetLastError (), 0, 0, 0));
	//hd.et = DetourGetFinalCode ((BYTE *)ethr, TRUE);
	hd.et = GetProcAddress (kern, "ExitThread");
	fail_if (!hd.et, 0, PE ("GetProcAddress for ExitThread failed, %i", GetLastError (), 0, 0, 0));
	//hd.vfe = DetourGetFinalCode ((BYTE *)vfex, TRUE);
	hd.vfe = GetProcAddress (kern, "VirtualFreeEx");
	fail_if (!hd.vfe, 0, PE ("GetProcAddress for VirtualFreeEx failed, %i", GetLastError (), 0, 0, 0));


	hd.libname = pos + sizeof (hookiedata_t);
	hd.funcname = hd.libname + strlen (lib) + 1;
	hd.param = (void *)infectious;

	temp = DetourGetFinalCode ((BYTE *)MnForeignStub, TRUE);
	fail_if (!temp, 0, PE ("Could not get final code", 0, 0, 0, 0));

	funcmod = _alloca (HOOKIE_FUNC_SIZE);
	memcpy (funcmod, temp, HOOKIE_FUNC_SIZE);
	*((void **)(funcmod + HOOKIE_VALUE_OFS)) = addy;

	len = sizeof (hookiedata_t);
	st = WriteProcessMemory (process, pos, &hd, len, &writ);
	fail_if (!st || writ != len, 0, PE ("WriteProcessMemory of %i bytes failed, %i", len, GetLastError (), 0, 0));
	pos += len;

	len = strlen (lib) + 1;
	st = WriteProcessMemory (process, pos, lib, len, &writ);
	fail_if (!st || writ != len, 0, PE ("WriteProcessMemory of %i bytes failed, %i", len, GetLastError (), 0, 0));
	pos += len;

	len = strlen (function) + 1;
	st = WriteProcessMemory (process, pos, function, len, &writ);
	fail_if (!st || writ != len, 0, PE ("WriteProcessMemory of %i bytes failed, %i", len, GetLastError (), 0, 0));
	pos += len;

	len = HOOKIE_FUNC_SIZE;
	st = WriteProcessMemory (process, pos, funcmod, len, &writ);
	fail_if (!st || writ != len, 0, PE ("WriteProcessMemory of %i bytes failed, %i", len, GetLastError (), 0, 0));
	//pos += len; leave pos pointing to thread function

	//ok, it's all written.  Start a thread.
	thread = CreateRemoteThread (process, NULL, 0, (void *)pos, (void *)infectious, 0, &writ);
	fail_if (!thread, 0, PE ("CreateRemoteThread failed, %i", GetLastError (), 0, 0, 0));

	//all good.
	CloseHandle (thread);
	
	//addy is the responsibility of the thread now.
	return 1;

failure:

	if (addy)
		VirtualFreeEx (process, addy, 0, MEM_RELEASE);

	return 0;
}	



int MnTestHookie (parerror_t *pe)
{
	DERR st;

	st = MnStartAgentThread (GetCurrentProcess (), 234, pe, "instQument.dll", "targ");
	return st;
}
	

int MnHookProcess (DWORD pid, parerror_t *pe)
{
	char buffer[MAX_PATH];
	HANDLE process = NULL;
	int st;

	fail_if (!IsmCheckMode (MODE_MANAGER), 0, PE ("MnSendMessage is for managers only", 0, 0, 0, 0));

	process = OpenProcess (PROCESS_ALL_ACCESS, FALSE, pid);
	fail_if (!process, 0, PE ("Could not open process %i (%i)", pid, GetLastError (), 0, 0));

	st = GetModuleFileName (instance, buffer, MAX_PATH);
	fail_if (!st, 0, PE ("GetModuleFileName failed (%i)", GetLastError (), 0, 0, 0));
	buffer[MAX_PATH - 1] = '\0';

	st = DetourContinueProcessWithDllA(process, buffer);
	fail_if (!st, 0, PE_STR ("Injection of dll %s into %i failed (%i)", COPYSTRING (buffer), pid, GetLastError (), 0));

	st = MnStartAgentThread (process, 0, pe, buffer, "AgnInstrumentationListener");
	fail_if (!st, 0, 0);
	
	CloseHandle (process);

	return 1;
failure:

	if (process)
		CloseHandle (process);

	return 0;
}

int MnScriptForceHookProcess (DWORD pid, parerror_t *pe)
{
	int st;

	st = MnHookProcess (pid, pe);
	if (st)
		return pid;
	else
		return 0;
}



int MnScriptHookProcess (DWORD pid, parerror_t *pe)
{
	int x;

	if (MnIsProcessHooked (pid))
		return pid; //do nothing

	if (!MnHookProcess (pid, pe))
		return 0;
	
	//some delay is typically needed to let the other process get control and
	//startup its thread.
	for (x=0;x<20;x++)
	{
		Sleep (50);
		if (MnIsProcessHooked (pid))
			return pid;
	}

	//if we got here we probably shouldn't let the command go through
	PE ("Process did not activate its hook thread in the alloted timeout", 0, 0, 0, 0);
	return 0;
}



message_t *MnCreateInstrumentMessage (int command, instrument_t *inst, action_t *action, parerror_t *pe)
{
	message_t *out = NULL;
	int size;
	int st;

	size = sizeof (message_t) + inst->size + action->size;
	fail_if (size > MAX_INSTRUMENT_SIZE, 0, PE ("Instrument too big (%i)", size, 0, 0, 0));

	out = IsmMalloc (size);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (size));

	out->magic = MAGIC_NUMBER;
	out->instrumentofs = 0;
	out->actionsofs = inst->size;
	out->size = size;
	out->command = command;
	
	memcpy (out->data, inst, inst->size);
	memcpy (out->data + inst->size, action, action->size);

	return out;
failure:
	if (out)
		IsmFree (out);

	return NULL;
}

message_t *MnCreateSetInstrumentMessage (instrument_t *inst, action_t *action, parerror_t *pe)
{
	return MnCreateInstrumentMessage (COMMAND_INSTRUMENT, inst, action, pe);
}

message_t *MnCreateUnsetInstrumentMessage (instrument_t *inst, action_t *action, parerror_t *pe)
{
	return MnCreateInstrumentMessage (COMMAND_UNINSTRUMENT, inst, action, pe);
}

message_t *MnCreatePrintMessage (char *string, parerror_t *pe)
{
	message_t *out = NULL;
	int size;
	int st;

	size = sizeof (message_t) + strlen (string) + 1;
	fail_if (size > MAX_INSTRUMENT_SIZE, 0, PE ("Instrument too big (%i)", size, 0, 0, 0));

	out = IsmMalloc (size);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (size));

	out->command = COMMAND_PRINT;
	out->magic = MAGIC_NUMBER;
	out->size = size;

	strcpy (out->data, string);

	return out;
failure:

	if (out)
		IsmFree (out);

	return NULL;
}


void MnTestFunction (parerror_t *pe, char *actions)
{
	int st;
	action_t *a;

	fail_if (!actions, 0, PE_BAD_PARAM (1));

	a = ActnParseString (actions, pe);
	if (a)
		IsmFree (a);

failure:
	return;
}




int MnSetFunctionInstrument (parerror_t *pe, int pid, char *lib, char *func, int stdcall, int numparams, char *action)
{
	message_t *m = NULL;
	instrument_t *i = NULL;
	action_t *a = NULL;
	int st;

	fail_if (!lib, 0, PE_BAD_PARAM (2));
	fail_if (!func, 0, PE_BAD_PARAM (3));
	fail_if (!action, 0, PE_BAD_PARAM (6));

	i = InCreateFunctionInstrument (lib, func, numparams, stdcall, 1, pe);
	fail_if (!i, 0, 0);

	a = ActnParseString (action, pe);
	fail_if (!a, 0, 0);

	m = MnCreateSetInstrumentMessage (i, a, pe);
	fail_if (!m, 0, 0);

	st = MnSendMessage (pid, m, pe);
	fail_if (!st, 0, 0);

	//ok, all's well.  Free the stuff, and return
	IsmFree (a);
	IsmFree (m);
	IsmFree (i);

	if (st == 1)
		return 1;

	//means st == 2, or not aknowledged
	return 0;
failure:

	if (m)
		IsmFree (m);

	if (a)
		IsmFree (a);

	if (i)
		IsmFree (i);

	return 0;
}


int MnRemoveFunctionInstrument (parerror_t *pe, int pid, char *lib, char *func, int stdcall, int numparams)
{
	message_t *m = NULL;
	instrument_t *i = NULL;
	action_t *a = NULL;
	int st;

	fail_if (!lib, 0, PE_BAD_PARAM (2));
	fail_if (!func, 0, PE_BAD_PARAM (3));

	i = InCreateFunctionInstrument (lib, func, numparams, stdcall, 1, pe);
	fail_if (!i, 0, 0);

	a = ActnParseString ("f0", pe); //no-op.  The actions string is ignored by the agent anyhow.
	fail_if (!a, 0, 0);

	m = MnCreateUnsetInstrumentMessage (i, a, pe);
	fail_if (!m, 0, 0);

	st = MnSendMessage (pid, m, pe);
	fail_if (!st, 0, 0);

	//ok, all's well.  Free the stuff, and return
	IsmFree (a);
	IsmFree (m);
	IsmFree (i);

	if (st == 1)
		return 1;

	//means st == 2, or not aknowledged
	return 0;
failure:

	if (m)
		IsmFree (m);

	if (a)
		IsmFree (a);

	if (i)
		IsmFree (i);

	return 0;

}


int MnSetAddressInstrument (parerror_t *pe, int pid, int addy, int stdcall, int numparams, char *action)
{
	message_t *m = NULL;
	instrument_t *i = NULL;
	action_t *a = NULL;
	int st;

	fail_if (!action, 0, PE_BAD_PARAM (5));

	i = InCreateAddressInstrument ((void *)addy, numparams, stdcall, 1, pe);
	fail_if (!i, 0, 0);

	a = ActnParseString (action, pe);
	fail_if (!a, 0, 0);

	m = MnCreateSetInstrumentMessage (i, a, pe);
	fail_if (!m, 0, 0);

	st = MnSendMessage (pid, m, pe);
	fail_if (!st, 0, 0);

	//ok, all's well.  Free the stuff, and return
	IsmFree (a);
	IsmFree (m);
	IsmFree (i);

	if (st == 1)
		return 1;

	//means st == 2, or not aknowledged
	return 0;
failure:

	if (m)
		IsmFree (m);

	if (a)
		IsmFree (a);

	if (i)
		IsmFree (i);

	return 0;
}


int MnRemoveAddressInstrument (parerror_t *pe, int pid, int addy, int stdcall, int numparams)
{
	message_t *m = NULL;
	instrument_t *i = NULL;
	action_t *a = NULL;
	int st;

	i = InCreateAddressInstrument ((void *)addy, numparams, stdcall, 1, pe);
	fail_if (!i, 0, 0);

	a = ActnParseString ("f0", pe); //no-op.  The actions string is ignored by the agent anyhow.
	fail_if (!a, 0, 0);

	m = MnCreateUnsetInstrumentMessage (i, a, pe);
	fail_if (!m, 0, 0);

	st = MnSendMessage (pid, m, pe);
	fail_if (!st, 0, 0);

	//ok, all's well.  Free the stuff, and return
	IsmFree (a);
	IsmFree (m);
	IsmFree (i);

	if (st == 1)
		return 1;

	//means st == 2, or not aknowledged
	return 0;
failure:

	if (m)
		IsmFree (m);

	if (a)
		IsmFree (a);

	if (i)
		IsmFree (i);

	return 0;

}

int MnSetWindowInstrumentFull (parerror_t *pe, int pid, HWND hwnd, UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask, char *action)
{
	message_t *m = NULL;
	instrument_t *i = NULL;
	action_t *a = NULL;
	int st;

	fail_if (!action, 0, PE_BAD_PARAM (6));

	i = InCreateWindowInstrument (hwnd, message, wParam, wMask, lParam, lMask, pe);
	fail_if (!i, 0, 0);

	a = ActnParseString (action, pe);
	fail_if (!a, 0, 0);

	m = MnCreateSetInstrumentMessage (i, a, pe);
	fail_if (!m, 0, 0);

	st = MnSendMessage (pid, m, pe);
	fail_if (!st, 0, 0);

	//ok, all's well.  Free the stuff, and return
	IsmFree (a);
	IsmFree (m);
	IsmFree (i);

	if (st == 1)
		return 1;

	//means st == 2, or not aknowledged
	return 0;
failure:

	if (m)
		IsmFree (m);

	if (a)
		IsmFree (a);

	if (i)
		IsmFree (i);

	return 0;
}

int MnSetWindowInstrumentBasic (parerror_t *pe, int pid, HWND hwnd, UINT message, char *actions)
{
	return MnSetWindowInstrumentFull (pe, pid, hwnd, message, 0, 0, 0, 0, actions);
}


int MnRemoveWindowInstrumentFull (parerror_t *pe, int pid, HWND hwnd, UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask)
{
	message_t *m = NULL;
	instrument_t *i = NULL;
	action_t *a = NULL;
	int st;

	i = InCreateWindowInstrument (hwnd, message, wParam, wMask, lParam, lMask, pe);
	fail_if (!i, 0, 0);

	a = ActnParseString ("f0", pe);
	fail_if (!a, 0, 0);

	m = MnCreateUnsetInstrumentMessage (i, a, pe);
	fail_if (!m, 0, 0);

	st = MnSendMessage (pid, m, pe);
	fail_if (!st, 0, 0);

	//ok, all's well.  Free the stuff, and return
	IsmFree (a);
	IsmFree (m);
	IsmFree (i);

	if (st == 1)
		return 1;

	//means st == 2, or not aknowledged
	return 0;
failure:

	if (m)
		IsmFree (m);

	if (a)
		IsmFree (a);

	if (i)
		IsmFree (i);

	return 0;
}

int MnRemoveWindowInstrumentBasic (parerror_t *pe, int pid, HWND hwnd, UINT message)
{
	return MnRemoveWindowInstrumentFull (pe, pid, hwnd, message, 0, 0, 0, 0);
}


//This function is an odd one.  It is called by AGENTS when they detect a new process
//has been spawned.  The purpose is to set any instruments that exist in this process
//in the newly spawned process.  This function is called in a detour of CreateProcess
//which is installed by the agent thread when it is started in infectious mode.  The
//process in question should already be started, suspended, pending the return of the
//call to CreateProcess.  
