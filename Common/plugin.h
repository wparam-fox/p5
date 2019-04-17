/* Copyright 2008 wParam, licensed under the GPL */

#ifndef _PLUGIN_H_
#define _PLUGIN_H_


#define DERRCODE(base, extent) (((base) << 24) | ((extent) << 16))
#define DERRINFO(errcode, info) ((errcode) | ((info) & 0xFFFF))
#define GETDERRCODE(val) ((val) & 0xFFFF0000)
#define GETDERRINFO(val) ((val) & 0xFFFF)
typedef unsigned long DERR;

#define log_if(cond, status, other) if (cond) { st = status ; other ; /*printf ("line: %i\n", __LINE__);*/ }
#define fail_if(cond, status, other) if (cond) { st = status ; other ; /*printf ("line: %i\n", __LINE__);*/ goto failure ; }
#define DI(val) st = DERRINFO (st, (val))
#define DIGLE DI (GetLastError ())
#define PE(fmt, p1, p2, p3, p4) pe->error = STATUS_USER_DEFINED_ERROR; pe->format = fmt; pe->param1 = (int)(p1) ; pe->param2 = (int)(p2) ; pe->param3 = (int)(p3) ; pe->param4 = (int)(p4)
#define PE_STR(fmt, p1, p2, p3, p4) pe->error = 0xBEEF; pe->flags = 0 ;  \
												pe->param4 = (int)(p4) ; pe->flags <<= 1 ; \
												pe->param3 = (int)(p3) ; pe->flags <<= 1 ; \
												pe->param2 = (int)(p2) ; pe->flags <<= 1 ; \
												pe->param1 = (int)(p1) ; pe->flags <<= 1 ; \
												pe->format = fmt;  pe->info = 0 ; pe->error = STATUS_USER_DEFINED_ERROR
#ifdef IS_PLUGIN
#define COPYSTRING(str) ((pe->info = (pe->error == 0xBEEF ? (int)ParMalloc (PlStrlen (str) + 1) : 0)) ? pe->flags |= 1 , PlStrcpy ((char *)pe->info, str) : "[*Out of memory*]")
#else
#define COPYSTRING(str) ((pe->info = (pe->error == 0xBEEF ? (int)ParMalloc (strlen (str) + 1) : 0)) ? pe->flags |= 1 , strcpy ((char *)pe->info, str) : "[*Out of memory*]")
#endif

//some common errors simplified.  Parameters should be 1-based, i.e. "param 1 invalid" means the first param was invalid.
#define PE_BAD_PARAM(x) PE ("Parameter %i is invalid", x, 0, 0, 0)
#define PE_OUT_OF_MEMORY(x) PE ("Out of memory allocating %i bytes", x, 0, 0, 0)
#define PE_DERR(str, x, p3, p4) PE (str " {%s,%i}", PlGetDerrString (x), GETDERRINFO (x), (p3), (p4))

#define DERR_OK(a) ((a) == PF_SUCCESS)

//scheme codes taken from wp_scheme2.h
#define STATUS_SYNTAX_ERROR				3
#define STATUS_USER_DEFINED_ERROR		13

//standard global error codes
#define PF_SUCCESS			0


typedef struct parser_s parser_t;
typedef struct
{
	int error;
	int info;

	//now the extra info.  This is how functions report error info back.
	//It's just a text string for the user's benefit; there isn't any
	//provision made for the caller to interpret what happened.  (The caller
	//of ParRunLine gets only the text string back, the integers passed here
	//are discarded before ParRunLine returns.
	//
	//NOTE: is it an error to set any of these fields if you do not also set
	//error to STATUS_USER_DEFINED_ERROR.  If that is not set, an error will
	//not occur, and any memory allocated for these parameters will be lost.
	int flags; //which params need to be passed to ParFree
	char *format;
	int param1;
	int param2;
	int param3;
	int param4;
} parerror_t;
#define PEF_FORMAT 1
#define PEF_PARAM1 2
#define PEF_PARAM2 4
#define PEF_PARAM3 8
#define PEF_PARAM4 16



typedef struct
{
	char *name;
	char *proto;
	void *addy;
	char *help_desc;
	char *help_usage;
	char *help_more;

	//following are for internal use only

	int registered; //whether it's been
} plugfunc_t;

typedef struct
{
	char *name;
	char *description;
	plugfunc_t *functions;

	void (*init)(parerror_t *);
	void (*denit)(parerror_t *);
	void (*kill)(parerror_t *);
	void (*shutdown)(void);


} pluginfo_t;



typedef struct
{
	int size;

	void *(*ParMalloc)(int);
	void (*ParFree)(void *);

	char *(*PfDerrString)(DERR);

	DERR (*ConWriteStringf)(char *string, ...);

	void *(__stdcall *PlugMalloc)(void *, int);
	void *(__stdcall *PlugRealloc)(void *, void *, int);
	void (__stdcall *PlugFree)(void *, void *);

	int (*PfIsMainThread) (void);

	DERR (*RopInherit) (parser_t **out);
	DERR (*ParDestroy) (parser_t *par);
	DERR (*ParReset) (parser_t *par);
	void (*ParProcessErrorType) (int st, char *error, int errlen, parerror_t *pe);
	void (*ParCleanupError) (parerror_t *pe);
	DERR (*ParRunLine) (parser_t *par, char *line, char **output, int *nesting, char *error, int errlen);

	DERR (*PfMainThreadParse) (char *line, char **output, parerror_t *pe);

	void *(*memset) (void *dest, int c, size_t count);
	void *(*_alloca) (size_t size);
	void (*_chkesp)(void);
	size_t (*strlen) (const char *string);
	char *(*strcat) (char *strDestination, const char *strSource);
	char *(*strcpy) (char *strDestination, const char *strSource);

	HWND (*PfGetMainWindow) (void);
	int (*PlugAddMessageCallback) (UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask, WNDPROC proc);
	int (*PlugRemoveMessageCallback) (UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask, WNDPROC proc);

	int (*atoi)(char *);

	DERR (*ParSplitString) (char *weekdays, int *outcount, char **outarray, char delim);

	int (*_snprintf) (char *buffer, size_t count, const char *format, ...);

	DERR (*ParAddAtom) (parser_t *par, char *atom, char *value, int flags);
	DERR (*ParRemoveAtom) (parser_t *par, char *name);

	DERR (*InfAddInfoBlock) (char *name, HINSTANCE module, int resid);
	void (*InfRemoveInfoBlock) (char *name);

	DERR (*UtilPixshellThreeRun) (char *command, int async);
	DERR (*UtilProperShell) (char *line);

	int (*strcmp) (const char *string1, const char *string2);
	char *(*strstr) (const char *string, const char *strCharSet);
	int (*strncmp) (const char *string1, const char *string2, size_t count);

	char *(*UtilLongIntToType) (parerror_t *pe, __int64 in);
	int (*UtilTypeToLongInt) (parerror_t *pe, char *in, __int64 *output);
	char *(*UtilBinaryToType) (parerror_t *pe, void *voidin, int inlen, char *typename);
	int (*UtilTypeToBinary) (parerror_t *pe, char *type, char *typeexpected, int *lenout, void *voiddata, int datalen);

	void *(*memcpy)( void *dest, const void *src, size_t count );

	DERR (__stdcall * PlugAddWatcher) (void *plug, char *type, void *value);
	DERR (*PlugDeleteWatcher) (char *type, void *value);
	int (*WatCheck) (char *type, void *value);

	double (*fmod)(double, double);
	double (*sqrt)(double);
	double (*sin)(double);
	double (*cos)(double);
	double (*tan)(double);
	double (*asin)(double);
	double (*acos)(double);
	double (*atan)(double);
	double (*ceil)(double);
	double (*floor)(double);
	double (*log)(double);
	double (*log10)(double);
	double (*pow)(double, double);
	long (*ftol)(double);





} pluglib_t;









#endif