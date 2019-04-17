/* Copyright 2008 wParam, licensed under the GPL */

#include <windows.h>


#include "..\common\libplug.h"



__declspec (naked) static void temp_checkesp (void)
{
	__asm
	{
		ret;
	}
}

static void *checkesp = temp_checkesp;

#ifndef HAVE_CRUNTIME

__declspec (naked) void _chkesp (void)
{
	//needs some special processing, because this function will be called
	//from dllmain.
	__asm
	{
		mov ecx, checkesp
		jmp ecx
	}
}

#endif


static pluglib_t *PlInterface;
static pluginfo_t *PlInfo; //only used as a key into some of the p5 functions

//Transfers execution to the given element of PlInterface
#define JMP(func) __asm { mov ecx, [PlInterface] } ; __asm { jmp dword ptr [ecx]PlInterface.func }

//Transfers execution to the given elemtnt of PlInterface, adding the
//given extra info to the beginning of the param list.  YOU MUST USE
//__stdcall FOR THIS TO WORK PROPERLY, in both the PlXxx function below
//and the actual target function.
#define JMPEXTRA(func, extra) __asm { pop ecx } ; __asm {push extra} ; __asm { push ecx } ; __asm { mov ecx, [PlInterface] } ; __asm { jmp dword ptr [ecx]PlInterface.func }
		

__declspec (naked) void *ParMalloc (int size)
{
	JMP (ParMalloc);
}

__declspec (naked) void ParFree (void *block)
{
	JMP (ParFree);
}

__declspec (naked) char *PlGetDerrString (DERR in)
{
	JMP (PfDerrString);
}

__declspec (naked) DERR PlPrintf (char *in, ...)
{
	//here's an example of a function that wouldn't work if we weren't
	//using the assembly way.  (We'd have to define a ConWriteStringfv)
	JMP (ConWriteStringf);
}

__declspec (naked) void * __stdcall PlMalloc (int size)
{
	JMPEXTRA (PlugMalloc, PlInfo);
}

__declspec (naked) void * __stdcall PlRealloc (void *block, int size)
{
	JMPEXTRA (PlugRealloc, PlInfo);
}

__declspec (naked) void * __stdcall PlFree (void *block)
{
	JMPEXTRA (PlugFree, PlInfo);
}

__declspec (naked) int PlIsMainThread (void)
{
	JMP (PfIsMainThread);
}

__declspec (naked) DERR PlCreateParser (parser_t **out)
{
	JMP (RopInherit);
}

__declspec (naked) DERR PlDestroyParser (parser_t *par)
{
	JMP (ParDestroy);
}

__declspec (naked) DERR PlResetParser (parser_t *par)
{
	JMP (ParReset);
}

__declspec (naked) void PlProcessError (int st, char *error, int errlen, parerror_t *pe)
{
	JMP (ParProcessErrorType);
}

__declspec (naked) void PlCleanupError (parerror_t *pe)
{
	JMP (ParCleanupError);
}

__declspec (naked) void *PlMemset (void *dest, int c, size_t count)
{
	JMP (memset);
}

#ifndef HAVE_CRUNTIME
__declspec (naked) void *_alloca_probe (size_t sizealloc)
{
	JMP (_alloca);
}
#endif

__declspec (naked) size_t PlStrlen (const char *string)
{
	JMP (strlen);
}

__declspec (naked) char *PlStrcat (char *strDestination, const char *strSource)
{
	JMP (strcat);
}

__declspec (naked) char *PlStrcpy (char *strDestination, const char *strSource)
{
	JMP (strcpy);
}

__declspec (naked) DERR PlMainThreadParse (char *line, char **output, parerror_t *pe)
{
	JMP (PfMainThreadParse);
}

__declspec (naked) DERR PlRunParser (parser_t *par, char *line, char **output, int *nesting, char *error, int errlen)
{
	JMP (ParRunLine);
}

__declspec (naked) HWND PlGetMainWindow (void)
{
	JMP (PfGetMainWindow);
}

__declspec (naked) int PlRemoveMessageCallback (UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask, WNDPROC proc)
{
	JMP (PlugRemoveMessageCallback);
}

__declspec (naked) int PlAddMessageCallback (UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask, WNDPROC proc)
{
	JMP (PlugAddMessageCallback);
}

__declspec (naked) int PlAtoi (char * foo)
{
	JMP (atoi);
}

__declspec (naked) DERR PlSplitString (char *weekdays, int *outcount, char **outarray, char delim)
{
	JMP (ParSplitString);
}

__declspec (naked) int PlSnprintf (char *buffer, size_t count, const char *format, ...)
{
	JMP (_snprintf);
}

__declspec (naked) DERR PlParserAddAtom (parser_t *par, char *atom, char *value, int flags)
{
	JMP (ParAddAtom);
}

__declspec (naked) DERR PlParserRemoveAtom (parser_t *par, char *name)
{
	JMP (ParRemoveAtom);
}

__declspec (naked) DERR PlAddInfoBlock (char *name, HINSTANCE module, int resid)
{
	JMP (InfAddInfoBlock);
}

__declspec (naked) void PlRemoveInfoBlock (char *name)
{
	JMP (InfRemoveInfoBlock);
}

__declspec (naked) DERR PlPixshellThreeRun (char *command, int async)
{
	JMP (UtilPixshellThreeRun);
}

__declspec (naked)DERR PlProperShell (char *line)
{
	JMP (UtilProperShell);
}

__declspec (naked) int PlStrcmp (const char *string1, const char *string2)
{
	JMP (strcmp);
}

__declspec (naked) char *PlStrstr (const char *string, const char *strCharSet)
{
	JMP (strstr);
}

__declspec (naked) int PlStrncmp (const char *string1, const char *string2, size_t count)
{
	JMP (strncmp);
}

__declspec (naked) char *PlLongIntToType (parerror_t *pe, __int64 in)
{
	JMP (UtilLongIntToType);
}

__declspec (naked) int PlTypeToLongInt (parerror_t *pe, char *in, __int64 *output)
{
	JMP (UtilTypeToLongInt);
}

__declspec (naked) char *PlBinaryToType (parerror_t *pe, void *voidin, int inlen, char *typename)
{
	JMP (UtilBinaryToType);
}

__declspec (naked) int PlTypeToBinary (parerror_t *pe, char *type, char *typeexpected, int *lenout, void *voiddata, int datalen)
{
	JMP (UtilTypeToBinary);
}

__declspec (naked) void *PlMemcpy (void *dest, const void *src, size_t count)
{
	JMP (memcpy);
}

__declspec (naked) DERR __stdcall PlAddWatcher (char *type, void *value)
{
	JMPEXTRA (PlugAddWatcher, PlInfo);
}

__declspec (naked) DERR PlDeleteWatcher (char *type, void *value)
{
	JMP (PlugDeleteWatcher);
}

__declspec (naked) int PlCheckWatcher (char *type, void *value)
{
	JMP (WatCheck);
}

__declspec (naked) double PlFmod(double val, double modby)
{
	JMP (fmod);
}

__declspec (naked) double PlSqrt(double f)
{
	JMP (sqrt);
}

__declspec (naked) double PlSin(double f)
{
	JMP (sin);
}

__declspec (naked) double PlCos(double f)
{
	JMP (cos);
}

__declspec (naked) double PlTan(double f)
{
	JMP (tan);
}

__declspec (naked) double PlAsin(double f)
{
	JMP (asin);
}

__declspec (naked) double PlAcos(double f)
{
	JMP (acos);
}

__declspec (naked) double PlAtan(double f)
{
	JMP (atan);
}

__declspec (naked) double PlCeil(double f)
{
	JMP (ceil);
}

__declspec (naked) double PlFloor(double f)
{
	JMP (floor);
}

__declspec (naked) double PlLog(double f)
{
	JMP (log);
}

__declspec (naked) double PlLog10(double f)
{
	JMP (log10);
}

__declspec (naked) double PlPow(double f, double r)
{
	JMP (pow);
}

__declspec (naked) long _ftol (double f)
{
	JMP (ftol);
}



/*
	The skeletal asm from which the two macros are defined.
	__asm
	{
		pop eax
		push poo
		push eax
		mov         eax,[poo]
		jmp dword ptr [eax+4]
	}

	__asm
	{
		mov eax,[poo]
		jmp dword ptr [eax+blah]
	}
*/


extern char *PluginName;
extern char *PluginDescription;
extern plugfunc_t PluginFunctions[];
void PluginInit (parerror_t *);
void PluginDenit (parerror_t *);
void PluginKill (parerror_t *);
void PluginShutdown (void);

__declspec (dllexport) void P5_PluginExchange (parerror_t *pe, pluglib_t *lib, pluginfo_t *info)
{
	if (lib->size < sizeof (pluglib_t))
	{
		/* We've been loaded into a p5 process that doesn't support all of the
		 * functions we do.  We need to abort because there's a NULL pointer
		 * call risk. */
		PE ("Plugin expects for newer version of p5.exe (libsize %i needs to be at least %i)", lib->size, sizeof (pluglib_t), 0, 0);
		return;
	}

	PlInterface = lib;
	PlInfo = info;
	checkesp = lib->_chkesp;

	info->name = PluginName;
	info->description = PluginDescription;
	info->functions = PluginFunctions;
	info->init = PluginInit;
	info->denit = PluginDenit;
	info->kill = PluginKill;
	info->shutdown = PluginShutdown;

}