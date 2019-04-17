/* Copyright 2008 wParam, licensed under the GPL */

#ifndef _GOVERN_G_
#define _GOVERN_H_

#include "..\common\libplug.h"

#define GOV_MAX_COMMAND_LENGTH 1024
#define GOV_MAX_COMMAND_PARAMS 1

extern HINSTANCE instance;

void GovSetPlugin (void);
int GovIsPlugin (void);
/**/



int GovGetName (char *outbuf, int outlen, char *type);
int GovPrintGoverns (void);

void GovDprintf (int level, char *format, ...);

int GsGovernorRunning (void);
int GsShutdownGovernor (int useforce, parerror_t *pe);


int GbGetSize (parerror_t *pe);
int GbGetFreeSpace (parerror_t *pe);
int GbGetDefaultSize (void);
void GbSetDefaultSize (parerror_t *pe, int newsize);
void GbClearBuffer (void *buffer); /* Must be called inside try/except */

typedef struct
{
	int nextobjofs;
	int flags; /* Opaque 32 bit value to the buffer system */
	char strings[0]; /* First the key, then the parse text */
} sectionobj_t;
#define OBJFLAG_DISABLED 1

int GbCreateHandles (parerror_t *pe, int exclusive);
void GbCloseHandles (void);

/* IMPORTANT NOTE:  Remember to surround accesses to the buffer
 * with try/except blocks.  Don't assume exceptions won't happen*/
int GbGetBuffer (parerror_t *pe, void **buffer);
void GbReleaseBuffer (void *buffer);
void GbTestBuffer (parerror_t *pe);
void GbDumpBuffer (void *buffer);
int GbSetEntry (void *buffer, char *type, char *key, char *parse, int flags);
int GbGetEntry (void *buffer, char *type, char *key, sectionobj_t **obj);


LRESULT CALLBACK GhHookProc (int code, WPARAM wParam, LPARAM lParam);


#define PEIF(fmt, a, b, c, d) do { if (pe) { PE (fmt, a, b, c, d); } else { GovDprintf (ERROR, fmt "\n", a, b, c, d); } } while (0)
#define DP GovDprintf

#undef ERROR /* screw you, GDI */
//debug levels:
#define CRIT -1
#define NONE 0
#define ERROR 1
#define WARN 2
#define NOISE 3

#define ACTIVATE "ACT"
#define ACTIVATE_ONE "ACT1"
#define CREATE "CREA"
#define DESTROY "DEST"
#define MOVESIZE "MOVE"
#define MINMAX "MINX"
#define SETFOCUS "SETF"

#define RUN "-"
#define DENY "d"
#define ZORDER "z"

#define CLASS "CL"
#define TITLE "TI"
#define ALL "AL"

#endif