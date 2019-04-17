/* Copyright 2008 wParam, licensed under the GPL */

#ifndef _P5_H_
#define _P5_H_


#define CONSOLE_DERR_BASE	0x2C
#define PARSER_DERR_BASE	0x09
#define PF_DERR_BASE		0x00
#define SCRIPT_DERR_BASE	0x5C
#define HELPWIN_DERR_BASE	0x4E
#define INF_DERR_BASE		0x10
#define UTIL_DERR_BASE		0x02
#define WATCHER_DERR_BASE	0x62



#include "..\common\plugin.h"





#define P(sem) EnterCriticalSection (sem)
#define V(sem) LeaveCriticalSection (sem)



typedef struct
{
	char *name;
	char *proto;

	char *description;
	char *usage;
	char *more;

	int type;
} helpblock_t;
#define FUNCTYPE_ALIAS 1
#define FUNCTYPE_DEFINE 2
#define FUNCTYPE_NATIVE 3
#define FUNCTYPE_VARIABLE 4 //not really a function.  only name and proto are valid, proto is the "value"

DERR ParInit (void);
void ParDenit (void);
DERR ParCreate (parser_t **parout, parser_t *parent);
DERR ParDestroy (parser_t *par);
DERR ParAddFunction (parser_t *par, char *name, char *proto, void *addy, char *help_desc, char *help_usage, char *help_more);
DERR ParAddDefine (parser_t *par, char *name, char *proto, char *action, char *help_desc, char *help_usage, char *help_more);
DERR ParRemoveFunction (parser_t *par, char *name, char *proto);
DERR ParRemoveDefine (parser_t *par, char *name, char *proto);
DERR ParAddAtom (parser_t *par, char *atom, char *value, int flags);
DERR ParRemoveAtom (parser_t *par, char *name);
void *ParMalloc (int size);
void ParFree (void *block);
DERR ParRunLine (parser_t *par, char *line, char **output, int *nesting, char *error, int errlen);
DERR ParRunLineCurrent (parser_t *cur, char *line, char **output, int *nesting, parerror_t *pe);
int ParHeapCount (void);
void ParSetDegrade (int new);
DERR ParReset (parser_t *par);
void ParProcessErrorType (int st, char *error, int errlen, parerror_t *pe);
void ParCleanupError (parerror_t *pe);
DERR ParSplitString (char *weekdays, int *outcount, char **outarray, char delim);
DERR ParRemoveAlias (parser_t *par, char *name, char *proto);
DERR ParAddAlias (parser_t *par, char *name, char *proto, char *target);
DERR ParGetHelpData (parser_t *par, helpblock_t **helpout);






DERR ConOpenConsole (char *title, int color);
DERR ConOpenDebugConsole (void);
DERR ConCloseConsole (void);
DERR ConForceCloseConsole (void);
DERR ConRunDummyConsole (char *params);
DERR ConWriteString (char *string);
DERR ConWriteStringf (char *format, ...);
DERR ConAddGlobalCommands (parser_t *root);




DERR RopInit (void);
void RopDenit (void);
DERR RopInherit (parser_t **out);
void RopParserStressTest (void);
DERR RopAddRootFunction (char *name, char *proto, void *addy, char *h1, char *h2, char *h3);
DERR RopRemoveRootFunction (char *name, char *proto);



#define ERROR_ABORT			1
#define ERROR_VERBOSE		4
#define ERROR_SKIP			8
#define MAX_LABEL 256
DERR ScrExecute (char *filename, char *targetlabel, int *lineout, char *error, int errlen, int paramc, char **paramv);
DERR ScrAddGlobalCommands (parser_t *root);
DERR ScrHashProcessing (char *buffer, int *incomment, char *labelbuf, char **targetlabel, int *restart,
						int *errormode, int nesting, int *abort, int paramc, char *error, int errlen, int *gotocount);
int ScrIsHashLine (char *line);



DERR PfSaneInitCriticalSection (CRITICAL_SECTION *cs);
int PfMessageBoxf (HWND hwnd, char *fmt, char *title, int flags, ...);
char *PfDerrString (DERR in);
DERR PfAddGlobalCommands (parser_t *root);
void *PfMalloc (int size);
void PfFree (void *block);
int PfHeapCount (void);
int PfIsMainThread (void);
DERR PfMainThreadParse (char *line, char **output, parerror_t *pe);
HWND PfGetMainWindow (void);
extern HINSTANCE PfInstance;
DERR PfRunInMainThread (DERR (*func)(void *), void *param);
//todo: the PfRunInMainThread cannot be reliably handled in the case
//of failure.  Fix it somehow so that the Interlocked exchange that happens
//to handle freeing the memory it uses to talk to the main thread also
//frees the param.


DERR PlugInit (void);
void PlugDenit (void);
DERR PlugAddGlobalCommands (parser_t *root);
void PlugShutdown (void);



DERR HwCreateHelpWindow (helpblock_t *hb);



DERR InfInit (void);
void InfDenit (void);
DERR InfAddGlobalCommands (parser_t *root);
DERR InfAddInfoBlock (char *name, HINSTANCE module, int resid);
void InfRemoveInfoBlock (char *name);
DERR InfGetInfoText (char *name, char **text);
DERR InfGetInfoNameList (char ***output);



DERR MthAddGlobalCommands (parser_t *root);

DERR UtilAddGlobalCommands (parser_t *root);
DERR UtilProperShell (char *line);
DERR UtilPixshellThreeRun (char *command, int async);

//These functions allocate from the parser heap.  If they return 0 or NULL,
//then an appropriate error has been set in *pe, and the calling function
//should just throw it.
//Do not pass NULL for anything, unless noted.
char *UtilLongIntToType (parerror_t *pe, __int64 in);
int UtilTypeToLongInt (parerror_t *pe, char *in, __int64 *output);
char *UtilBinaryToType (parerror_t *pe, void *voidin, int inlen, char *typename);
//data can be null, if this is the case, length is computed and returned.
int UtilTypeToBinary (parerror_t *pe, char *type, char *typeexpected, int *lenout, void *voiddata, int datalen);



char *WatGetErrorString (int errindex);
DERR WatInit (void);
void WatDenit (void);
DERR WatAdd (char *plugin, char *type, void *value);
DERR WatDelete (char *type, void *value);
DERR WatAddGlobalCommands (parser_t *root);
int WatCheck (char *type, void *value);



#endif