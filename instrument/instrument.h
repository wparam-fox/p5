/* Copyright 2006 wParam, licensed under the GPL */

#ifndef _INSTRUMENT_H_
#define _INSTRUMENT_H_

//GDI defines ERROR as something to do with regions.  I want to use it for
//my own purposes, so undefine it.
#undef ERROR

#include "..\common\libplug.h"

#ifdef SEVEN
#include "c:\apps\vs\detours\include\detours.h"
#else
#include "d:\apps\vs\detours\include\detours.h"
#endif


//makes fail_if macros look a little nicer
#define DP IsmDprintf

extern HINSTANCE instance;


#define MAX_INSTRUMENT_SIZE 2000


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

#define INSTRUMENT_ADDRESS 3
typedef struct
{
	int stdcall;
	int numparams;
	int dofinalcode;

	void *address;
} instaddress_t;






typedef struct actionitemstruct
{
	//size MUST be the first entry
	DWORD size;

	DWORD type;

	BYTE data[0];
} actionitem_t;

typedef struct actionsstruct
{
	DWORD size; //total size of all actions

	DWORD numglobals; //number of 32 bit globals
	DWORD globalofs;

	int numvars;
	int scratchsize;

	BYTE items[0];
} action_t;


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

extern instimpl_t WhWindowImplementation;
extern instimpl_t FhFunctionImplementation;
extern instimpl_t FhAddressImplementation;



void IsmDprintf (int level, char *format, ...);
void *IsmMalloc (int size);
void IsmFree (void *block);
void *IsmRealloc (void *block, int size);
int IsmCheckMode (DWORD desiredmode);
int IsmSetMode (DWORD mode);
void IsmSetDbLevel (int level);
int IsmCheckIsPlugin (void);
void IsmSetPluginMode (void);


//debug levels:
#define NONE 0
#define ERROR 1
#define WARN 2
#define NOISE 3

//dll modes:
#define MODE_UNKNOWN 0
#define MODE_MANAGER 1
#define MODE_AGENT 2

extern BYTE IsmMessageBuffer[];
extern HANDLE IsmMessageMutex;
extern HANDLE IsmManagerMutex;
extern int IsmResponseNumber;


instlist_t *InSearchInstruments (int (*compare)(instlist_t *, void *), void *data);
int InSetInstrument (instrument_t *inst, action_t *actions);
int InUnsetInstrument (instrument_t *inst);
instrument_t *InCreateWindowInstrument (HWND hwnd, UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask, parerror_t *pe);
instrument_t *InCreateFunctionInstrument (char *lib, char *func, int numparams, int stdcall, int dofinalcode, parerror_t *pe);
instrument_t *InCreateAddressInstrument (void *addy, int numparams, int stdcall, int dofinalcode, parerror_t *pe);




HANDLE UtilCreateEventf (SECURITY_ATTRIBUTES *sa, BOOL manualreset, BOOL initstate, char *name, ...);
HANDLE UtilOpenEventf (char *name, ...);
void UtilListInsert (instlist_t **head, instlist_t *item);
void UtilListRemove (instlist_t **head, instlist_t *item);



DWORD CALLBACK AgnInstrumentationListener (void *data);


DWORD ActnExecute (action_t *action, DWORD(*realfunc)(void *), void *funcdata, void **params, int numparams);
action_t *ActnParseString (char *string, parerror_t *pe);

DWORD CALLBACK MnForeignStub (void *param);

#endif