/* Copyright 2008 wParam, licensed under the GPL */

#include <windows.h>
#include "p5.h"
#include "..\common\wp_scheme2.h"
#include <stdio.h>

#define PARSER_OUT_OF_MEMORY			DERRCODE (PARSER_DERR_BASE, 0x01)
#define PARSER_HEAP_CREATE_FAILED		DERRCODE (PARSER_DERR_BASE, 0x02)
#define PARSER_SCHEME_CREATE_FAILED		DERRCODE (PARSER_DERR_BASE, 0x03)
#define PARSER_WTF						DERRCODE (PARSER_DERR_BASE, 0x04)
#define PARSER_CREATE_SEMAPHORE_FAILED	DERRCODE (PARSER_DERR_BASE, 0x05)
#define PARSER_FUNCTION_IN_USE			DERRCODE (PARSER_DERR_BASE, 0x06)
#define PARSER_NAME_COLLISION			DERRCODE (PARSER_DERR_BASE, 0x07)
#define PARSER_TLS_PARSER_INVALID		DERRCODE (PARSER_DERR_BASE, 0x08)
#define PARSER_FINDFUNC_IS_INSANE		DERRCODE (PARSER_DERR_BASE, 0x09)
#define PARSER_TLS_ALLOC_FAIL			DERRCODE (PARSER_DERR_BASE, 0x0A)
#define PARSER_ATOM_NOT_FOUND			DERRCODE (PARSER_DERR_BASE, 0x0B)
#define PARSER_FUNC_NOT_FOUND			DERRCODE (PARSER_DERR_BASE, 0x0C)
#define PARSER_FUNCTION_TYPE_MISMATCH	DERRCODE (PARSER_DERR_BASE, 0x0D)
#define PARSER_TLS_SET_VALUE_FAILED		DERRCODE (PARSER_DERR_BASE, 0x0E)
#define PARSER_SCHEME_ERROR				DERRCODE (PARSER_DERR_BASE, 0x0F)
#define PARSER_TLS_GET_VALUE_FAILED		DERRCODE (PARSER_DERR_BASE, 0x10)
#define PARSER_ARGUMENT_LENGTH_ERROR	DERRCODE (PARSER_DERR_BASE, 0x11)
#define PARSER_INTERNAL_LIST_ERROR		DERRCODE (PARSER_DERR_BASE, 0x12)


char *ParErrors[] = 
{
	NULL,
	"PARSER_OUT_OF_MEMORY",
	"PARSER_HEAP_CREATE_FAILED",
	"PARSER_SCHEME_CREATE_FAILED",
	"PARSER_WTF",
	"PARSER_CREATE_SEMAPHORE_FAILED",
	"PARSER_FUNCTION_IN_USE",
	"PARSER_NAME_COLLISION",
	"PARSER_TLS_PARSER_INVALID",
	"PARSER_FINDFUNC_IS_INSANE",
	"PARSER_TLS_ALLOC_FAIL",
	"PARSER_ATOM_NOT_FOUND",
	"PARSER_FUNC_NOT_FOUND",
	"PARSER_FUNCTION_TYPE_MISMATCH",
	"PARSER_TLS_SET_VALUE_FAILED",
	"PARSER_SCHEME_ERROR",
	"PARSER_TLS_GET_VALUE_FAILED",
	"PARSER_ARGUMENT_LENGTH_ERROR",
	"PARSER_INTERNAL_LIST_ERROR",
};

char *ParGetErrorString (int errindex)
{
	int numerrors = sizeof (ParErrors) / sizeof (char *);

	if (errindex >= numerrors || !ParErrors[errindex])
		return "PARSER_UNKNOWN_ERROR_CODE";

	return ParErrors[errindex];
}






static HANDLE ParHeap = NULL;
//static DWORD ParTlsCurrentParser = 0xFFFFFFFF;
//static DWORD ParTlsCurrentDefine = 0xFFFFFFFF;


typedef struct paratom_t
{
	char *atom;
	char *value;
	int flags;
} paratom_t;

typedef struct help_t
{
	char *desc;
	char *usage;
	char *more;
} help_t;

//This struct holds the things that defines need that functions don't.
typedef struct define_t
{
	char **params;		//prototype followed by the names of each of the parameters
	int count;			//includes an extra one for the prototype itself
						//note that this is the given prototype in the define, NOT
						//the proto used for the function itself.  (The func itself
						//adds an 'n' and an 'e' to the beginning of the list.)
						//the parameters element is all one block; ParFree (parameters)
						//is all you need to do.

	char *defineaction;	//what is executed when you call the define.
} define_t;

typedef struct parfunc_t
{
	func_t func;

	int refcount;
	
	help_t *help;

	define_t *define;	//nonzero for defined functions
	char *alias;		//nonzero for aliases.  Aliases have a NULL function pointer, so don't return them to wp_scheme2.c

	struct parfunc_t *next;
	struct parser_s *parser; //needed so that releases lock the right parser
} parfunc_t;

#define FUNCARRAY_DELTA 10
#define ATOMARRAY_DELTA 10

typedef struct
{
	parfunc_t **array;
	int count;
	int width; //available slots in the array
} functions_t;

typedef struct
{
	paratom_t *array;
	int count;
	int width;
} atoms_t;

typedef struct defineparams_t
{
	define_t *def; //stores the names they map to and the count

	//first entry in this array is always NULL.
	char **params;

	struct defineparams_t *next;
} defineparams_t;


struct parser_s
{
	scheme_t *scheme;

	CRITICAL_SECTION guard; //guards funcs and atoms, and dps.
	functions_t functions;
	atoms_t atoms;
	defineparams_t *defpars; //do not access outside of the guard

	struct parser_s *parent;



};








#define ParAssert(cond, print) if (!(cond)) { if (MessageBox (NULL, print "\r\n\r\n Press cancel to force a breakpoint", "ParAssert failure", MB_OKCANCEL) == IDCANCEL) { __asm {int 3} } }

#ifdef _DEBUG
static int ParDegrade = 0;
#endif

void ParSetDegrade (int new)
{
#ifdef _DEBUG
	ParDegrade = new;
#endif
}

void *ParMalloc (int size)
{
#ifdef _DEBUG
	if (ParDegrade && ((rand () % 100) < ParDegrade))
		return NULL; //simulate low mem
#endif

	return HeapAlloc (ParHeap, 0, size);
}

void ParFree (void *block)
{
	HeapFree (ParHeap, 0, block);
}

void *ParRealloc (void *block, int size)
{
	return HeapReAlloc (ParHeap, 0, block, size);
}

int ParHeapCount (void)
{
	PROCESS_HEAP_ENTRY phe = {0};
	int count = 0;
	int busycount = 0;

	while (HeapWalk (ParHeap, &phe))
	{
		if (phe.wFlags & PROCESS_HEAP_ENTRY_BUSY)
			busycount++;

		count++;
	}

	return busycount;
}






static int ParFindFuncFunction (char *name, char *proto, unsigned long param, func_t **out);
static int ParReleaseFuncFunction (func_t *func, unsigned long param);
int ParBindAtomFunction (atom_t *atom, unsigned long param, int *changed);

DERR ParCreate (parser_t **parout, parser_t *parent)
{
	parser_t *par;
	int res;
	int st;
	int guard = 0;

	par = ParMalloc (sizeof (parser_t));
	fail_if (!par, PARSER_OUT_OF_MEMORY, DI (sizeof (parser_t)));

	memset (par, 0, sizeof (parser_t));

	par->parent = parent;

	res = SchCreate (&par->scheme);
	fail_if (!DERR_OK (res), PARSER_SCHEME_CREATE_FAILED, DI (res));

	SchSetFindFunc (par->scheme, ParFindFuncFunction);
	SchSetReleaseFunc (par->scheme, ParReleaseFuncFunction);
	SchSetAtomBind (par->scheme, ParBindAtomFunction);
	SchSetParam (par->scheme, (unsigned long)par);

	res = PfSaneInitCriticalSection (&par->guard);
	fail_if (!DERR_OK (res), PARSER_CREATE_SEMAPHORE_FAILED, DIGLE);
	guard = 1;

	par->atoms.array = ParMalloc (sizeof (paratom_t) * ATOMARRAY_DELTA);
	fail_if (!par->atoms.array, PARSER_OUT_OF_MEMORY, DI (sizeof (paratom_t) * ATOMARRAY_DELTA));
	memset (par->atoms.array, 0, sizeof (paratom_t) * ATOMARRAY_DELTA);
	par->atoms.count = 0;
	par->atoms.width = ATOMARRAY_DELTA;

	par->functions.array = ParMalloc (sizeof (parfunc_t *) * FUNCARRAY_DELTA);
	fail_if (!par->functions.array, PARSER_OUT_OF_MEMORY, DI (sizeof (parfunc_t *) * FUNCARRAY_DELTA));
	memset (par->functions.array, 0, sizeof (parfunc_t *) * FUNCARRAY_DELTA);
	par->functions.count = 0;
	par->functions.width = FUNCARRAY_DELTA;

	*parout = par;

	return PF_SUCCESS;
failure:

	if (par && par->functions.array)
		ParFree (par->functions.array);

	if (par && par->atoms.array)
		ParFree (par->atoms.array);

	if (par && par->scheme)
		SchDestroy (par->scheme);

	if (par && guard)
		DeleteCriticalSection (&par->guard);

	if (par)
		ParFree (par);

	return st;
}


static void ParFreeFunction (parfunc_t *pf);
static void ParFreeAtom (paratom_t *pa);
DERR ParDestroy (parser_t *par)
{
	int x;
	parfunc_t *walk, *temp;
	defineparams_t *dpw, *dpt;

	if (!par)
		return PARSER_WTF;

	ParReset (par);

	for (x=0;x<par->functions.count;x++)
	{
		walk = par->functions.array[x];
		while (walk)
		{
			temp = walk->next;
			ParFreeFunction (walk);
			walk = temp;
		}
	}

	dpw = par->defpars;
	while (dpw)
	{
		dpt = dpw->next;
		ParFree (dpw);
		dpw = dpt;
	}

	ParFree (par->functions.array);

	for (x=0;x<par->atoms.count;x++)
	{
		ParFreeAtom (&par->atoms.array[x]);
	}

	ParFree (par->atoms.array);

	DeleteCriticalSection (&par->guard);
	SchDestroy (par->scheme);
	ParFree (par);

	return PF_SUCCESS;
}


DERR ParInit (void)
{
	int st;

	ParHeap = HeapCreate (0, 1024 * 1024, 0);
	fail_if (!ParHeap, PARSER_HEAP_CREATE_FAILED, DIGLE);

	//ParTlsCurrentParser = TlsAlloc ();
	//fail_if (ParTlsCurrentParser == 0xFFFFFFFF, PARSER_TLS_ALLOC_FAIL, DIGLE);

	//ParTlsCurrentDefine = TlsAlloc ();
	//fail_if (ParTlsCurrentDefine == 0xFFFFFFFF, PARSER_TLS_ALLOC_FAIL, DIGLE);

	return PF_SUCCESS;
failure:

	//if (ParTlsCurrentParser != 0xFFFFFFFF)
	//	TlsFree (ParTlsCurrentParser);

	//if (ParTlsCurrentDefine != 0xFFFFFFFF)
	//	TlsFree (ParTlsCurrentDefine);

	if (ParHeap)
		HeapDestroy (ParHeap);

	ParHeap = NULL;
	//ParTlsCurrentParser = 0xFFFFFFFF;
	//ParTlsCurrentDefine = 0xFFFFFFFF;
	
	return st;
}

void ParDenit (void)
{
	HeapDestroy (ParHeap);
	//TlsFree (ParTlsCurrentParser);
	//TlsFree (ParTlsCurrentDefine);


	ParHeap = NULL;
	//ParTlsCurrentParser = 0xFFFFFFFF;
	//ParTlsCurrentDefine = 0xFFFFFFFF;

}





void ParConvertErrorToString (parerror_t *pe, char *error, int errlen)
{
	char *format;

	if (pe->format)
		format = pe->format;
	else
		format = "No error format string given: %.8X %.8X %.8X %.8X";

	__try
	{
		//use a try catch block on the off-chance that the plugin was unloaded by
		//some other thread
		_snprintf (error, errlen - 1, format, pe->param1, pe->param2, pe->param3, pe->param4);
		error[errlen - 1] = '\0';
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		_snprintf (error, errlen - 1, "Exception 0x%.8X while formatting the error string", GetExceptionCode ());
		error[errlen - 1] = '\0';
	}

	if (pe->flags & PEF_FORMAT)
		ParFree (pe->format);

	if (pe->flags & PEF_PARAM1)
		ParFree ((void *)pe->param1);

	if (pe->flags & PEF_PARAM2)
		ParFree ((void *)pe->param2);

	if (pe->flags & PEF_PARAM3)
		ParFree ((void *)pe->param3);

	if (pe->flags & PEF_PARAM4)
		ParFree ((void *)pe->param4);
}


void ParCleanupError (parerror_t *pe)
{
	if (pe->flags & PEF_FORMAT)
		ParFree (pe->format);

	if (pe->flags & PEF_PARAM1)
		ParFree ((void *)pe->param1);

	if (pe->flags & PEF_PARAM2)
		ParFree ((void *)pe->param2);

	if (pe->flags & PEF_PARAM3)
		ParFree ((void *)pe->param3);

	if (pe->flags & PEF_PARAM4)
		ParFree ((void *)pe->param4);

	memset (pe, 0, sizeof (parerror_t));
}

DERR ParRunLineCurrent (parser_t *cur, char *line, char **output, int *nesting, parerror_t *pe)
{
	scheme_t *backup = NULL;
	//parser_t *cur;
	DERR st;
	int res;

	ParAssert (cur->parent, "Root parser passed to ParRunLineCurrent");

	//cur = TlsGetValue (ParTlsCurrentParser);
	//fail_if (!cur, STATUS_TLS_GET_VALUE_FAILED, DIGLE);
	
	res = SchSaveExpression (cur->scheme, &backup);
	fail_if (res, PARSER_SCHEME_ERROR, PE ("Scheme error running current line: %s", SchGetErrorString (res), 0, 0, 0));

	res = SchParseLine (cur->scheme, line, (error_t *)pe, output);
	
	if (nesting)
		*nesting = SchGetNesting (cur->scheme);

	SchReset (cur->scheme); //always blow away anything that's left, ParUseCurrent runs MUST be complete expressions.

	SchRestoreExpression (cur->scheme, backup);

	//NOW we test and fail if the parse didn't work.
	//note that STATUS_MORE_DATA_REQUIRED _IS_ considered failure in this case
	//also note that if we got STATUS_USER_DEFINED_ERROR here, we don't do any PE() on it.
	//Just pass it back.
	fail_if (res, PARSER_SCHEME_ERROR, if (res != STATUS_USER_DEFINED_ERROR) { PE ("Scheme error running current line: %s", SchGetErrorString (res), 0, 0, 0); } );

	//sucess.
	st = PF_SUCCESS; //fall through
failure:
	return st;
}

void ParProcessErrorType (int st, char *error, int errlen, parerror_t *pe)
{

	//if it's an error that didn't fill in the user stuff, do that now.
	if (GETDERRCODE (st) == PARSER_SCHEME_ERROR && pe->error != STATUS_USER_DEFINED_ERROR)
	{
		pe->format = "Scheme Error: %s,0x%.8X";
		pe->param1 = (int)SchGetErrorString (pe->error);
		pe->param2 = pe->info;
		pe->flags = 0;
	}
	else if (GETDERRCODE (st) == PARSER_TLS_SET_VALUE_FAILED)
	{
		pe->format = "Parsing error: TlsSetValue failed, %i";
		pe->param1 = GETDERRINFO (st);
		pe->flags = 0;
	}

	//this function also frees stuff
	ParConvertErrorToString (pe, error, errlen);
}
	



//currenterror should be NULL except when using 'ParUseCurrent'.
//nesting may be null if you don't care, however, that's the only way "more data needed"
//is reported back, so you should probably read it.
DERR ParRunLine (parser_t *par, char *line, char **output, int *nesting, char *error, int errlen)
{
	parerror_t parerror = {0};
	//parser_t *old = NULL;
	int st, res;

	ParAssert (par->parent, "Root parser passed to ParRunLine");


	//unlike everything else, doing this for the current interpreter is different
	//enough that it gets its own function.
	//if (par == ParUseCurrent)
	//	return ParRunLineCurrent (line, output, nesting, currenterror);

	//old = TlsGetValue (ParTlsCurrentParser);
	//if old is NULL, we have to assume that's the right answer

	//res = TlsSetValue (ParTlsCurrentParser, par);
	//fail_if (!res, PARSER_TLS_SET_VALUE_FAILED, DIGLE);

	res = SchParseLine (par->scheme, line, (error_t *)&parerror, output);
	if (res == STATUS_NEED_MORE_DATA)
		res = STATUS_SUCCESS; //more data needed will show up as nesting != 0
	fail_if (res != STATUS_SUCCESS, PARSER_SCHEME_ERROR, DI (res));

	//if this one fails, it may be really bad.  (But probably only if old is not null, meaning
	//that there is another parser is also in this function somewhere farther up the stack.
	//(Really only a danger when using the bg.sync command to do something time consuming.)
	//res = TlsSetValue (ParTlsCurrentParser, old);
	//if (!res)
	//	PfMessageBoxf (NULL, "Warning: TlsSetValue has failed, trying to set the current parser back to %X, with error code %i.  This could prove fatal, or it could not matter at all.  Even if nothing bad happens, shutting down and restarting p5 is recommended.", "p5 bad bad error", MB_ICONERROR, old, GetLastError ());

	//successful parse, we're done.
	if (error && errlen != 0) //make the error an empty string for good measure
		error[0] = '\0';

	//set nesting if they want it
	if (nesting)
		*nesting = SchGetNesting (par->scheme);

	return PF_SUCCESS;

failure:

	//not much we can do if this is a failure.
	//res = TlsSetValue (ParTlsCurrentParser, old);
	//if (!res)
	//	PfMessageBoxf (NULL, "Warning: TlsSetValue has failed, trying to set the current parser back to %X, with error code %i.  This could prove fatal, or it could not matter at all.  Even if nothing bad happens, shutting down and restarting p5 is recommended.", "p5 bad bad error", MB_ICONERROR, old, GetLastError ());

	if (error)
	{
		ParProcessErrorType (st, error, errlen, &parerror);
	}
	else
	{
		ParCleanupError (&parerror);
	}

	return st;
}


DERR ParReset (parser_t *par)
{
	SchReset (par->scheme);

	return PF_SUCCESS;
}


DERR ParGetHelpData (parser_t *par, helpblock_t **helpout)
{
	helpblock_t *help = NULL, *h;
	char *c;
	int funccount;
	int len, temp;
	parfunc_t *walk;
	int x;
	DERR st;
	int totallen;


	P (&par->guard);

	len = 0;
	funccount = 0;

	//functions first
	for (x=0;x<par->functions.count;x++)
	{
		walk = par->functions.array[x];

		while (walk)
		{
			funccount++;

			len += strlen (walk->func.name) + 1;
			len += strlen (walk->func.prototype) + 1;
			len += strlen (walk->help->desc) + 1;
			len += strlen (walk->help->usage) + 1;
			len += strlen (walk->help->more) + 1;

			walk = walk->next;
		}
	}

	//then atoms
	for (x=0;x<par->atoms.count;x++)
	{
		len += strlen (par->atoms.array[x].atom) + 1;
		len += strlen (par->atoms.array[x].value) + 1;

		funccount++;
	}



	len += sizeof (helpblock_t) * (funccount + 1);

	totallen = len; //save this for sanity checking at the end
	help = ParMalloc (len);
	fail_if (!help, PARSER_OUT_OF_MEMORY, DI (len));

	h = help;
	c = (char *)(help + (funccount + 1));
	len -= sizeof (helpblock_t) * (funccount + 1);
	//len is supposed to be the length remaining between c and the end of the
	//memory block.

	for (x=0;x<par->functions.count;x++)
	{
		walk = par->functions.array[x];

		while (walk)
		{
			h->name = c;
			temp = strlen (walk->func.name) + 1;
			fail_if (temp > len, PARSER_INTERNAL_LIST_ERROR, DI (1));
			strcpy (c, walk->func.name);
			len -= temp;
			c += temp;

			h->proto = c;
			temp = strlen (walk->func.prototype) + 1;
			fail_if (temp > len, PARSER_INTERNAL_LIST_ERROR, DI (2));
			strcpy (c, walk->func.prototype);
			len -= temp;
			c += temp;

			h->description = c;
			temp = strlen (walk->help->desc) + 1;
			fail_if (temp > len, PARSER_INTERNAL_LIST_ERROR, DI (3));
			strcpy (c, walk->help->desc);
			len -= temp;
			c += temp;

			h->usage = c;
			temp = strlen (walk->help->usage) + 1;
			fail_if (temp > len, PARSER_INTERNAL_LIST_ERROR, DI (4));
			strcpy (c, walk->help->usage);
			len -= temp;
			c += temp;

			h->more = c;
			temp = strlen (walk->help->more) + 1;
			fail_if (temp > len, PARSER_INTERNAL_LIST_ERROR, DI (5));
			strcpy (c, walk->help->more);
			len -= temp;
			c += temp;

			if (walk->alias)
				h->type = FUNCTYPE_ALIAS;
			else if (walk->define)
				h->type = FUNCTYPE_DEFINE;
			else
				h->type = FUNCTYPE_NATIVE;

			h++;
			walk = walk->next;
		}
	}

	for (x=0;x<par->atoms.count;x++)
	{
		h->name = c;
		temp = strlen (par->atoms.array[x].atom) + 1;
		fail_if (temp > len, PARSER_INTERNAL_LIST_ERROR, DI (9));
		strcpy (c, par->atoms.array[x].atom);
		len -= temp;
		c += temp;

		h->proto = c;
		temp = strlen (par->atoms.array[x].value) + 1;
		fail_if (temp > len, PARSER_INTERNAL_LIST_ERROR, DI (10));
		strcpy (c, par->atoms.array[x].value);
		len -= temp;
		c += temp;

		h->description = NULL;
		h->more = NULL;
		h->usage = NULL;

		h->type = FUNCTYPE_VARIABLE;

		h++;
	}

	//h should point to the last entry, which should be nulled out
	memset (h, 0, sizeof (helpblock_t));

	//sanity checks
	fail_if (h - help != funccount, PARSER_INTERNAL_LIST_ERROR, DI (6));
	fail_if (len != 0, PARSER_INTERNAL_LIST_ERROR, DI (7));
	fail_if (c - ((char *)help) != totallen, PARSER_INTERNAL_LIST_ERROR, DI (8));

	V (&par->guard);

	*helpout = help;


	return PF_SUCCESS;

failure:

	V (&par->guard);

	if (help)
		ParFree (help);

	*helpout = NULL;

	return st;
}


//************************************************************
//				Functions and atoms
//************************************************************

DERR ParReallocArray (void **target, int x, int *nv, int *nt, int width, int delta)
{
	char *base = *target;
	char *temp;
	int numvalid = *nv;
	int numtotal = *nt;
	int st;
	
	//first handle reallocation
	if (numvalid == numtotal)
	{
		numtotal += delta;
		temp = ParRealloc (base, width * numtotal);
		fail_if (!temp, PARSER_OUT_OF_MEMORY, DI (width * numtotal));
		base = temp;
		
		*nt = numtotal;
	}

	//now move
	memmove (base + (width * (x + 1)), base + (width * x), width * (numvalid - x));

	*target = base;
	(*nv)++;

	return PF_SUCCESS;

failure:
	return st;
}

int ParCompareFunctions (parfunc_t **func1, parfunc_t **func2)
{
	if (!func1 || !*func1)
		return -1;

	if (!func2 || !*func2)
		return 1;

	return strcmp ((*func1)->func.name, (*func2)->func.name);
}


//If no function by the given name exists, *destindex will be the index in
//	the array where it should be added
//If a function by that name but not that prototype exists, *properslot will
//	point to the head of the list where the new entry should go.  (Meaning:
//	allocate a new parfunc_t, set new->next = **properslot, *properslot = new
//	slot is also set to the slot when exactmatch is set
//If an exact match already exists, *exactmatch points to what points to the
//	exact match.
//
//This function needs to be called with the guard locked
static DERR ParFindFunctionLocation (parser_t *par, char *name, char *proto, int *destindex, parfunc_t ***properslot, parfunc_t ***exactmatch)
{
	parfunc_t temp, *ptemp;
	int x;
	int res;
	parfunc_t **slot, **walk;
	

	temp.func.name = name;
	ptemp = &temp;

	*destindex = -1;
	*properslot = NULL;
	*exactmatch = NULL;

	for (x=0;x<par->functions.count;x++)
	{
		res = ParCompareFunctions (par->functions.array + x, &ptemp);
		
		if (res == 0)
			goto name_match; //match; more work is needed
			
		if (res > 0) //meaning temp < x
		{
			*destindex = x;
			return PF_SUCCESS;
		}
	}

	//if we got here, nothing by that name, and it belongs at the end.
	*destindex = x;
	return PF_SUCCESS;

name_match:

	walk = slot = par->functions.array + x;
	*properslot = slot;

	while (*walk)
	{
		if (strcmp ((*walk)->func.prototype, proto) == 0)
		{
			//exact match
			*exactmatch = walk;
			return PF_SUCCESS;
		}

		walk = &((*walk)->next);
	}

	//didn't find an exact match
	//properslot is already set
	//*properslot = slot;

	return PF_SUCCESS;
}

static help_t *ParMakeHelp (char *desc, char *usage, char *more)
{
	int len;
	help_t *out;
	char *temp;
	int st;

	len = sizeof (help_t) + (strlen (desc) + 1) + (strlen (usage) + 1) + (strlen (more) + 1);

	out = ParMalloc (len);
	fail_if (!out, PARSER_OUT_OF_MEMORY, DI (len));

	temp = ((char *)out) + sizeof (help_t);

	strcpy (temp, desc);
	out->desc = temp;
	temp += strlen (temp) + 1;

	strcpy (temp, usage);
	out->usage = temp;
	temp += strlen (temp) + 1;

	strcpy (temp, more);
	out->more = temp;

	return out;
failure:
	return NULL;
}
	
	

//frees all mem associated with pf AND pf itself
static void ParFreeFunction (parfunc_t *pf)
{
	if (!pf)
		return;

	if (pf->func.name)
		ParFree (pf->func.name);

	if (pf->func.prototype)
		ParFree (pf->func.prototype);

	if (pf->help)
		ParFree (pf->help); //the strings are in the same memory block

	if (pf->define)
		ParFree (pf->define);

	if (pf->alias)
		ParFree (pf->alias);

	ParFree (pf);
}
		

//defineinfo will NOT be freed by this function if it returns failure.
//neither will alias.  
static DERR ParRealAddFunction (parser_t *par, char *name, char *proto, void *addy, char *help_desc, char *help_usage, char *help_more, void *defineinfo, char *alias)
{
	parfunc_t **slot, **match, *f = NULL, *temp;
	int index, res;
	int insection = 0;
	int st;


	P (&par->guard);
	insection = 1;

	ParFindFunctionLocation (par, name, proto, &index, &slot, &match);

	//We are creating f first, because once we call ParReallocArray, we can't fail.
	//(not without considerable difficulty, anyways.)

	f = ParMalloc (sizeof (parfunc_t));
	fail_if (!f, PARSER_OUT_OF_MEMORY, DI (sizeof (parfunc_t)));
	memset (f, 0, sizeof (parfunc_t));

	res = strlen (name);
	f->func.name = ParMalloc (res + 1);
	fail_if (!f->func.name, PARSER_OUT_OF_MEMORY, DI (res + 1));
	strcpy (f->func.name, name);

	res = strlen (proto);
	f->func.prototype = ParMalloc (res + 1);
	fail_if (!f->func.prototype, PARSER_OUT_OF_MEMORY, DI (res + 1));
	strcpy (f->func.prototype, proto);

	f->func.address = addy;
	f->parser = par;

	f->help = ParMakeHelp (help_desc, help_usage, help_more);
	fail_if (!f->help, PARSER_OUT_OF_MEMORY, DI (sizeof (help_t)));

	//ok, f is successfully created.

	//Ok.  Decide what to do, based on what was found.  In the end, slot needs
	//to point to the slot where the function will go.

	if (match)
	{
		//first and foremost, if the function is in use, or it is a built in function,
		//fail.  (Plugin provided functions cannot be overridden by defines.)

		fail_if (!defineinfo, PARSER_NAME_COLLISION, 0);
		fail_if ((*match)->refcount, PARSER_FUNCTION_IN_USE, DI ((*match)->refcount));

		//unlink it, and free it.
		temp = *match;
		*match = (*match)->next;
		ParFreeFunction (temp);

		//slot is already set correctly
	}
	else if (slot)
	{
		//nothing to do
		;
	}
	else
	{
		fail_if (index == -1, PARSER_FINDFUNC_IS_INSANE, 0);

		//now we need to reallocate the list.

		res = ParReallocArray ((void **)&par->functions.array, index, &par->functions.count, &par->functions.width, sizeof (func_t *), FUNCARRAY_DELTA);
		fail_if (!DERR_OK (res), res, 0);

		slot = par->functions.array + index;
		*slot = NULL;
	}

	//link it and return

	//don't set this until we're in a "no fail zone" at the end; if the function fails with this set,
	//ParFreeFunction will delete it, which is NOT what is supposed to happen.
	f->define = defineinfo;
	f->alias = alias;

	f->next = *slot;
	*slot = f;

	V (&par->guard);

	return PF_SUCCESS;

failure:

	if (f)
		ParFreeFunction (f);

	if (insection)
		V (&par->guard);

	return st;
}


DERR ParAddFunction (parser_t *par, char *name, char *proto, void *addy, char *help_desc, char *help_usage, char *help_more)
{
	return ParRealAddFunction (par, name, proto, addy, help_desc, help_usage, help_more, NULL, NULL);
}

DERR ParAddAlias (parser_t *par, char *name, char *proto, char *target)
{
	char *alias = NULL;
	DERR st;

	st = strlen (target);
	alias = ParMalloc (st + 1);
	fail_if (!alias, PARSER_OUT_OF_MEMORY, DI (st + 1));
	strcpy (alias, target);

	st = ParRealAddFunction (par, name, proto, NULL, target, "", "", NULL, alias);
	fail_if (!DERR_OK (st), st, 0);

	//alias is property of the function now

	return PF_SUCCESS;
failure:
	if (alias)
		ParFree (alias);

	return st;
}

//count is NOT optional
DERR ParSplitString (char *weekdays, int *outcount, char **outarray, char delim)
{
	char *a, *dest;
	int count=0;
	char **out;
	char **word;

	*outcount = 0;

	a = weekdays;
	while (*a)
	{
		if (*a == delim)
			count++;
		a++;
	}
	count++;

	if (!outarray)
	{
		//this is just a query for the size
		*outcount = sizeof (char *) * count + strlen (weekdays) + 1;
		return PF_SUCCESS;
	}

	out = outarray;

	a = weekdays;
	dest = (char *)(out + count);
	word = out;

	*word = dest;
	word++;

	while (*a)
	{
		*dest = *a;
		if (*dest == delim)
		{
			*dest = '\0';
			*word = dest + 1;
			word++;
		}

		dest++;
		a++;
	}
	*dest = *a; //copy the null

	*outcount = count;

	return PF_SUCCESS;

}

DERR ParSplitPrototype (char *weekdays, int *outcount, char **outarray)
{
	return ParSplitString (weekdays, outcount, outarray, ';');
}


void *ParExecuteDefine (parser_t *current, parerror_t *pe, parfunc_t *func, int count, ...);
DERR ParAddDefine (parser_t *par, char *name, char *proto, char *action, char *help_desc, char *help_usage, char *help_more)
{
	DERR st;
	int paramsize;
	int len;
	define_t *def = NULL;
	char *walk;
	char *realproto;


	//get the size of the split prototype
	ParSplitPrototype (proto, &paramsize, NULL);

	len = sizeof (define_t) + paramsize + strlen (action) + 1; //the +2 is for a sentinel byte I'm adding

	def = ParMalloc (len);
	fail_if (!def, PARSER_OUT_OF_MEMORY, DI (len));

	walk = ((char *)def) + sizeof (define_t);
	//split the prototype.  This function can't fail, happily
	ParSplitPrototype (proto, &def->count, (char **)walk);
	def->params = (char **)walk;

	walk += paramsize;

	strcpy (walk, action);
	def->defineaction = walk;

	walk += strlen (action) + 1;
	ParAssert (walk - ((char *)def) == len, "define_t creation in ParAddDefine has gone astray");

	fail_if ((unsigned)def->count != strlen (def->params[0]), PARSER_ARGUMENT_LENGTH_ERROR, 0);

	//form the actual prototype
	realproto = _alloca (strlen (def->params[0]) + 5); //we add "penc" to the proto, hence +4 for that and +1 for the null
	realproto[0] = def->params[0][0];
	strcpy (realproto + 1, "penc");
	strcpy (realproto + 5, def->params[0] + 1);

	st = ParRealAddFunction (par, name, realproto, ParExecuteDefine, help_desc, help_usage, help_more, def, NULL);
	fail_if (!DERR_OK (st), st, 0);

	return PF_SUCCESS;

failure:
	if (def)
		ParFree (def);

	return st;
}






static parfunc_t **ParFindFunction (parser_t *par, char *name, char *proto, int exact, parfunc_t ***slot)
{
	parfunc_t temp, *ptemp;
	parfunc_t **val;

	temp.func.name = name;
	ptemp = &temp;

	val = bsearch (&ptemp, par->functions.array, par->functions.count, sizeof (parfunc_t *), (void *)ParCompareFunctions);
	if (!val)
		return NULL;

	//set this now, before we lose it
	//also: Setting this means that a function with the given name was found.
	//is IS POSSIBLE for slot to be set, but NULL to be returned, in this case
	//it means there was a type mismatch
	if (slot)
		*slot = val; 

	//ok, we found that name, now look for a match
	while (*val)
	{
		if (exact)
		{
			//exact match, used for deleting functions
			
			//the procedure is slightly different for defined functions.
			//They use the same namespace, so there won't ever be a function and
			//a define at the same time, so we should be safe doing this split here.
			//if it's a define, test the define's prototype, not the actual prototype,
			//which has some extra params added to it.

			if ((*val)->define) //a defined function
			{
				if (strcmp ((*val)->define->params[0], proto) == 0)
					return val;
			}
			else //normal function
			{
				if (strcmp ((*val)->func.prototype, proto) == 0)
					return val;
			}
		}
		else
		{
			//fuzzy match, used for calling functions
			if (SchEqualPrototype ((*val)->func.prototype, proto))
				return val;
		}

		val = &((*val)->next);
	}

	return NULL;
}


//userdefined values:
//0 - a built in function
//1 - a user defined function (one whose address is ParExecuteDefine)
//2 - an alias
static DERR ParRealRemoveFunction (parser_t *par, char *name, char *proto, int userdefined)
{
	DERR st;
	parfunc_t **func, *temp, **slot;
	int insection = 0;
	int x;


	//if (par == ParUseCurrent)
	//{
	//	par = (parser_t *)TlsGetValue (ParTlsCurrentParser);
	//	fail_if (!par, PARSER_TLS_PARSER_INVALID, DIGLE);
	//}

	P (&par->guard);
	insection = 1;

	func = ParFindFunction (par, name, proto, 1, &slot);
	fail_if (!func || !*func, PARSER_FUNC_NOT_FOUND, DI (userdefined));

	//make sure the found target matches userdefined or not
	fail_if (userdefined == 0 && ( (*func)->define ||  (*func)->alias), PARSER_FUNCTION_TYPE_MISMATCH, DI (userdefined));
	fail_if (userdefined == 1 && (!(*func)->define ||  (*func)->alias), PARSER_FUNCTION_TYPE_MISMATCH, DI (userdefined));
	fail_if (userdefined == 2 && ( (*func)->define || !(*func)->alias), PARSER_FUNCTION_TYPE_MISMATCH, DI (userdefined));

	//make sure it's not in use
	fail_if ((*func)->refcount, PARSER_FUNCTION_IN_USE, DI ((*func)->refcount));

	//ok then, first unlink and delete the function
	temp = *func;
	*func = (*func)->next;

	ParFreeFunction (temp);

	//now check and see if we need to ajust the array or not

	if (*slot == NULL)
	{
		x = slot - par->functions.array;

		if (x == par->functions.count - 1)
		{
			par->functions.count--;
		}
		else
		{
			par->functions.count--;
			memmove (slot, slot + 1, (par->functions.count - x) * sizeof (parfunc_t *));
		}
	}

	V (&par->guard);

	return PF_SUCCESS;

failure:

	if (insection)
		V (&par->guard);

	return st;
}

DERR ParRemoveFunction (parser_t *par, char *name, char *proto)
{
	return ParRealRemoveFunction (par, name, proto, 0);
}

DERR ParRemoveDefine (parser_t *par, char *name, char *proto)
{
	return ParRealRemoveFunction (par, name, proto, 1);
}

DERR ParRemoveAlias (parser_t *par, char *name, char *proto)
{
	return ParRealRemoveFunction (par, name, proto, 2);
}




static int ParFindFuncGoTime (char *name, char *proto, parser_t *par, func_t **outfunc)
{
	parfunc_t *out = NULL, **temp, **slot = NULL;
	int res;
	char *targetname = name;

	if (!par)
		return FF_NOTFOUND;

	P (&par->guard);

	res = 0; //use res as a repeat counter to catch infinite alias loops.

	//this section of the code is really ugly, but I really don't think I can
	//do too much to clean it up, without changing the break; into a return;,
	//and I want to avoid multiple returns in a block where a lock is held.

	//The basic idea is to search for the function, and if we find it's an
	//alias, search again for the alias' target name.  If we repeat this too
	//many times (currently 10) we call it an infinite loop and break it,
	//returning NOTFOUND.  The loop is broken by setting targetname to NULL.

	while (targetname)
	{
		temp = ParFindFunction (par, targetname, proto, 0, &slot);
		targetname = NULL; //default is not to repeat the loop

		//check to see if this is an alias.
		if (temp && (*temp)->alias)
		{
			//just change the name we're looking for and run the find again.
			//it's ok to reference the string held in the func_t directly because
			//we're holding the parser's guard.  NOTE:  Do not reference this string
			//past the V() line.  If we search the parent parser, we start a search
			//for the original name, not the aliased one.  Setting targetname causes
			//the loop to repeat
			targetname = (*temp)->alias;
			
			if (++res > 10)
			{
				ConWriteStringf ("Warning: Infinite alias loop detected; original command was %s\n", name);
				temp = NULL;
				slot = NULL;
				targetname = NULL; //break the loop
			}
		}
	}

	if (temp)
		out = *temp; //set it if we found something, otherwise leave it NULL

	//take a reference, if we found something
	if (out)
		out->refcount++;

	//and we're done.  Release the guard, and return the func_t.  This is ok,
	//because the function will not be removed until the reference is removed,
	//in the release function below.

	V (&par->guard);

	if (out)
	{
		if (!out->func.address)
		{
			//should never happen, but catch it just in case.
			ConWriteStringf ("Warning: Attempted to return NULL function to parser; command was %s\n", name);
			return FF_NOTFOUND;
		}

		*outfunc = &out->func;
		return FF_FOUND;
	}

	//check the parent parser.
	res =  ParFindFuncGoTime (name, proto, par->parent, outfunc);

	//if NOTFOUND is returned, upgrade it to type mismatch if we had a name match.
	//(If it returned type mismatch, that's what we return, but we upgrade not found
	//because the root parser might not have a func at all, but we have it with a
	//bad prototype.)
	if (res == FF_NOTFOUND)
	{
		if (slot)
			res = FF_TYPEMISMATCH;
	}

	return res;
}

static int ParFindFuncFunction (char *name, char *proto, unsigned long param, func_t **out)
{
	parser_t *par = (parser_t *)param;

	return ParFindFuncGoTime (name, proto, par, out);
}

static int ParReleaseFuncFunction (func_t *func, unsigned long param)
{
	//parser_t *par = (parser_t *)param;
	parfunc_t *f;

	f = (parfunc_t *)func;

	if (f)
	{
		P (&f->parser->guard);
		
		f->refcount--;
		
		V (&f->parser->guard);
	}

	return STATUS_SUCCESS;
}



//************************   atoms    *******************************


int ParCompareAtoms (paratom_t *atom1, paratom_t *atom2)
{
	return strcmp (atom1->atom, atom2->atom);
}


void ParFreeAtom (paratom_t *atom)
{
	if (!atom)
		return;

	if (atom->atom)
		ParFree (atom->atom);

	if (atom->value)
		ParFree (atom->value);
}


DERR ParAddAtom (parser_t *par, char *atom, char *value, int flags)
{
	DERR st, len;
	paratom_t tempatom = {0};
	int insection = 0;
	int x, res;
	paratom_t *newatom = NULL;

	//if (par == ParUseCurrent)
	//{
	//	par = (parser_t *)TlsGetValue (ParTlsCurrentParser);
	//	fail_if (!par, PARSER_TLS_PARSER_INVALID, DIGLE);
	//}

	P (&par->guard);
	insection = 1;

	//first allocate the atom.  Not terribly efficient, but as with the add function code above,
	//once the array is reallocated, it becomes very difficult to fail gracefully

	len = strlen (atom) + 1;
	tempatom.atom = ParMalloc (len);
	fail_if (!tempatom.atom, PARSER_OUT_OF_MEMORY, DI (len));
	strcpy (tempatom.atom, atom);

	len = strlen (value) + 1;
	tempatom.value = ParMalloc (len);
	fail_if (!tempatom.value, PARSER_OUT_OF_MEMORY, DI (len));
	strcpy (tempatom.value, value);

	tempatom.flags = flags;

	//ok, atom is made.  Look for where it belongs
	for (x=0;x<par->atoms.count;x++)
	{
		res = ParCompareAtoms (par->atoms.array + x, &tempatom);

		if (!res) //found an atom with this name in existance
		{
			newatom = par->atoms.array + x;
			break;
		}

		if (res > 0)
			break;
	}

	if (!newatom) //realloc now if one wasn't found
	{
		res = ParReallocArray (&par->atoms.array, x, &par->atoms.count, &par->atoms.width, sizeof (paratom_t), ATOMARRAY_DELTA);
		fail_if (!DERR_OK (res), res, 0);

		//NO FAILURES BENEATH THIS POINT!!!

		newatom = par->atoms.array + x;
	}
	else
	{
		//else one WAS found, and we need to clean it up to avoid a leak.
		ParFree (newatom->atom);
		ParFree (newatom->value);
	}

	newatom->atom = tempatom.atom;
	newatom->flags = tempatom.flags;
	newatom->value = tempatom.value;

	V (&par->guard);

	return PF_SUCCESS;

failure:

	ParFreeAtom (&tempatom);

	if (insection)
		V (&par->guard);

	return st;
}


DERR ParRemoveAtom (parser_t *par, char *name)
{
	DERR st;
	paratom_t *a;
	paratom_t tempatom;
	int insection = 0;
	int x;


	//if (par == ParUseCurrent)
	//{
	//	par = (parser_t *)TlsGetValue (ParTlsCurrentParser);
	//	fail_if (!par, PARSER_TLS_PARSER_INVALID, DIGLE);
	//}

	P (&par->guard);
	insection = 1;

	tempatom.atom = name;
	a = bsearch (&tempatom, par->atoms.array, par->atoms.count, sizeof (paratom_t), (void *)ParCompareAtoms);
	fail_if (!a, PARSER_ATOM_NOT_FOUND, 0);

	ParFreeAtom (a);

	x = a - par->atoms.array;

	if (x == par->atoms.count - 1)
	{
		par->atoms.count--;
	}
	else
	{
		par->atoms.count--;
		memmove (a, a + 1, (par->atoms.count - x) * sizeof (atom_t));
	}

	V (&par->guard);

	return PF_SUCCESS;

failure:

	if (insection)
		V (&par->guard);

	return st;
}

/* NOTE: This function is called by script functions directly, not just by the parser.
 * It is expected that it has no side effects other than potentially changing atom */
static int ParBindAtomFunctionGoTime (atom_t *atom, parser_t *par, int *changed)
{
	int st;
	int insection = 0;
	paratom_t *patom, tempatom;
	char *temp;
	int len;
	defineparams_t *defpar;
	int x;

	if (!par)
		return STATUS_SUCCESS;

	if (!atom->a)
		return 0xf5c7;

	if (atom->a[0] >= '0' && atom->a[0] <= '9')
		return STATUS_SUCCESS;	//we're going to say no atoms start with integers

	P (&par->guard);
	insection = 1;

	patom = NULL;

	//first look through define params.
	defpar = par->defpars;
	while (defpar)
	{
		for (x=1;x<defpar->def->count;x++)
		{
			if (strcmp (atom->a, defpar->def->params[x]) == 0)
			{
				//ok, a match.
				tempatom.value = defpar->params[x];
				patom = &tempatom;
				goto match_found; //break out of both loops
			}
		}

		defpar = defpar->next;
	}
match_found:

	//if we didn't find it in defines, search the normal atoms
	if (!patom)
	{
		tempatom.atom = atom->a;
		patom = bsearch (&tempatom, par->atoms.array, par->atoms.count, sizeof (paratom_t), (void *)ParCompareAtoms);
	}

	//make sure you don't access anything besides "value" in patom
	if (patom)
	{
		len = strlen (patom->value) + 1;
		
		if (atom->size < len)
		{
			//attempt to reallocate
			temp = ParRealloc (atom->a, len);
			fail_if (!temp, STATUS_OUT_OF_MEMORY, 0); //atom has not been changed, so we're good.
			atom->a = temp;

			atom->size = len;
		}

		//strcpy (atom->a, a->atom); //we really could have chosen these names better...
		strcpy (atom->a, patom->value); //indeed--changed.

		atom->len = len;

		//all good.
		*changed = 1;

		//I'm falling through, instead of just doing V() and return here, to keep the
		//critical section logic from getting too spread out.
	}

	V (&par->guard);

	if (patom)
		return STATUS_SUCCESS;

	//else check the parent
	return ParBindAtomFunctionGoTime (atom, par->parent, changed);

failure:
	if (insection)
		V (&par->guard);

	return st;
}


static int ParBindAtomFunction (atom_t *atom, unsigned long param, int *changed)
{
	parser_t *par = (parser_t *)param;

	return ParBindAtomFunctionGoTime (atom, par, changed);
}

DERR ParIsBound (parser_t *current, char *atom)
{
	DERR st;
	int changed = 0;
	atom_t a = {0};
	unsigned int len;
	int val;

	fail_if (!atom, PARSER_WTF, 0);

	len = 512; /* allocating semi-big saves a reallocate later */
	if (strlen (atom) + 1 > len)
		len = strlen (atom) + 1;

	a.a = ParMalloc (len);
	a.size = len;
	memset (a.a, 0, len);
	a.len = 0;
	strcpy (a.a, atom);

	val = ParBindAtomFunctionGoTime (&a, current, &changed);
	fail_if (val != STATUS_SUCCESS, PARSER_SCHEME_ERROR, DI (val));
	fail_if (changed, PARSER_ATOM_NOT_FOUND, 0);
	
	ParFree (a.a);

	return PF_SUCCESS;
failure:
	if (a.a)
		ParFree (a.a);

	return st;
}



//************************************************************
//				Script functions
//************************************************************



void *ParExecuteDefine (parser_t *current, parerror_t *pe, parfunc_t *func, int count, ...)
{
	defineparams_t *defpar = NULL;
	char *output = NULL;
	int len;
	int st;
	va_list va;
	int x;
	char *proto; //makes accessing it a little less painful to type
	void *param; //also used as the return value
	char *walk;
	float ftemp;
	int nesting;

	fail_if (!func->define, 0, PE ("Insane define detected", 0, 0, 0, 0));

	//first count up how much space this will take.
	len = 0;
	proto = func->define->params[0];

	va_start (va, count);
	for (x=1;x<func->define->count;x++) //yes the x-1 is correct. (proto[0] is the return type)
	{
		param = va_arg (va, void *);

		switch (proto[x])
		{
		case 'i':
		case 'f':
			len += 30; //should be plenty;
			break;

		case 'a':
			len += strlen (param) + 1;
			break;

		case 's':
		case 't':
			if (param)
				len += strlen (param) + 1;
			else
				len += 3; //"()"
			break;

		default:
			va_end (va);
			fail_if (TRUE, 0, PE ("Invalid character \'%c\' in prototype", proto[x], 0, 0, 0));
		}
	}
	va_end (va);

	len += sizeof (char *) * func->define->count;
	len += sizeof (defineparams_t);

	//len should now hold everything.
	defpar = ParMalloc (len);
	fail_if (!defpar, 0, PE ("Out of memory, %i bytes", len, 0, 0, 0));

	memset (defpar, 0, len);

	defpar->def = func->define;

	walk = ((char *)defpar) + sizeof (defineparams_t);
	defpar->params = (char **)walk;
	defpar->params[0] = NULL;

	walk += (sizeof (char *) * func->define->count);

	va_start (va, count);
	for (x=1;x<func->define->count;x++)
	{
		param = va_arg (va, void *);

		switch (proto[x])
		{
		case 'i':
			_snprintf (walk, 29, "%i", param);
			walk[29] = '\0';
			defpar->params[x] = walk;
			walk += 30;
			break;

		case 'f':
			//need to pass an actual float to _snprintf
			*((void **)&ftemp) = param;
			_snprintf (walk, 29, "%f", ftemp);
			walk[29] = '\0';
			defpar->params[x] = walk;
			walk += 30;
			break;

		case 'a':
			strcpy (walk, param);
			defpar->params[x] = walk;
			walk += strlen (walk) + 1;
			break;

		case 's':
		case 't':
			if (!param)
				param = "()";
			strcpy (walk, param);
			defpar->params[x] = walk;
			walk += strlen (walk) + 1;

			//no default because we already checked for it
		}
	}
	va_end (va);

	ParAssert (walk - ((char *)defpar) == len, "defineparams_t improperly formed in ParExecuteDefine");

	//defpar has been successfully formed.  Grab the guard and stick it at the head of
	//the define parameters list in the current parser.  Make sure we don't fail while
	//it's on there.

	P (&current->guard);

	defpar->next = current->defpars;
	current->defpars = defpar;

	V (&current->guard);

	st = ParRunLineCurrent (current, func->define->defineaction, &output, &nesting, pe);
	
	//put off error checking until we get defpar off of the list

	P (&current->guard);

	ParAssert (current->defpars == defpar, "Someone didn't clear themselves off of parser->defpars");
	current->defpars = current->defpars->next;
	defpar->next = NULL;

	V (&current->guard);

	ParFree (defpar);
	defpar = NULL;

	//ok, now proceed with checking.

	fail_if (!DERR_OK (st), 0, 0); //no modification to pe, it should already be setup
	fail_if (nesting, 0, PE ("Syntax error in define expression", 0, 0, 0, 0));

	//ok, looks like the expression parsed correctly, so figure out the return value and get
	//the hell out of here.

	switch (func->define->params[0][0]) //switch on the return type
	{
	case 'i':
		fail_if (!output, 0, PE ("Parse returned NULL for an integer define", 0, 0, 0, 0));
		param = (void *)atoi (output);
		ParFree (output);
		output = NULL;
		break;

	case 'f':
		fail_if (!output, 0, PE ("Parse returned NULL for a float define", 0, 0, 0, 0));
		sscanf (output, "%f", &ftemp);
		ParFree (output);
		output = NULL;
		param = (void *)1000999;
		__asm { fld ftemp };
		break;

	case 'a':
		fail_if (!output, 0, PE ("Parse returned NULL for an atom define", 0, 0, 0, 0));
		param = output;
		output = NULL; //don't free it
		break;

	case 's':
	case 't':
		//no NULL check, it's ok for strings and types
		param = output;
		output = NULL; //don't free
		break;

	default: //I don't think I want to fail if they gave garbage here.  Scheme should return an error anyway.
	case 'v':
		//void is allowed as a return type
		if (output)
		{
			ParFree (output);
			output = NULL;
		}
		param = NULL;
		break;
	}

	ParAssert (output == NULL, "Output not cleared in ParExecuteDefine");

	//defpar and output have both been freed (or set to return), I do believe we're done.

	return param;


failure:

	if (defpar)
		ParFree (defpar);

	if (output)
		ParFree (output);

	return NULL;


}

