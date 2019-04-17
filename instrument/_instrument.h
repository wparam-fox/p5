/* Copyright 2006 wParam, licensed under the GPL */

#ifndef _INSTRUMENT_H_
#define _INSTRUMENT_H_

#include <windows.h>
#undef ERROR


#ifdef SEVEN
#include "c:\apps\vs\detours\include\detours.h"
#else
#include "d:\apps\vs\detours\include\detours.h"
#endif
#include <stdio.h>

#define INSTRUMENT_NOERROR 1
#define INSTRUMENT_TFP 2
#define INSTRUMENT_BADPIDTYPE 3
#define INSTRUMENT_NOMESSAGE 4
#define INSTRUMENT_NOACK 5
#define INSTRUMENT_NOSEND 6


extern HINSTANCE instance;
extern int dblevel;

extern int SafeToUnload;


//instruments are never allowed to go above this size
//(makes implementation much easier)
#define MAX_INSTRUMENT_SIZE 1024

#define MAGIC_NUMBER 0x1D49FEBA
typedef struct messagebuf
{
	DWORD size;

	DWORD magic; //just in case

	DWORD pid; //pid of process message was intended for
	DWORD event; //id of return event

	DWORD command;

	DWORD instrumentofs;
	DWORD actionsofs;

	BYTE data[0];
} message_t;

#define COMMAND_INSTRUMENT 1
#define COMMAND_UNINSTRUMENT 2
#define COMMAND_REPORT 3
#define COMMAND_ABORT 4
#define COMMAND_PRINT 5



typedef struct instrumentstruct
{
	DWORD size;

	DWORD type;

	BYTE data[0];

} instrument_t;

#define INSTRUMENT_WINDOW 1
typedef struct
{
	HWND hwnd;
	UINT message;

	//if msg.wParam & this.wMask == this.wParam
	WPARAM wParam;
	WPARAM wMask;

	LPARAM lParam;
	LPARAM lMask;
} instwindow_t;

#define INSTRUMENT_FUNCTION 2
typedef struct
{
	int nameofs; //offset from the end of this struct.
				 //the lib name is implied offset to 0
	
	int stdcall;
	int numparams;

	int dofinalcode;

	BYTE data[0];
} instfunction_t;


typedef struct actionitemstruct
{
	DWORD size;

	DWORD type;

	BYTE data[0];
} actionitem_t;

typedef struct actionsstruct
{
	DWORD size; //total size of all actions

	DWORD numvars; //number of 32 bit variables
	DWORD varofs;

	BYTE items[0];
} action_t;

DWORD ExecuteActions (action_t *action, DWORD(*realfunc)(void *), void *funcdata, void **params, int numparams);
action_t *ActionsFromStrings (int argc, char **argv);



typedef struct instlist_s
{
	struct instlist_s *next;

	instrument_t *inst;
	action_t *actions;
	void **runtime;

	BYTE data[0];
} instlist_t;


typedef struct
{
	char *name;
	int numrumtime;

	int (*set)(instlist_t *inst);
	int (*unset)(instlist_t *inst);
	int (*parse)(int argc, char **argv, instrument_t **);
} instimpl_t;

extern instimpl_t WindowHook;
extern instimpl_t FunctionHook;


#define MODE_UNKNOWN 0
#define MODE_MANAGER 1
#define MODE_AGENT 2
int CheckMode (DWORD desiredmode);
int SetMode (DWORD mode);
extern BYTE msgbuf[];
void CancelAgent (void);
extern int responsenumber; //shared; protected by manager mutex
int SmartIntConvert (char *string);


HANDLE OpenEventf (char *name, ...);
HANDLE CreateEventf (SECURITY_ATTRIBUTES *sa, BOOL manualreset, BOOL initstate, char *name, ...);
void ListInsert (instlist_t **head, instlist_t *item);
void ListRemove (instlist_t **head, instlist_t *item);



extern HANDLE manager_mutex;
extern HANDLE msgbuf_mutex;
#define P(sem) WaitForSingleObject (sem, INFINITE)
#define V(sem) ReleaseMutex (sem)


DWORD CALLBACK Instrument (void *data);
extern int instrumenting;


void SetDbLevel (int level);
#define NONE 0
#define ERROR 1
#define WARN 2
#define NOISE 3
void dprintf (int lev, char *format, ...);

#define fail_if(cond, val) if (cond) { dprintf ##val ; goto failure; }
#define log_if(cond, val) if (cond) { dprintf ##val ; }


int UnsetInstrument (instrument_t *inst);
int SetInstrument (instrument_t *inst, action_t *actions);
instlist_t *SearchInstruments (int (*compare)(instlist_t *, void *), void *data);
int ParseInstrument (int argc, char **argv, instrument_t **instout);

instrument_t *CreateWindowInstrument (HWND hwnd, UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask);
instrument_t *CreateFunctionInstrument (char *lib, char *func, int numparams, int stdcall, int dofinalcode);



message_t *ParseMessage (int argc, char **argv);
int ManagerSend (DWORD pid, message_t *message);
int ManagerHookProcess (DWORD pid);


#endif