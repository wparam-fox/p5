/* Copyright 2006 wParam, licensed under the GPL */

#include <windows.h>
#include "instrument.h"
//#include "wp_codeseg.h"


//#define MONEY_GETLENGTH 1
//#define MONEY_GETLIVEOFFSET 2


typedef struct
{
	int stdcall;
	int numparams;

	BYTE *trampoline;
	void *original; //addy of function that was instrumented

	action_t *actions;

	BYTE function[0];
} prep_t;

typedef struct
{
	void **args;
	prep_t *prep;
} hookcall_t;

DWORD FhGenericHookCallback (hookcall_t *params)
{
	//ok.  We need to call the trampoline as if it was a function.
	int x;
	void *temp;
	int st;

	fail_if (!params->prep->trampoline, 0, DP (ERROR, "Trampoline is gone; returning zero\n"));

	//push params on the stack in reverse order
	for (x=params->prep->numparams - 1;x>=0;x--)
	{
		temp = params->args[x];
		__asm { push temp }
	}

	//call the function and snag the return value from eax
	temp = params->prep->trampoline;
	__asm 
	{ 
		call temp 
		mov temp, eax
	}

	//if it's a __cdecl function, pop the stack
	if (!params->prep->stdcall)
	{
		x = params->prep->numparams * 4;
		__asm { add esp, x }
	}

	return (DWORD)temp;

failure:
	return 0;
}



DWORD __cdecl FhGenericHook (void **args, prep_t *prep)
{
	hookcall_t hc = {args, prep};
	int st;

	//IsmDprintf (NOISE, "%X %X %X\n", args, prep, prep->actions);

	fail_if (!prep, 0, DP (ERROR, "OH GOD We're missing the prep_t... returning 0\n"));
	fail_if (!args, 0, DP (ERROR, "OH GOD We're missing the args... returning 0\n"));
	fail_if (!prep->trampoline, 0, DP (ERROR, "In detour function, but have no trampoline.  Landing will hurt\n"));

	if (!prep->actions)
		return FhGenericHookCallback (&hc);

	return ActnExecute (prep->actions, FhGenericHookCallback, &hc, args, prep->numparams);

failure:
	return 0;

}


#define GENERIC_HOOK_VALUE_OFS 13
#define GENERIC_HOOK_RET_OFS 45
#define GENERIC_HOOK_LEN 100

//By default, this function acts as int (__stdcall *)(int, int), NOT __cdecl as declared
//I could change it, but I won't, because it doesn't matter, but it might.
//Basically, the calling convention will vary.  (This functions is never actually called;
//rather, copies of it are allocated on the heap and modified as appropriate.
__declspec (naked) DWORD __cdecl FhGenericHookPrep (int a, ...)
{
	__asm
	{
		//six no-op instructions, six bytes.  This is here to solve problems with
		//double-instrumenting the same function.  I don't want a double instrument
		//to cause the process to abort in a strange way, so I'm adding these bytes.
		//The bug happens like this:
		//*The function is instrumented, so now GetFinalCode() returns a pointer to
		//one of these prep functions on the heap.
		//*The detour library obediently replaces the first 5 bytes worth of
		//instructions (which was formerly our "call getEIP" instruction) with a call
		//to the new detour (fine).
		//*The detour function eventually calls indo FhGenericHookCallback, which
		//calls the "original address" trampoline (which is actually the trampoline
		//formed by one of these prep functions)
		//*The first instruction in the trampoline is the same as what used to be the
		//first instruction here, i.e. "call getEIP"
		//*However, when getEIP runs, the return address on the stack NO LONGER POINTS
		//INTO THIS PREP FUNCTION, it points somewhere else, so a garbage value is
		//retrieved.
		//*getEIP returns, and the trampoline jumps to the "jmp past" instruction below,
		//and everything proceeds as normal, except with a corrupt "prep" parameter
		//being sent to FhGenericHook.
		//So, we put six bytes in here so that the detours lib isn't moving our call
		//instruction.
		nop
		nop
		nop
		nop
		nop
		nop

		//EIP isn't available with mov... you have to call and read it from the stack;
		call getEIP

		//this jump was just to keep the VALUE_OFS above from changing
		jmp past

		//These 4 emits are replaced by the prep_t at runtime.
		_emit 0x00
		_emit 0x00
		_emit 0x00
		_emit 0x00
getEIP:
		mov eax, [esp - 0h]
		ret

past:
		//add 2, because eax, as returned by getEIP, points to the
		//"jmp past" instruction, which is 2 bytes long.
		add eax, 2
		mov eax, [eax]
		push eax

		mov eax, esp
		add eax, 8
		push eax
		//pop eax

		lea eax, FhGenericHook

		//return.  The return instruction will be modified at runtime to match
		//the function in question. 
		call eax
		add esp, 8
		ret 8

	}
}



void *FhFindFunction (char *lib, char *func)
{
	HMODULE hmod;
	void *out;
	int loaded = 0;
	int st;

	hmod = GetModuleHandle (lib);
	if (!hmod)
	{
		//try to load it then
		hmod = LoadLibrary (lib);
		fail_if (!hmod, 0, DP (ERROR, "Unable to load library %s (%i)\n", lib, GetLastError ()));
		loaded = 1;
	}

	out = GetProcAddress (hmod, func);
	fail_if (!out, 0, DP (ERROR, "Unable to find function %s in %s (%i)\n", func, lib, GetLastError ()));

	return out;

failure:
	if (loaded)
		FreeLibrary (hmod);

	return NULL;
}



prep_t *FhCreateNewPrep (int stdcall, int numparams)
{
	prep_t *out = NULL;
	void *func;
	int st;


	out = IsmMalloc (sizeof (prep_t) + GENERIC_HOOK_LEN);
	fail_if (!out, 0, DP (ERROR, "Failed to allocate %i bytes in CreateNewPrep\n", sizeof (prep_t) + GENERIC_HOOK_LEN));
	memset (out, 0, sizeof (prep_t)); //no need to zero the funciton

	func = DetourGetFinalCode ((BYTE *)FhGenericHookPrep, TRUE);
	fail_if (!func, 0, DP (ERROR, "GetFinalCode failed (this should be impossible (%i)\n", GetLastError ()));

	memcpy (out->function, func, GENERIC_HOOK_LEN);

	*((prep_t **)(out->function + GENERIC_HOOK_VALUE_OFS)) = out;

	//if it's not stdcall, we don't have to clean up any params, hence the 0
	//this is turning the "ret 8" instruction into "ret X", where X is the
	//approptiate value.
	*((short *)(out->function + GENERIC_HOOK_RET_OFS)) = stdcall ? numparams * 4 : 0;

	out->numparams = numparams;
	out->stdcall = stdcall;

	return out;

failure:
	if (out)
		IsmFree (out);

	return NULL;
}







int FhSetFunctionHook (instlist_t *item)
{
	instfunction_t *instf;
	prep_t *prep = NULL;
	void *func;
	int st;

	fail_if (!IsmCheckMode (MODE_AGENT), 0, DP (ERROR, "Instruments can only be set by an agent\n"));
	fail_if (item->inst->type != INSTRUMENT_FUNCTION, 0, DP (ERROR, "Instrument type mismatch (%i should be %i)\n", item->inst->type, INSTRUMENT_FUNCTION));

	instf = (instfunction_t *)item->inst->data;

	prep = FhCreateNewPrep (instf->stdcall, instf->numparams);
	fail_if (!prep, 0, DP (ERROR, "Could not create new prep.\n"));

	func = FhFindFunction (instf->data, instf->data + instf->nameofs);
	fail_if (!func, 0, DP (ERROR, "FuncFunction could not find function\n"));

	if (instf->dofinalcode)
		func = DetourGetFinalCode (func, TRUE);

	prep->original = func;
	prep->actions = item->actions;

	//there is a time between when the function is hooked and when the trampoline
	//is set.  Can't be helped, really.  To try to make it less likely that we will
	//be interrupted inside of DetourFunction, I'm calling Sleep (1) to force the
	//thread off of the processor, so that hopefully it will have a full timeslice
	//to work with.
	Sleep (1);
	prep->trampoline = DetourFunction (func, prep->function);
	fail_if (!prep->trampoline, 0, DP (ERROR, "Couldn't set trampoline (%i)\n", GetLastError ()));

	IsmDprintf (NOISE, "Function detour set: %X %X %X\n", prep->trampoline, func, prep->function);

	*((prep_t **)item->runtime) = prep;

	return 1;

failure:
	if (prep)
		free (prep);

	return 0;

}

int FhSetAddressHook (instlist_t *item)
{
	instaddress_t *insta;
	prep_t *prep = NULL;
	void *func;
	int st;

	fail_if (!IsmCheckMode (MODE_AGENT), 0, DP (ERROR, "Instruments can only be set by an agent\n"));
	fail_if (item->inst->type != INSTRUMENT_ADDRESS, 0, DP (ERROR, "Instrument type mismatch (%i should be %i)\n", item->inst->type, INSTRUMENT_ADDRESS));

	insta = (instaddress_t *)item->inst->data;

	prep = FhCreateNewPrep (insta->stdcall, insta->numparams);
	fail_if (!prep, 0, DP (ERROR, "Could not create new prep.\n"));

	func = insta->address;
	fail_if (!func, 0, DP (ERROR, "Instrument contains NULL function."));

	if (insta->dofinalcode)
		func = DetourGetFinalCode (func, TRUE);

	prep->original = func;
	prep->actions = item->actions;

	//there is a time between when the function is hooked and when the trampoline
	//is set.  Can't be helped, really.  To try to make it less likely that we will
	//be interrupted inside of DetourFunction, I'm calling Sleep (1) to force the
	//thread off of the processor, so that hopefully it will have a full timeslice
	//to work with.
	Sleep (1);
	prep->trampoline = DetourFunction (func, prep->function);
	fail_if (!prep->trampoline, 0, DP (ERROR, "Couldn't set trampoline (%i)\n", GetLastError ()));

	IsmDprintf (NOISE, "Function detour set: %X %X %X\n", prep->trampoline, func, prep->function);

	*((prep_t **)item->runtime) = prep;

	return 1;

failure:
	if (prep)
		free (prep);

	return 0;

}


int FhUnsetFunctionHook (instlist_t *item)
{
	//BYTE *tramp;
	instfunction_t *instf;
	prep_t *prep;
	int st;
	//int res;

	fail_if (!IsmCheckMode (MODE_AGENT), 0, DP (ERROR, "Instruments can only be unset by an agent\n"));
	fail_if (item->inst->type != INSTRUMENT_FUNCTION, 0, DP (ERROR, "Instrument type mismatch (%i should be %i)\n", item->inst->type, INSTRUMENT_FUNCTION));

	//dprintf (NOISE, "Prep is %X\n", prep);
	instf = (instfunction_t *)item->inst->data;
	prep = *((prep_t **)item->runtime);


	/* Ok.  After much gnashing of teeth, I have decided that the detours unhooking
	   routine is just not reliable enough to be useful.  Sometimes it unhooks right,
	   sometimes not, often depending on release/debug and whether a debugger
	   is actually attached or not.  So the decision has been made not to support
	   actually unhooking, instead, we just set actions to null, which causes the
	   hook routine to do nothing but run the original code unmodified.

	   This is still not quite thread safe, by the way.  Unhooking can't possibly
	   be a safe thing to do.  Stupid multithreading...*/

	prep->actions = NULL;
	
	
	/*tramp = prep->trampoline;
	//prep->trampoline = 0;

	dprintf (NOISE, "undet: %X %X %X\n", tramp, prep->original, prep->function);
	res = DetourRemove (tramp, prep->function);
	fail_if (!res, (ERROR, "DetourRemove failed (%i)\n", GetLastError ()));

	if (instf->freeonunhook)
		free (prep);*/

	//if they didn;t set free on unhook, the memory is leaked.
	//It's not always safe to be freeing this memory; if a function
	//is still running (i.e. if they hook GetMessage or something) then
	//it wouldn't be safe to go around freeing this memory.  When you
	//set freeonunhook to 1, you're taking a risk in the name of not leaking
	//128 bytes of memory.

	return 1;

failure:
	return 0;
}

int FhUnsetAddressHook (instlist_t *item)
{
	prep_t *prep;
	int st;

	fail_if (!IsmCheckMode (MODE_AGENT), 0, DP (ERROR, "Instruments can only be unset by an agent\n"));
	fail_if (item->inst->type != INSTRUMENT_ADDRESS, 0, DP (ERROR, "Instrument type mismatch (%i should be %i)\n", item->inst->type, INSTRUMENT_ADDRESS));

	prep = *((prep_t **)item->runtime);


	/* Ok.  After much gnashing of teeth, I have decided that the detours unhooking
	   routine is just not reliable enough to be useful.  Sometimes it unhooks right,
	   sometimes not, often depending on release/debug and whether a debugger
	   is actually attached or not.  So the decision has been made not to support
	   actually unhooking, instead, we just set actions to null, which causes the
	   hook routine to do nothing but run the original code unmodified.

	   This is still not quite thread safe, by the way.  Unhooking can't possibly
	   be a safe thing to do.  Stupid multithreading...*/

	prep->actions = NULL;
	
	return 1;

failure:
	return 0;
}


instimpl_t FhFunctionImplementation = 
{
	"function",
	1,
	FhSetFunctionHook,
	FhUnsetFunctionHook,
	NULL,//ParseFunctionHook,
};

instimpl_t FhAddressImplementation = 
{
	"address",
	1,
	FhSetAddressHook,
	FhUnsetAddressHook,
	NULL,//ParseFunctionHook,
};

