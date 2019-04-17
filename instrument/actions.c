/* Copyright 2006 wParam, licensed under the GPL */

#include <windows.h>
#include "instrument.h"


typedef struct
{
	DWORD returnvalue;
	int skipping;
	int equality; //equality register, used for jne, jeq, etc. instructions
} actdata_t;

typedef struct
{
	action_t *root;
	actionitem_t *self;

	DWORD(*realfunc)(void *);
	void *funcdata;

	actdata_t *locals;

	int *vars;
	int *globals;

	void **params;
	int numparams;

	char *scratch;
} actparams_t;

typedef struct
{
	char *name;
	char *params;
	int numparams; //same as strlen (params)

	int type;
	int flags;

	int (*doaction)(actparams_t *);
} actioncommand_t;



char *ActnCopyBuffer (parerror_t *pe, char *buffer)
{
	char *out;
	int st;

	out = ParMalloc (strlen (buffer) + 1);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (strlen (buffer) + 1));
	strcpy (out, buffer);

	return out;
failure:
	return NULL;
}





enum commandtypes
{
	CMD_NOP = 0,	//0
	CMD_NOPS,		//1
	CMD_NOPII,		//2
	CMD_NOPV,		//3
	CMD_SETRETURN,	//4
	CMD_RUN,		//5
	CMD_LABEL,		//6
	CMD_PRINT,		//7
	CMD_MFPAR,		//8
	CMD_MTPAR,		//9
	CMD_INC,		//10
	CMD_MFRET,		//11
	CMD_MTRET,		//12
	CMD_MOV,		//13
	CMD_SET,		//14
	CMD_ADD,		//15
	CMD_SUBTRACT,	//16
	CMD_MULTIPLY,	//17
	CMD_DIVIDE,		//18
	CMD_CALLDLL,	//19
	CMD_CALLDLL2,	//20
	CMD_CMPI,		//21
	CMD_CMPS,		//22
	CMD_CMPNS,		//23
	CMD_CMPSW,		//24
	CMD_CMPNSW,		//25
	CMD_CMPSAW,		//26
	CMD_CMPNSAW,	//27
	CMD_JMP,		//28
	CMD_JEQ,		//29
	CMD_JNE,		//30
	CMD_JGT,		//31
	CMD_JLT,		//32
	CMD_JGE,		//33
	CMD_JLE,		//34
	CMD_LOAD,		//35
	CMD_STORE,		//36
	CMD_SETS,		//37
	CMD_MFGLOB,		//38
	CMD_MTGLOB,		//39
	CMD_NUMGLOBS,	//40
	CMD_BSIZE,		//41
	CMD_GETBUF,		//42
	CMD_STRNCAT,	//43
	CMD_STRNCATW,	//44
	CMD_INTNCAT,	//45
	CMD_ATOW,		//46
	CMD_WTOA,		//47
	CMD_PRINTS,		//48
	CMD_PRINTSW,	//49
	CMD_STRSTR,		//50
	CMD_STRSTRW,	//51
	CMD_P5,			//52
	CMD_MSGBOX,		//53
	CMD_MSGBOXW,	//54
	CMD_GETGLOB,	//55
	CMD_AND,		//56
	CMD_OR,			//57
	CMD_NOT,		//58
	CMD_SLE,		//59
	CMD_SLE2,		//60
	CMD_UPCASE,		//61
	CMD_UPCASEW,	//62

	CMD_MAX
};

#define STD_SKIP() if (a->locals->skipping) return COMMAND_OK;
#define PAR(val) ((int *)a->self->data)[val]

//command return values
#define COMMAND_OK 1		//general all good return
#define COMMAND_FAIL 0		//something went wrong; abort execution and return whatever is in the return value
#define COMMAND_RESTART 2	//restart the sequence from the beginning (used to goto a label in the past

int ActnCmdRun (actparams_t *a)
{
	STD_SKIP ();

	a->locals->returnvalue = a->realfunc (a->funcdata);
	return COMMAND_OK;
}

int ActnCmdSetReturn (actparams_t *a)
{
	STD_SKIP ();

	a->locals->returnvalue = (DWORD)(PAR (0));

	return COMMAND_OK;
}

int ActnCmdNop (actparams_t *a)
{
	return COMMAND_OK;
}

int ActnCmdLabel (actparams_t *a)
{

	if (a->locals->skipping == PAR (0))
		a->locals->skipping = 0;

	return COMMAND_OK;
}

int ActnCmdPrint (actparams_t *a)
{
	STD_SKIP ();

	IsmDprintf (NOISE, "%s\n", a->self->data);

	return COMMAND_OK;
}

int ActnCmdMoveFromParameter (actparams_t *a)
{
	int var, par;

	STD_SKIP ();

	var = PAR (0);
	par = PAR (1);

	a->vars[var] = (int)a->params[par];

	return COMMAND_OK;
}

int ActnCmdMoveToParameter (actparams_t *a)
{
	int var, par;

	STD_SKIP ();

	par = PAR (0);
	var = PAR (1);

	a->params[par] = (void *)a->vars[var];

	return COMMAND_OK;
}

int ActnCmdIncrement (actparams_t *a)
{
	int var;

	STD_SKIP ();

	var = PAR (0);

	a->vars[var]++;

	return COMMAND_OK;
}

int ActnCmdMoveFromReturnValue (actparams_t *a)
{
	int var;

	STD_SKIP ();

	var = PAR (0);

	a->vars[var] = a->locals->returnvalue;

	return COMMAND_OK;
}

int ActnCmdMoveToReturnValue (actparams_t *a)
{
	int var;

	STD_SKIP ();

	var = PAR (0);

	a->locals->returnvalue = a->vars[var];

	return COMMAND_OK;
}

int ActnCmdMove (actparams_t *a)
{
	STD_SKIP ();

	a->vars[PAR(0)] = a->vars[PAR(1)];

	return COMMAND_OK;
}

int ActnCmdSet (actparams_t *a)
{
	STD_SKIP ();

	a->vars[PAR(0)] = PAR(1);

	return COMMAND_OK;
}

int ActnCmdAdd (actparams_t *a)
{
	STD_SKIP ();

	a->vars[PAR(0)] = a->vars[PAR(0)] + a->vars[PAR(1)];

	return COMMAND_OK;
}

int ActnCmdSub (actparams_t *a)
{
	STD_SKIP ();

	a->vars[PAR(0)] = a->vars[PAR(0)] - a->vars[PAR(1)];

	return COMMAND_OK;
}

int ActnCmdMul (actparams_t *a)
{
	STD_SKIP ();

	a->vars[PAR(0)] = a->vars[PAR(0)] * a->vars[PAR(1)];

	return COMMAND_OK;
}

int ActnCmdDiv (actparams_t *a)
{
	STD_SKIP ();

	if (a->vars[PAR(1)] == 0)
		a->vars[PAR(0)] = 0x7FFFFFFF;
	else
		a->vars[PAR(0)] = a->vars[PAR(0)] / a->vars[PAR(1)];

	return COMMAND_OK;
}


//target function prototype:
//int __cdecl funcname (void **params, int numparams, int *vars, int numvars, int *retvalue, int *globals, int numglobals, char *scratch);
int ActnCmdCallDll (actparams_t *a)
{
	char *dll;
	char *func;
	int (__cdecl *addy)(void **, int, int *, int, int *, int *, int, char *);
	HMODULE hmod;
	int st;
	int loaded = 0;

	STD_SKIP ();

	if (PAR (0)) //the first "parameter" is actually used to cache the function address
	{
		addy = (void *)PAR (0);
		IsmDprintf (NOISE, "ActnCallDll: Using cached address of %X\n", addy);

		st = addy (a->params, a->numparams, a->vars, a->root->numvars, &a->locals->returnvalue, a->globals, a->root->numglobals, a->scratch);
		fail_if (!st, 0, 0);

		return COMMAND_OK;
	}

	dll = (char *)(a->self->data) + sizeof (int);
	func = dll + strlen (dll) + 1;

	hmod = GetModuleHandle (dll);
	if (!hmod)
	{
		IsmDprintf (NOISE, "Had to LoadLibrary ()\n");
		hmod = LoadLibrary (dll);
		fail_if (!hmod, 0, DP (ERROR, "Could not load dll %s\n", dll));
	}

	addy = (void *)GetProcAddress (hmod, func);
	fail_if (!addy, 0, DP (ERROR, "Could not find function %s in dll %s\n", dll, func));

	PAR (0) = (int)addy;

	st = addy (a->params, a->numparams, a->vars, a->root->numvars, &a->locals->returnvalue, a->globals, a->root->numglobals, a->scratch);
	fail_if (!st, 0, 0);

	return COMMAND_OK;

failure:

	//not freeing the dll
	return COMMAND_FAIL;
}

int ActnCmdCallDllSlow (actparams_t *a)
{
	char *dll;
	char *func;
	int (__cdecl *addy)(void **, int, int *, int, int *, int *, int, char *);
	HMODULE hmod = NULL;
	int st;

	STD_SKIP ();

	dll = a->self->data;
	func = dll + strlen (dll) + 1;

	hmod = LoadLibrary (dll);
	fail_if (!hmod, 0, DP (ERROR, "Could not load dll %s\n", dll));
	
	addy = (void *)GetProcAddress (hmod, func);
	fail_if (!addy, 0, DP (ERROR, "Could not find function %s in dll %s\n", dll, func));

	st = addy (a->params, a->numparams, a->vars, a->root->numvars, &a->locals->returnvalue, a->globals, a->root->numglobals, a->scratch);
	fail_if (!st, 0, 0);

	FreeLibrary (hmod);

	return COMMAND_OK;

failure:

	if (hmod)
		FreeLibrary (hmod);

	return COMMAND_FAIL;
}


int ActnCmdIntCompare (actparams_t *a)
{
	STD_SKIP ();

	if (a->vars[PAR (0)] < a->vars[PAR (1)])
		a->locals->equality = -1;
	if (a->vars[PAR (0)] == a->vars[PAR (1)])
		a->locals->equality = 0;
	if (a->vars[PAR (0)] > a->vars[PAR (1)])
		a->locals->equality = 1;

	return COMMAND_OK;
}

int ActnCmdStringCompare (actparams_t *a)
{
	STD_SKIP ();

	__try
	{
		a->locals->equality = strcmp ((char *)a->vars[PAR (0)], (char *)a->vars[PAR (1)]);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception 0x%.8X in cmps", GetExceptionCode ());
	}

	return COMMAND_OK;
}

int ActnCmdStringCompareN (actparams_t *a)
{
	STD_SKIP ();

	__try
	{
		a->locals->equality = strncmp ((char *)a->vars[PAR (0)], (char *)a->vars[PAR (1)], PAR (2));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception 0x%.8X in cmps", GetExceptionCode ());
	}

	return COMMAND_OK;
}

int ActnCmdStringCompareWide (actparams_t *a)
{
	STD_SKIP ();

	__try
	{
		a->locals->equality = wcscmp ((WCHAR *)a->vars[PAR (0)], (WCHAR *)a->vars[PAR (1)]);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception 0x%.8X in cmps", GetExceptionCode ());
	}

	return COMMAND_OK;
}

int ActnCmdStringCompareWideN (actparams_t *a)
{
	STD_SKIP ();

	__try
	{
		a->locals->equality = wcsncmp ((WCHAR *)a->vars[PAR (0)], (WCHAR *)a->vars[PAR (1)], PAR (2));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception 0x%.8X in cmps", GetExceptionCode ());
	}

	return COMMAND_OK;
}

int ActnCmdStringCompareAnsiWide (actparams_t *a)
{
	WCHAR *buffer;
	int len;
	int st;

	STD_SKIP ();

	__try
	{
		len = (strlen ((char *)a->vars[PAR (0)]) + 1);
		buffer = _alloca (len * sizeof (WCHAR));
		st = MultiByteToWideChar (CP_ACP, 0, (char *)a->vars[PAR (0)], -1, buffer, len);
		if (!st)
			return COMMAND_OK; //do nothing if error

		a->locals->equality = wcscmp (buffer, (WCHAR *)a->vars[PAR (1)]);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception 0x%.8X in cmps", GetExceptionCode ());
	}

	return COMMAND_OK;
}

int ActnCmdStringCompareAnsiWideN (actparams_t *a)
{
	WCHAR *buffer;
	int len;
	int st;

	STD_SKIP ();

	__try
	{
		len = (strlen ((char *)a->vars[PAR (0)]) + 1);
		buffer = _alloca (len * sizeof (WCHAR));
		st = MultiByteToWideChar (CP_ACP, 0, (char *)a->vars[PAR (0)], -1, buffer, len);
		if (!st)
			return COMMAND_OK; //do nothing if error

		IsmDprintf (NOISE, "crap: %ws %ws\n", buffer, (WCHAR *)a->vars[PAR (1)]);

		a->locals->equality = wcsncmp (buffer, (WCHAR *)a->vars[PAR (1)], PAR (2));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception 0x%.8X in cmps", GetExceptionCode ());
	}

	return COMMAND_OK;
}

int ActnCmdJump (actparams_t *a)
{
	STD_SKIP ();

	a->locals->skipping = PAR (0);

	return COMMAND_RESTART;
}

int ActnCmdJeq (actparams_t *a)
{
	STD_SKIP ();

	if (a->locals->equality == 0)
	{
		a->locals->skipping = PAR (0);
		return COMMAND_RESTART;
	}

	return COMMAND_OK;
}

int ActnCmdJne (actparams_t *a)
{
	STD_SKIP ();

	if (a->locals->equality != 0)
	{
		a->locals->skipping = PAR (0);
		return COMMAND_RESTART;
	}

	return COMMAND_OK;
}

int ActnCmdJgt (actparams_t *a)
{
	STD_SKIP ();

	if (a->locals->equality > 0)
	{
		a->locals->skipping = PAR (0);
		return COMMAND_RESTART;
	}

	return COMMAND_OK;
}

int ActnCmdJlt (actparams_t *a)
{
	STD_SKIP ();

	if (a->locals->equality < 0)
	{
		a->locals->skipping = PAR (0);
		return COMMAND_RESTART;
	}

	return COMMAND_OK;
}

int ActnCmdJge (actparams_t *a)
{
	STD_SKIP ();

	if (a->locals->equality >= 0)
	{
		a->locals->skipping = PAR (0);
		return COMMAND_RESTART;
	}

	return COMMAND_OK;
}

int ActnCmdJle (actparams_t *a)
{
	STD_SKIP ();

	if (a->locals->equality <= 0)
	{
		a->locals->skipping = PAR (0);
		return COMMAND_RESTART;
	}

	return COMMAND_OK;
}

int ActnCmdLoad (actparams_t *a)
{
	STD_SKIP ();

	__try
	{
		a->vars[PAR (0)] = *(int *)(a->vars[PAR (1)]);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception 0x%.8X in ActnCommandLoad", GetExceptionCode ());
		a->vars[PAR (0)] = 0;
	}

	return COMMAND_OK;
}

int ActnCmdStore (actparams_t *a)
{
	STD_SKIP ();

	__try
	{
		*(int *)(a->vars[PAR (0)]) = a->vars[PAR (1)];
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception 0x%.8X in ActnCommandLoad", GetExceptionCode ());
	}

	return COMMAND_OK;
}

int ActnCmdSetString (actparams_t *a)
{
	STD_SKIP ();

	a->vars[PAR (0)] = (int)(((char *)a->self->data) + sizeof (int));

	return COMMAND_OK;
}

int ActnCmdMoveFromGlobal (actparams_t *a)
{
	STD_SKIP ();

	a->vars[PAR (0)] = a->globals[PAR (1)];

	return COMMAND_OK;
}

int ActnCmdMoveToGlobal (actparams_t *a)
{
	STD_SKIP ();

	a->globals[PAR (0)] = a->vars[PAR (1)];

	return COMMAND_OK;
}

#define ActnCmdNumGlobals ActnCmdNop
#define ActnCmdBufferSize ActnCmdNop

int ActnCmdGetBuffer (actparams_t *a)
{
	STD_SKIP ();

	a->vars[PAR (0)] = (int)a->scratch;

	return COMMAND_OK;
}

int ActnCmdStrncat (actparams_t *a)
{
	char *s1, *s2;

	STD_SKIP ();

	__try
	{

		s1 = (char *)a->vars[PAR (0)];
		s2 = (char *)a->vars[PAR (1)];

		if (strlen (s1) + strlen (s2) + 1 > (unsigned)PAR (2))
		{
			IsmDprintf (ERROR, "String buffer length exceeded (%i > %i)\n", strlen (s1) + strlen (s2) + 1,  PAR (2));
			return COMMAND_FAIL;
		}

		strcat (s1, s2);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception in strncat, 0x%.8X\n", GetExceptionCode ());
		return COMMAND_FAIL;
	}

	return COMMAND_OK;
}

int ActnCmdStrncatWide (actparams_t *a)
{
	WCHAR *s1, *s2;

	STD_SKIP ();

	__try
	{

		s1 = (WCHAR *)a->vars[PAR (0)];
		s2 = (WCHAR *)a->vars[PAR (1)];

		if (((wcslen (s1) + wcslen (s2) + 1) * sizeof (WCHAR)) > (unsigned)(PAR (2)))
		{
			IsmDprintf (ERROR, "String buffer length exceeded (%i > %i)\n", (wcslen (s1) + wcslen (s2) + 1) * sizeof (WCHAR),  PAR (2));
			return COMMAND_FAIL;
		}

		wcscat (s1, s2);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception in strncatw, 0x%.8X\n", GetExceptionCode ());
		return COMMAND_FAIL;
	}

	return COMMAND_OK;
}

int ActnCmdIntncat (actparams_t *a)
{
	char *s1, *s2;
	char buffer[40];

	STD_SKIP ();

	PlSnprintf (buffer, 39, "%i", PAR (1));
	buffer[39] = '\0';
	s2 = buffer;

	__try
	{
		
		s1 = (char *)a->vars[PAR (0)];

		if (strlen (s1) + strlen (s2) + 1 > (unsigned)PAR (2))
		{
			IsmDprintf (ERROR, "String buffer length exceeded (%i > %i)\n", strlen (s1) + strlen (s2) + 1,  PAR (2));
			return COMMAND_FAIL;
		}

		strcat (s1, s2);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception in strncat, 0x%.8X\n", GetExceptionCode ());
		return COMMAND_FAIL;
	}

	return COMMAND_OK;
}

int ActnCmdAtow (actparams_t *a)
{
	WCHAR *target;
	char *source;
	int tlen;
	int st;

	STD_SKIP ();

	target = (WCHAR *)a->vars[PAR (0)];
	tlen = PAR (1);
	source = (char *)a->vars[PAR (2)];

	__try
	{
		st = MultiByteToWideChar (CP_ACP, 0, source, -1, target, tlen / sizeof (WCHAR));
		target[(tlen / sizeof (WCHAR)) - 1] = L'\0';
		if (!st)
			return COMMAND_FAIL; //do nothing if error
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception 0x%.8X in atow\n", GetExceptionCode ());
	}

	return COMMAND_OK;
}

int ActnCmdWtoa (actparams_t *a)
{
	char *target;
	WCHAR *source;
	int tlen;
	int st;

	STD_SKIP ();

	target = (char *)a->vars[PAR (0)];
	tlen = PAR (1);
	source = (WCHAR *)a->vars[PAR (2)];

	__try
	{
		st = WideCharToMultiByte (CP_ACP, 0, source, -1, target, tlen, NULL, NULL);
		target[tlen - 1] = '\0';
		if (!st)
			return COMMAND_FAIL; //do nothing if error
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception 0x%.8X in wtoa\n", GetExceptionCode ());
	}

	return COMMAND_OK;
}

int ActnCmdPrints (actparams_t *a)
{
	STD_SKIP ();

	__try
	{
		OutputDebugString ((char *)a->vars[PAR (0)]);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception 0x%.8X in prints\n", GetExceptionCode ());
	}

	return COMMAND_OK;
}

int ActnCmdPrintsWide (actparams_t *a)
{
	STD_SKIP ();

	__try
	{
		OutputDebugStringW ((WCHAR *)a->vars[PAR (0)]);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception 0x%.8X in printsw\n", GetExceptionCode ());
	}

	return COMMAND_OK;
}

int ActnCmdStrstr (actparams_t *a)
{
	STD_SKIP ();

	__try
	{
		a->vars[PAR (0)] = (int)strstr ((char *)a->vars[PAR (1)], (char *)a->vars[PAR (2)]);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception 0x%.8X in strstr\n", GetExceptionCode ());
	}

	return COMMAND_OK;
}

int ActnCmdStrstrWide (actparams_t *a)
{
	STD_SKIP ();

	__try
	{
		a->vars[PAR (0)] = (int)wcsstr ((WCHAR *)a->vars[PAR (1)], (WCHAR *)a->vars[PAR (2)]);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception 0x%.8X in strstrw\n", GetExceptionCode ());
	}

	return COMMAND_OK;
}

#include "..\common\p5lib.h"
int ActnCmdParseInP5 (actparams_t *a)
{
	char *slotname;
	int st, len;

	STD_SKIP ();


	//noteL GetThreadDesktop returns a handle you don't need to close.
	//(it actually returns the value of a handle win32 keeps open all the time)
	st = PfGetDefaultPipeName (GetThreadDesktop (GetCurrentThreadId ()), NULL, NULL, &len);
	fail_if (!DERR_OK (st), st, 0);

	slotname = _alloca (len);

	st = PfGetDefaultPipeName (GetThreadDesktop (GetCurrentThreadId ()), NULL, slotname, &len);
	fail_if (!DERR_OK (st), st, 0);
	
	st = PfSendMailCommand (slotname, (char *)a->vars[PAR (0)]);
	fail_if (!DERR_OK (st), st, 0);

	return COMMAND_OK;

failure:

	{
		char buffer[1024];
		wsprintf (buffer, "failure: 0x%.8X\n", st);
		OutputDebugString (buffer);
	}

	return COMMAND_OK; //don't abort the parse
}

int ActnCmdMsgbox (actparams_t *a)
{
	STD_SKIP ();

	__try
	{
		a->vars[PAR (0)] = MessageBox (NULL, (char *)a->vars[PAR (1)], (char *)a->vars[PAR (2)], PAR (3));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception 0x%.8X in msgbox\n", GetExceptionCode ());
	}

	return COMMAND_OK;
}

int ActnCmdMsgboxWide (actparams_t *a)
{
	STD_SKIP ();

	__try
	{
		a->vars[PAR (0)] = MessageBoxW (NULL, (WCHAR *)a->vars[PAR (1)], (WCHAR *)a->vars[PAR (2)], PAR (3));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IsmDprintf (ERROR, "Exception 0x%.8X in msgboxw\n", GetExceptionCode ());
	}

	return COMMAND_OK;
}

int ActnCmdGetGlobals (actparams_t *a)
{
	STD_SKIP ();

	a->vars[PAR (0)] = (int)a->globals;

	return COMMAND_OK;
}

int ActnCmdAnd (actparams_t *a)
{
	STD_SKIP ();

	a->vars[PAR (0)] = a->vars[PAR (0)] & a->vars[PAR (1)];

	return COMMAND_OK;
}

int ActnCmdOr (actparams_t *a)
{
	STD_SKIP ();

	a->vars[PAR (0)] = a->vars[PAR (0)] | a->vars[PAR (1)];

	return COMMAND_OK;
}

int ActnCmdNot (actparams_t *a)
{
	STD_SKIP ();

	a->vars[PAR (0)] = ~a->vars[PAR (0)];

	return COMMAND_OK;
}

int ActnCmdSetLastError (actparams_t *a)
{
	STD_SKIP ();

	SetLastError (PAR (0));

	return COMMAND_OK;
}

int ActnCmdSetLastErrorVar (actparams_t *a)
{
	STD_SKIP ();

	SetLastError (a->vars[PAR (0)]);

	return COMMAND_OK;
}
	
int ActnCmdUpcaseString (actparams_t *a)
{
	STD_SKIP ();

	__try
	{
		_strupr ((char *)a->vars[PAR (0)]);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return COMMAND_FAIL;
	}

	return COMMAND_OK;
}

int ActnCmdUpcaseStringWide (actparams_t *a)
{
	STD_SKIP ();

	__try
	{
		_wcsupr ((WCHAR *)a->vars[PAR (0)]);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return COMMAND_FAIL;
	}

	return COMMAND_OK;
}

actioncommand_t ActnCommands[] = 
{
	{"nop",			"",		0,	CMD_NOP,		0, ActnCmdNop},		//0
	{"nops",		"s",	1,	CMD_NOPS,		0, ActnCmdNop},		//1
	{"nopii",		"ii",	2,	CMD_NOPII,		0, ActnCmdNop},		//2
	{"nopv",		"v",	1,	CMD_NOPV,		0, ActnCmdNop},		//3
	{"setreturn",	"i",	1,	CMD_SETRETURN,	0, ActnCmdSetReturn},		//4
	{"run",			"",		0,	CMD_RUN,		0, ActnCmdRun},		//5
	{"label",		"i",	1,	CMD_LABEL,		0, ActnCmdLabel},		//6
	{"print",		"s",	1,	CMD_PRINT,		0, ActnCmdPrint},		//7
	{"mfpar",		"vi",	2,	CMD_MFPAR,		0, ActnCmdMoveFromParameter},		//8
	{"mtpar",		"iv",	2,	CMD_MTPAR,		0, ActnCmdMoveToParameter},			//9
	{"inc",			"v",	1,	CMD_INC,		0, ActnCmdIncrement},		//10
	{"mfret",		"v",	1,	CMD_MFRET,		0, ActnCmdMoveFromReturnValue},		//11
	{"mtret",		"v",	1,	CMD_MTRET,		0, ActnCmdMoveToReturnValue},		//12
	{"mov",			"vv",	2,	CMD_MOV,		0, ActnCmdMove},		//13
	{"set",			"vi",	2,	CMD_SET,		0, ActnCmdSet},		//14
	{"add",			"vv",	2,  CMD_ADD,		0, ActnCmdAdd},		//15
	{"sub",			"vv",	2,  CMD_SUBTRACT,	0, ActnCmdSub},		//16
	{"mul",			"vv",	2,  CMD_MULTIPLY,	0, ActnCmdMul},		//17
	{"div",			"vv",	2,  CMD_DIVIDE,		0, ActnCmdDiv},		//18
	{"calldll",		"iss",	3,	CMD_CALLDLL,	0, ActnCmdCallDll},	//19
	{"calldll2",	"ss",	2,	CMD_CALLDLL2,	0, ActnCmdCallDllSlow},		//20
	{"cmpi",		"vv",	2,	CMD_CMPI,		0, ActnCmdIntCompare},		//21
	{"cmps",		"vv",	2,	CMD_CMPS,		0, ActnCmdStringCompare},	//22
	{"cmpns",		"vvi",	3,	CMD_CMPNS,		0, ActnCmdStringCompareN},	//23
	{"cmpsw",		"vv",	2,	CMD_CMPSW,		0, ActnCmdStringCompareWide},	//24
	{"cmpnsw",		"vvi",	3,	CMD_CMPNSW,		0, ActnCmdStringCompareWideN},	//25
	{"cmpsaw",		"vv",	2,	CMD_CMPSAW,		0, ActnCmdStringCompareAnsiWide},	//26
	{"cmpnsaw",		"vvi",	3,	CMD_CMPNSAW,	0, ActnCmdStringCompareAnsiWideN},	//27
	{"jmp",			"i",	1,	CMD_JMP,		0, ActnCmdJump},	//28
	{"jeq",			"i",	1,	CMD_JEQ,		0, ActnCmdJeq},		//29
	{"jne",			"i",	1,	CMD_JNE,		0, ActnCmdJne},		//30
	{"jgt",			"i",	1,	CMD_JGT,		0, ActnCmdJgt},		//31
	{"jlt",			"i",	1,	CMD_JLT,		0, ActnCmdJlt},		//32
	{"jge",			"i",	1,	CMD_JGE,		0, ActnCmdJge},		//33
	{"jle",			"i",	1,	CMD_JLE,		0, ActnCmdJle},		//34
	{"load",		"vv",	2,	CMD_LOAD,		0, ActnCmdLoad},	//35
	{"store",		"vv",	2,	CMD_STORE,		0, ActnCmdStore},	//36
	{"sets",		"vs",	2,	CMD_SETS,		0, ActnCmdSetString}, //37
	{"mfglob",		"vg",	2,	CMD_MFGLOB,		0, ActnCmdMoveFromGlobal},	//38
	{"mtglob",		"gv",	2,	CMD_MTGLOB,		0, ActnCmdMoveToGlobal},	//39
	{"numglobs",	"g",	1,	CMD_NUMGLOBS,	0, ActnCmdNumGlobals},		//40
	{"bsize",		"b",	1,	CMD_BSIZE,		0, ActnCmdBufferSize},		//41
	{"getbuf",		"v",	1,	CMD_GETBUF,		0, ActnCmdGetBuffer},		//42
	{"strncat",		"vvi",	3,	CMD_STRNCAT,	0, ActnCmdStrncat},		//43
	{"strncatw",	"vvi",	3,	CMD_STRNCATW,	0, ActnCmdStrncatWide},	//44
	{"intncat",		"vii",	3,	CMD_INTNCAT,	0, ActnCmdIntncat},		//45
	{"atow",		"viv",	3,	CMD_ATOW,		0, ActnCmdAtow},		//46
	{"wtoa",		"viv",	3,	CMD_WTOA,		0, ActnCmdWtoa},		//47
	{"prints",		"v",	1,	CMD_PRINTS,		0, ActnCmdPrints},		//48
	{"printsw",		"v",	1,	CMD_PRINTSW,	0, ActnCmdPrintsWide},	//49
	{"strstr",		"vvv",	3,	CMD_STRSTR,		0, ActnCmdStrstr},		//50
	{"strstrw",		"vvv",	3,	CMD_STRSTRW,	0, ActnCmdStrstrWide},	//51
	{"p5",			"v",	1,	CMD_P5,			0, ActnCmdParseInP5},	//52
	{"msgbox",		"vvvi",	4,	CMD_MSGBOX,		0, ActnCmdMsgbox},		//53
	{"msgboxw",		"vvvi",	4,	CMD_MSGBOXW,	0, ActnCmdMsgboxWide},	//54
	{"getglob",		"v",	1,	CMD_GETGLOB,	0, ActnCmdGetGlobals},	//55
	{"and",			"vv",	2,	CMD_AND,		0, ActnCmdAnd},			//56
	{"or",			"vv",	2,	CMD_OR,			0, ActnCmdOr},			//57
	{"not",			"v",	1,	CMD_NOT,		0, ActnCmdNot},			//58
	{"sle",			"i",	1,	CMD_SLE,		0, ActnCmdSetLastError},//59
	{"sle2",		"v",	1,	CMD_SLE2,		0, ActnCmdSetLastErrorVar},	//60
	{"upcase",		"v",	1,	CMD_UPCASE,		0, ActnCmdUpcaseString},	//61
	{"upcasew",		"v",	1,	CMD_UPCASEW,	0, ActnCmdUpcaseStringWide},//62


	{NULL}
};

char *ActnParserUpcaseStringWide (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f62 v%i ", atom[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserUpcaseString (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f61 v%i ", atom[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserSetLastErrorVar (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f60 v%i ", atom[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserSetLastError (parerror_t *pe, int val)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f59 i%i ", val);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserNot (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f58 v%i ", atom[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserOr (parerror_t *pe, char *atom1, char *atom2)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f57 v%i v%i ", atom1[0] - 'A', atom2[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserAnd (parerror_t *pe, char *atom1, char *atom2)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f56 v%i v%i ", atom1[0] - 'A', atom2[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserGetGlobals (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f55 v%i ", atom[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserMsgboxWide (parerror_t *pe, char *atom1, char *atom2, char *atom3, int props)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f54 v%i v%i v%i i%i ", atom1[0] - 'A', atom2[0] - 'A', atom3[0] - 'A', props);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserMsgbox (parerror_t *pe, char *atom1, char *atom2, char *atom3, int props)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f53 v%i v%i v%i i%i ", atom1[0] - 'A', atom2[0] - 'A', atom3[0] - 'A', props);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserP5Parse (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f52 v%i ", atom[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserStrstrWide (parerror_t *pe, char *atom1, char *atom2, char *atom3)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f51 v%i v%i v%i ", atom1[0] - 'A', atom2[0] - 'A', atom3[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserStrstr (parerror_t *pe, char *atom1, char *atom2, char *atom3)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f50 v%i v%i v%i ", atom1[0] - 'A', atom2[0] - 'A', atom3[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserPrintsWide (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f49 v%i ", atom[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserPrints (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f48 v%i ", atom[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserWtoa (parerror_t *pe, char *target, int tlen, char *source)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f47 v%i i%i v%i ", target[0] - 'A', tlen, source[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserAtow (parerror_t *pe, char *target, int tlen, char *source)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f46 v%i i%i v%i ", target[0] - 'A', tlen, source[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserIntncat (parerror_t *pe, char *target, int svar, int maxlen)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f45 v%i i%i i%i ", target[0] - 'A', svar, maxlen);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserStrncatWide (parerror_t *pe, char *target, char *source, int maxlen)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f44 v%i v%i i%i ", target[0] - 'A', source[0] - 'A', maxlen);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserStrncat (parerror_t *pe, char *target, char *source, int maxlen)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f43 v%i v%i i%i ", target[0] - 'A', source[0] - 'A', maxlen);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserGetbuf (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f42 v%i ", atom[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserBufferSize (parerror_t *pe, int buf)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f41 b%i ", buf);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserNumGlobals (parerror_t *pe, int numg)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f40 g%i ", numg);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserMtglob (parerror_t *pe, int parindex, char *var)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f39 g%i v%i ", parindex, var[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserMfglob (parerror_t *pe, char *var, int parindex)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f38 v%i g%i ", var[0] - 'A', parindex);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserSetString (parerror_t *pe, char *atom1, char *string)
{
	char buffer[256];
	if (!string)
		string = "";
	PlSnprintf (buffer, 255, "f37 v%i s%i %s ", atom1[0] - 'A', strlen (string), string);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserStore (parerror_t *pe, char *atom1, char *atom2)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f36 v%i v%i ", atom1[0] - 'A', atom2[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserLoad (parerror_t *pe, char *atom1, char *atom2)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f35 v%i v%i ", atom1[0] - 'A', atom2[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserJle (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f34 i%i ", atom[0]);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserJge (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f33 i%i ", atom[0]);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserJlt (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f32 i%i ", atom[0]);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserJgt (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f31 i%i ", atom[0]);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserJne (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f30 i%i ", atom[0]);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserJeq (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f29 i%i ", atom[0]);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserJmp (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f28 i%i ", atom[0]);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserCmpnsAnsiWide (parerror_t *pe, char *atom1, char *atom2, int n)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f27 v%i v%i i%i ", atom1[0] - 'A', atom2[0] - 'A', n);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserCmpsAnsiWide (parerror_t *pe, char *atom1, char *atom2)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f26 v%i v%i ", atom1[0] - 'A', atom2[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserCmpnsWide (parerror_t *pe, char *atom1, char *atom2, int n)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f25 v%i v%i i%i ", atom1[0] - 'A', atom2[0] - 'A', n);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserCmpsWide (parerror_t *pe, char *atom1, char *atom2)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f24 v%i v%i ", atom1[0] - 'A', atom2[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserCmpns (parerror_t *pe, char *atom1, char *atom2, int n)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f23 v%i v%i i%i ", atom1[0] - 'A', atom2[0] - 'A', n);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserCmps (parerror_t *pe, char *atom1, char *atom2)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f22 v%i v%i ", atom1[0] - 'A', atom2[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserCmpi (parerror_t *pe, char *atom1, char *atom2)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f21 v%i v%i ", atom1[0] - 'A', atom2[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserCallDllSlow (parerror_t *pe, char *param1, char *param2)
{
	char buffer[256];
	int st;

	fail_if (!param1, 0, PE_BAD_PARAM (1));
	fail_if (!param2, 0, PE_BAD_PARAM (2));

	//the first parameter MUST be zero or bad things will happen.
	PlSnprintf (buffer, 255, "f20 s%i %s s%i %s ", strlen (param1), param1, strlen (param2), param2);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
failure:
	return NULL;
}
char *ActnParserCallDll (parerror_t *pe, char *param1, char *param2)
{
	char buffer[256];
	int st;

	fail_if (!param1, 0, PE_BAD_PARAM (1));
	fail_if (!param2, 0, PE_BAD_PARAM (2));

	//the first parameter MUST be zero or bad things will happen.
	PlSnprintf (buffer, 255, "f19 i0 s%i %s s%i %s ", strlen (param1), param1, strlen (param2), param2);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
failure:
	return NULL;
}
char *ActnParserDiv (parerror_t *pe, char *atom1, char *atom2)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f18 v%i v%i ", atom1[0] - 'A', atom2[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserMul (parerror_t *pe, char *atom1, char *atom2)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f17 v%i v%i ", atom1[0] - 'A', atom2[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserSub (parerror_t *pe, char *atom1, char *atom2)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f16 v%i v%i ", atom1[0] - 'A', atom2[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserAdd (parerror_t *pe, char *atom1, char *atom2)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f15 v%i v%i ", atom1[0] - 'A', atom2[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserSet (parerror_t *pe, char *atom1, int val)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f14 v%i i%i ", atom1[0] - 'A', val);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserMov (parerror_t *pe, char *atom1, char *atom2)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f13 v%i v%i ", atom1[0] - 'A', atom2[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserMtret (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f12 v%i ", atom[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserMfret (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f11 v%i ", atom[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserInc (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f10 v%i ", atom[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserMtpar (parerror_t *pe, int parindex, char *var)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f9 i%i v%i ", parindex, var[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserMfpar (parerror_t *pe, char *var, int parindex)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f8 v%i i%i ", var[0] - 'A', parindex);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserPrint (parerror_t *pe, char *param)
{
	char buffer[256];
	if (!param)
		param = "";
	PlSnprintf (buffer, 255, "f7 s%i %s ", strlen (param), param);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserLabel (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f6 i%i ", atom[0]);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserRun (parerror_t *pe)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f5 ");
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserSetReturn (parerror_t *pe, int val)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f4 i%i ", val);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserNop (parerror_t *pe)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f0 ");
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserNopstring (parerror_t *pe, char *param)
{
	char buffer[256];
	if (!param)
		param = "";
	PlSnprintf (buffer, 255, "f1 s%i %s ", strlen (param), param);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserNopintint (parerror_t *pe, int val, int val2)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f2 i%i i%i ", val, val2);
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}
char *ActnParserNopvar (parerror_t *pe, char *atom)
{
	char buffer[256];
	PlSnprintf (buffer, 255, "f3 v%i ", atom[0] - 'A');
	buffer[255] = '\0';
	return ActnCopyBuffer (pe, buffer);
}


DWORD ActnExecute (action_t *action, DWORD(*realfunc)(void *), void *funcdata, void **params, int numparams)
{
	actparams_t apar = {action, NULL, realfunc, funcdata, NULL, NULL, NULL, params, numparams, NULL};
	actdata_t ad = {0};
	actionitem_t *ai;
	char *scratchbuffer = NULL;
	int *vars;
	int st;

	ai = (actionitem_t *)action->items;

	//allcate local space for the vars.
	vars = _alloca (action->numvars * sizeof (int));
	memset (vars, 0, action->numvars * sizeof (int));

	if (action->scratchsize)
	{
		scratchbuffer = _alloca (action->scratchsize);
		*scratchbuffer = '\0';
	}

	apar.locals = &ad;
	apar.vars = vars;
	apar.globals = (int *)(((char *)action->items) + action->globalofs);
	apar.scratch = scratchbuffer;

	while (ai->size)
	{
		apar.self = ai;

		//IsmDprintf (NOISE, "Executing: %s [%i %i]\n", ActnCommands[ai->type].name, ad.skipping, ad.equality);

		if (ActnCommands[ai->type].doaction)
		{
			st = ActnCommands[ai->type].doaction (&apar);

			if (st == COMMAND_FAIL)
				return ad.returnvalue; //not much else to do

			if (st == COMMAND_RESTART)
			{
				ai = (actionitem_t *)action->items;
				continue;
			}
		}
			

		ai = (actionitem_t *)(((char *)ai) + ai->size);
	}

	return ad.returnvalue;
}





//								***********************************************************
//
//											Parsing functions (manager level only)
//
//								***********************************************************


int ActnBufferAdd (char *buffer, int *ofsio, void *data, int len)
{
	int ofs = *ofsio;

	if (ofs + len > MAX_INSTRUMENT_SIZE)
		return 0;

	memcpy (buffer + ofs, data, len);

	ofs += len;

	*ofsio = ofs;

	return 1;
}




//the string looks like a sequence of cXXXXXX, where c is a character and x is a number
//the string is space deliminated.
//chars:
//  f - a function code (index into ActnCommands)
//  i - an integer
//  v - a variable (integer; set to atom[0] - 'A' by the individual funcs
//  s - a string.  The int after the S specifies how many chars are part of the string.
//	b - size of the scratch buffer. (the command this is attached to does nothing.
//  g - a global variable.  
//
//f must be first
//so, for example, assume function 3 takes an int, a string, and a var.:
//assume the user gave 3475 as the int, "shit!" as the string, and A as the var
//f3 i3475 s5 shit! v0

#define ActnNextParam(a) while (*a && *a != ' ') a++; if (*a) a++;
action_t *ActnParseString (char *string, parerror_t *pe)
{
	char buffer[MAX_INSTRUMENT_SIZE];
	action_t *act;
	actionitem_t *item;
	int ofs;
	int st;
	char *walk;
	unsigned int val;
	actioncommand_t *ac;
	int maxvar = -1;
	int maxglobal = -1;
	int stringsize = 0;
	int x;

	//be careful with this param in the failure: clause
	act = PlAlloca (sizeof (action_t));

	ofs = 0;
	st = ActnBufferAdd (buffer, &ofs, act, sizeof (action_t));
	fail_if (!st, 0, PE ("Actions string too large", 0, 0, 0, 0));

	walk = string;

	while (*walk == 'f')
	{
		val = atoi (walk + 1);
		fail_if (val >= CMD_MAX, 0, PE ("Malformed action string (unknown command %i)", val, 0, 0, 0));

		ActnNextParam (walk);

		item = (actionitem_t *)(buffer + ofs);

		st = ActnBufferAdd (buffer, &ofs, item, sizeof (actionitem_t)); //this call is sort of silly, but kept in place for buffer overflow checking
		fail_if (!st, 0, PE ("Actions string too large", 0, 0, 0, 0));

		ac = ActnCommands + val;
		for (x=0;x<ac->numparams;x++)
		{
			//this has the nice side effect of aborting if the string has ended
			fail_if (*walk != ac->params[x], 0, PE ("Malformed action string (command %i param %i is of type \'%c\', not \'%c\')", ac - ActnCommands, x, ac->params[x], *walk ? *walk : '0'));

			val = atoi (walk + 1);
			switch (*walk)
			{
			case 'b':
				stringsize = val;
				break;

			case 'g':
				if ((signed)val > maxglobal)
					maxglobal = val;

				st = ActnBufferAdd (buffer, &ofs, &val, sizeof (int));
				fail_if (!st, 0, PE ("Actions string too large", 0, 0, 0, 0));
				break;

			case 'v':
				if ((signed)val > maxvar)
					maxvar = val;
				//A-Z are the only valid vars.
				fail_if (val >= 26, 0, PE ("Invalid variable detected (%i)", val, 0, 0, 0));
				//fall through

			case 'i':
				st = ActnBufferAdd (buffer, &ofs, &val, sizeof (int));
				fail_if (!st, 0, PE ("Actions string too large", 0, 0, 0, 0));
				break;

			case 's':
				ActnNextParam (walk);
				fail_if (strlen (walk) < val, 0, PE ("Malformed action string (string param %i too short (%i < %i)", x, strlen (walk), val, 0));
				st = ActnBufferAdd (buffer, &ofs, walk, val);
				fail_if (!st, 0, PE ("Actions string too large", 0, 0, 0, 0));
				st = ActnBufferAdd (buffer, &ofs, "", 1); //add a null too
				fail_if (!st, 0, PE ("Actions string too large", 0, 0, 0, 0));

				walk += val;

				fail_if (*walk != ' ', 0, PE ("Malformed action string (string param missing trailing space)", 0, 0, 0, 0));

				break;
			}

			//Past the parameter we just consumed
			ActnNextParam (walk);

			//all ready for the next iteration
		}

		//set the item params.
		item->type = ac - ActnCommands;
		val = ofs - (((char *)item) - buffer);
		item->size = (((val - 1) >> 2) + 1) << 2; //round up to sizeof(int) boundary

		st = 0;
		st = ActnBufferAdd (buffer, &ofs, &st, item->size - val); //add the padding
		fail_if (!st, 0, PE ("Actions string too large", 0, 0, 0, 0));
	}

	fail_if (*walk, 0, PE ("Malformed action string (bad number of params to a function or function without \'f\')", 0, 0, 0, 0));

	//this terminates the list.  THe execute function sees it as an actionitem_t with
	//a size of zero.
	st = 0;
	st = ActnBufferAdd (buffer, &ofs, &st, sizeof (int));
	fail_if (!st, 0, PE ("Actions string too large", 0, 0, 0, 0));

	//maxvars also needs incrementing
	maxvar++;

	//maxglobals needs to be incremented to be correct
	maxglobal++;
	fail_if (ofs + (sizeof (int) * maxglobal) > MAX_INSTRUMENT_SIZE, 0, PE ("Action string is too large", 0, 0, 0, 0));

	act = IsmMalloc (ofs + (sizeof (int) * maxglobal));
	fail_if (!act, 0, PE_OUT_OF_MEMORY (ofs + (sizeof (int) * maxglobal)));
	//can't fail beneath this point without ajusting some of the code above.
	//(act is non-null from the very beginning)

	memcpy (act, buffer, ofs);

	act->size = ofs + (sizeof (int) * maxglobal);
	act->numvars = maxvar;
	act->numglobals = maxglobal;
	act->scratchsize = stringsize;
	act->globalofs = ofs - sizeof (action_t);

	memset (act->items + act->globalofs, 0, sizeof (int) * maxglobal);

	return act;

failure:

	return NULL;
}


		









