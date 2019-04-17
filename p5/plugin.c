/* Copyright 2008 wParam, licensed under the GPL */

#include <windows.h>
#include "p5.h"
#include "..\common\wp_mainwin.h"
#include <stdio.h>
#include <math.h>

typedef struct plugin_t
{
	pluginfo_t info;

	HANDLE heap;
	HMODULE hmod;

	int unloading;	//this is set while waiting for denit/kill routines
					//if it's nonzero, the plugin should not be touched.

	struct plugin_t *next;
} plugin_t;


//bridges for some of the plugin function
int PlugAddMessageCallback (UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask, WNDPROC proc)
{
	return MwAddMessageFullCall (PfGetMainWindow (), message, wParam, wMask, lParam, lMask, 
		proc, FLAG_HWND | FLAG_MESSAGE | FLAG_WPARAM | FLAG_LPARAM, 0);
}

int PlugRemoveMessageCallback (UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask, WNDPROC proc)
{
	return MwDeleteMessageFullCall (PfGetMainWindow (), message, wParam, wMask, lParam, lMask, 
		proc, FLAG_HWND | FLAG_MESSAGE | FLAG_WPARAM | FLAG_LPARAM, 0);
}

void * __stdcall PlugMalloc (plugin_t *plug, int size);
void * __stdcall PlugRealloc (plugin_t *plug, void *block, int size);
void __stdcall PlugFree (plugin_t *plug, void *block);
void _chkesp (void);
void *_alloca_probe (size_t);
DERR __stdcall PlugAddWatcher (plugin_t *plug, char *type, void *value);
DERR PlugDeleteWatcher (char *type, void *value);
long _ftol (double);

static pluglib_t PlugLibrary =
{
	sizeof (pluglib_t),

	ParMalloc,
	ParFree,

	PfDerrString,

	ConWriteStringf,

	PlugMalloc,
	PlugRealloc,
	PlugFree,

	PfIsMainThread,

	RopInherit,
	ParDestroy,
	ParReset,
	ParProcessErrorType,
	ParCleanupError,
	ParRunLine,

	PfMainThreadParse,

	memset,
	_alloca_probe,
	_chkesp,
	strlen,
	strcat,
	strcpy,

	PfGetMainWindow,
	PlugAddMessageCallback,
	PlugRemoveMessageCallback,

	atoi,

	ParSplitString,

	_snprintf,

	ParAddAtom,
	ParRemoveAtom,

	InfAddInfoBlock,
	InfRemoveInfoBlock,

	UtilPixshellThreeRun,
	UtilProperShell,

	strcmp,
	strstr,
	strncmp,

	UtilLongIntToType,
	UtilTypeToLongInt,
	UtilBinaryToType,
	UtilTypeToBinary,

	memcpy,

	PlugAddWatcher,
	PlugDeleteWatcher,
	WatCheck,

	fmod,
	sqrt,
	sin,
	cos,
	tan,
	asin,
	acos,
	atan,
	ceil,
	floor,
	log,
	log10,
	pow,
	_ftol,

};

CRITICAL_SECTION PlugGuard; //guards all access to PlugListHead and the things it contains
plugin_t *PlugListHead = NULL;


DERR PlugInit (void)
{
	DERR st;

	st = PfSaneInitCriticalSection (&PlugGuard);
	fail_if (!DERR_OK (st), st, 0);

	return PF_SUCCESS;
failure:
	return st;
}

void PlugFreePlugin (plugin_t *plug, int freelibrary);
void PlugDenit (void)
{
	plugin_t *walk, *temp;
	parerror_t pe = {0};

	DeleteCriticalSection (&PlugGuard);

	//go through and clear the list.
	walk = PlugListHead;
	while (walk)
	{
		temp = walk->next;

		//call denit, and if denit fails, call kill, and if kill fails, I don't care, we're shutting down anyway
		__try
		{
			walk->info.denit (&pe);
			ParCleanupError (&pe);
			if (pe.error)
			{
				walk->info.kill (&pe);
				ParCleanupError (&pe);
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			//don't do anything special, proceed to the FreeLibrary call

			//well, I lied, we do clean the error, if necessary
			ParCleanupError (&pe);;
		}

		//changed:  We no longer free the libs in here.  This is to avoid a potential crash, if
		//a thread is running bg.sync or bg.async when a (quit) command comes through.  The threads
		//are allowed to continue, hoping that they will finish their tasks before we get to
		//PlugDenit.  Since they probably won't have, we leave the libs in RAM so that they have
		//something to walk on if they suddenly wake up in a plugin.  Plugins aren't supposed to
		//be using DllMain for denitialization anyhow.
		PlugFreePlugin (walk, 0);

		walk = temp;
	}

	PlugListHead = NULL;
			
}


//call each plugin's shutdown procedure.  That's all.  This is called from the WM_ENDSESSION
//message, so the process is going to be terminated pretty much as soon as this function returns.
//The reason we have a seperate path for system shutdown and normal (quit) is that system
//shutdown notification is given as a window mesasge, and I believe there's a decent chance
//that "proper" shutdown may be impossible in the sort of state we might be in when WM_ENDSESSION
//comes in.  (Think multiple bg.sync commands running, with a few fg's thrown in for good
//measure.)  The only thing that should be done in the shutdown routine is simple tasks like
//saving the state out to disk or registry, if that's desired.  If there's something that
//an ExitProcess() won't clean up, then that needs to be cleaned up as well.
void PlugShutdown (void)
{
	plugin_t *walk;

	//go through and clear the list.
	walk = PlugListHead;
	while (walk)
	{

		//call the shutdown procedure
		__try
		{
			walk->info.shutdown ();
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			//nothing in particular.
		}

		walk = walk->next;
	}
			
}


//These don't need to be synchronized.  The plugin denit function protects
//them, i.e. it will not return success until it can guarantee that any and
//all threads the plugin has created have exited.  If the plugin has no threads
//left, then it can't be making calls here, can it?
void * __stdcall PlugMalloc (plugin_t *plug, int size)
{
	return HeapAlloc (plug->heap, 0, size);
}

void * __stdcall PlugRealloc (plugin_t *plug, void *block, int size)
{
	return HeapReAlloc (plug->heap, 0, block, size);
}

void __stdcall PlugFree (plugin_t *plug, void *block)
{
	HeapFree (plug->heap, 0, block);
}

DERR __stdcall PlugAddWatcher (plugin_t *plug, char *type, void *value)
{
	return WatAdd (plug->info.name, type, value);
}

DERR PlugDeleteWatcher (char *type, void *value)
{
	/* Technically doesn't have to be in here, but it goes with PlugAddWatcher, so...*/
	return WatDelete (type, value);
}

//call WITHOUT the lock held
//note: returns hits on 'unloading' plugins as well.
int PlugNameExists (char *name)
{
	plugin_t *plug;
	int found = 0;

	P (&PlugGuard);

	plug = PlugListHead;
	while (plug)
	{
		if (strcmp (plug->info.name, name) == 0)
			found = 1;

		plug = plug->next;
	}

	V (&PlugGuard);

	return found;
}


char *PlugLoad (parerror_t *pe, char *plugin)
{
	DERR st;
	plugin_t *plug = NULL;
	void (*exchange)(parerror_t *, pluglib_t *, pluginfo_t *);
	int x = -1;
	char error[1024];
	char *plugname = NULL;

	fail_if (!plugin, 0, PE_BAD_PARAM (1));
	fail_if (!PfIsMainThread (), 0, PE ("PlugLoad must run in the main thread", 0, 0, 0, 0));

	plug = PfMalloc (sizeof (plugin_t));
	fail_if (!plug, 0, PE ("Out of memory on %i bytes", sizeof (plugin_t), 0, 0, 0));

	memset (plug, 0, sizeof (plugin_t));
	//this sanity test needs to come AFTER memset (0)
	fail_if ((int)plug != (int)(&plug->info), 0, PE ("Compiler sanity test has failed", 0, 0, 0, 0)); 

	plug->heap = HeapCreate (0, 64 * 1024, 0);
	fail_if (!plug->heap, 0, PE ("HeapCreate failed (%i)", GetLastError (), 0, 0, 0));

	plug->hmod = LoadLibrary (plugin);
	fail_if (!plug->hmod, 0, PE_STR ("Library %s not found (%i)", COPYSTRING (plugin), GetLastError (), 0, 0));

	exchange = (void *)GetProcAddress (plug->hmod, "P5_PluginExchange");
	fail_if (!exchange, 0, PE_STR ("Library %s does not contain a P5_PluginExchange (%i)", COPYSTRING (plugin), GetLastError (), 0, 0));
	
	__try
	{
		exchange (pe, &PlugLibrary, &plug->info);
		if (pe->error)
		{
			ParProcessErrorType (PF_SUCCESS, error, 1024, pe);
			fail_if (TRUE, 0, PE_STR ("Plugin exchange function failed: %s", COPYSTRING (error), 0, 0, 0)); //pass this error up the chain
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		fail_if (TRUE, 0, PE ("Exception 0x%.8X in exchange routine", GetExceptionCode (), 0, 0, 0));
	}

	//make sure it gave us what we need.
	fail_if (!plug->info.init || !plug->info.kill || !plug->info.init || !plug->info.name || !plug->info.denit ||
			 !plug->info.functions || !plug->info.shutdown, 0, PE ("PluginExchange function did not return all necessary info", 0, 0, 0, 0));

	//make sure a plugin with that name isn't already loaded
	fail_if (PlugNameExists (plug->info.name), 0, PE_STR ("A plugin named \'%s\' is already loaded", COPYSTRING (plug->info.name), 0, 0, 0));

	//copy the name for return purposes
	plugname = ParMalloc (strlen (plug->info.name) + 1);
	fail_if (!plugname, 0, PE ("Could not allocate buffer for return string", 0, 0, 0, 0));
	strcpy (plugname, plug->info.name);

	__try
	{
		//ok, last thing to do is call the init function.
		plug->info.init (pe);
		if (pe->error)
		{
			//we have to format this error ourselves because the plugin is going to be unloaded,
			//thus the strings are likely to become invalid.
			ParProcessErrorType (PF_SUCCESS, error, 1024, pe);

			fail_if (TRUE, 0, PE_STR ("Plugin init funciton failed: %s", COPYSTRING (error), 0, 0, 0));
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		fail_if (TRUE, 0, PE ("Exception 0x%>8X during plugin init routine", GetExceptionCode (), 0, 0, 0));
	}


	//change of plans.  it is EXTREMELY DIFFICULT, borderline impossible, to gracefully fail out
	//once we have reached this point.  (The issue is that the things we'd have to do to unwind it
	//once we've hit a failure can themselves cause failure, and it's just more garbage than I
	//really feel like dealing with.
	//
	//SO!  Since it's "hard", our solution is to just not deal with it.  If a function fails
	//to add, it gets marked as such, and the function returns success.  Call PlugVerify if you
	//care if all the funcs were properly registered
	
	
	for (x=0;plug->info.functions[x].name;x++)
	{
		st = RopAddRootFunction (plug->info.functions[x].name, plug->info.functions[x].proto,
							plug->info.functions[x].addy, plug->info.functions[x].help_desc,
							plug->info.functions[x].help_usage, plug->info.functions[x].help_more);
		if (DERR_OK (st))
			plug->info.functions[x].registered = 1;
		else
			plug->info.functions[x].registered = 0; //it should already be zero, btu just in case
	}

	
	P (&PlugGuard);

	plug->next = PlugListHead;
	PlugListHead = plug;

	V (&PlugGuard);

	return plugname;

failure:

	if (plug)
	{
		if (plug->hmod)
			FreeLibrary (plug->hmod);

		if (plug->heap)
			HeapDestroy (plug->heap);

		PfFree (plug);
	}
	
	if (plugname)
		ParFree (plugname);

	return NULL;
}
	

void PlugVerify (parerror_t *pe, char *plugin)
{
	plugin_t *plug;
	int st;
	plugfunc_t *f;
	int good; //flag for whether to return an error or not.

	P (&PlugGuard);

	fail_if (!plugin, 0, PE_BAD_PARAM (1)); //todo: check outside of lock and make lock conditional

	plug = PlugListHead;
	while (plug)
	{
		if (strcmp (plug->info.name, plugin) == 0)
			break;

		plug = plug->next;
	}

	fail_if (!plug, 0, PE_STR ("Plugin %s not found", COPYSTRING (plugin), 0, 0, 0));
	fail_if (plug->unloading, 0, PE_STR ("Plugin %s is marked as unloading", COPYSTRING (plugin), 0, 0, 0));

	f = plug->info.functions;
	good = 0; //good == 0 means all good
	while (f->name)
	{
		if (!f->registered)
		{
			ConWriteStringf ("%s:%s:%s not registered\n", plugin, f->name, f->proto);
			good++; //both a flag and a count
		}

		f++;
	}

	fail_if (good, 0, PE_STR ("Plugin %s is missing %i functions", COPYSTRING (plugin), good, 0, 0));

	V (&PlugGuard);

	return ;

failure:
	V (&PlugGuard);

	return;
}


//make sure the plugin is unlinked before you call this.
void PlugFreePlugin (plugin_t *plug, int freelibrary)
{
	if (freelibrary)
		FreeLibrary (plug->hmod);

	HeapDestroy (plug->heap);

	PfFree (plug);
}

void PlugUnload (parerror_t *pe, char *plugin)
{
	plugin_t *plug = NULL, **head;
	plugfunc_t *f;
	int st;
	int failures;

	P (&PlugGuard);

	fail_if (!plugin, 0, PE_BAD_PARAM (1));
	fail_if (!PfIsMainThread (), 0, PE ("PlugUnload must run in the main thread", 0, 0, 0, 0));

	plug = PlugListHead;
	while (plug)
	{
		if (strcmp (plug->info.name, plugin) == 0)
			break;

		plug = plug->next;
	}

	fail_if (!plug, 0, PE_STR ("Plugin %s not found", COPYSTRING (plugin), 0, 0, 0));
	fail_if (plug->unloading, 0, PE_STR ("Plugin %s is marked as unloading", COPYSTRING (plugin), 0, 0, 0));
	//checking to make sure plug->unloading isn't set SHOULD make sure we don't re-enter this
	//function in another thread and delete a plugin that's already on the stack being
	//deleted.

	//ok, so we have a plugin to unload.

	//first step is to unregister all of its functions.
	//count the number that fail to deregister

	f = plug->info.functions;
	failures = 0;
	while (f->name)
	{
		if (f->registered)
		{
			st = RopRemoveRootFunction (f->name, f->proto);
			if (!DERR_OK (st))
			{
				failures++;
				//leave st as it is, the last st to be received will be added to the parerror
			}
			else
			{
				f->registered = 0;
			}
		}

		f++;
	}

	fail_if (failures, st, PE_STR ("Failed to unload %i function(s) for plugin %s; unload aborted, status = {%s,%i}", failures, COPYSTRING (plugin), PfDerrString (st), GETDERRINFO (st)));

	//ok, so all of its functions are gone.  Now, in an effort to reduce deadlock, we're going to
	//release the plugin lock and yield execution.  The reason we do this is because it is possible
	//that the plugin has spawned another thread that is currently waiting on the plugin lock,
	//and we would prefer for the denit() routine to be able to shut it down gracefully rather
	//than have to call TerminateThread on it.  I'm not honestly sure how likely this is to happen,
	//or whether it will actually help, but the performance hit is rather minimal if there's no
	//contention, and if there is contention, then the performance hit may be worth it, so...

	//mark the plugin as being unloaded so that other funcitons won't try to operate on it.
	plug->unloading = 1;

	V (&PlugGuard);

	Sleep (10); //long enough to put us on a wait queue for one cycle

	P (&PlugGuard);

	//need to find plug again, since we released the lock.
	plug = PlugListHead;
	while (plug)
	{
		if (strcmp (plug->info.name, plugin) == 0)
			break;

		plug = plug->next;
	}

	//neither one of these should ever happen.
	fail_if (!plug, 0, PE ("Consistency failure during unload (plugin was removed)", 0, 0, 0, 0));
	fail_if (!plug->unloading, 0, PE ("Consistency failure during unload (plugin unload flag cleared)", 0, 0, 0, 0));

	__try
	{
		//ok.  Call its denit routine.
		plug->info.denit (pe);
		fail_if (pe->error, 0, 0); //pass it on (ok to pass it on; the dll isn't going to be freed)
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		fail_if (TRUE, 0, PE ("Exception 0x%.8X in plugin denit routine", GetExceptionCode (), 0, 0, 0));
	}

	//ok, so all is well, the denit routine returned success, so the plugin is now 'dead'
	//and safe to remove.

	//unlink the plugin_t
	head = &PlugListHead;
	while (*head)
	{
		if (*head == plug)
		{
			*head = plug->next;
			break;
		}

		head = &(*head)->next;
	}

	//not checking that for failure, I don't honestly think it can fail.  Besides, if plug
	//isn't in the list, then it's already done its job... somehow.

	//done accessing stuff in the plugin list
	V (&PlugGuard);

	PlugFreePlugin (plug, 1);

	return;

failure:

	if (plug)
		plug->unloading = 0;

	V (&PlugGuard);

	return;
}



void PlugKill (parerror_t *pe, char *plugin)
{
	int st;
	plugin_t *plug, **head;
	plugfunc_t *f;
	int count;
	char error[1024];

	P (&PlugGuard);

	fail_if (!plugin, 0, PE_BAD_PARAM (1));
	fail_if (!PfIsMainThread (), 0, PE ("PlugLoad must run in the main thread", 0, 0, 0, 0));

	plug = PlugListHead;
	while (plug)
	{
		if (strcmp (plug->info.name, plugin) == 0)
			break;

		plug = plug->next;
	}

	fail_if (!plug, 0, PE_STR ("Plugin %s not found", COPYSTRING (plugin), 0, 0, 0));
	fail_if (plug->unloading, 0, PE_STR ("Plugin %s is marked as unloading", COPYSTRING (plugin), 0, 0, 0));

	//make sure all of its functions are unloaded, if this isn't true, we can't help
	//todo: some way to forcibly unlock a function in the parser.

	count = 0;
	f = plug->info.functions;
	while (f->name)
	{
		if (f->registered)
			count++;

		f++;
	}

	fail_if (count, 0, PE_STR ("Plugin %s has %i function(s) still registered", COPYSTRING (plugin), count, 0, 0));

	//ok, so it's clear for destruction.

	//this time, we're not releasing the lock; the kill routine is a last resort
	//and should not require any help from anything else in p5.exe

	__try
	{
		plug->info.kill (pe);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		//set this manually, the following if statement handles it
		pe->error = STATUS_USER_DEFINED_ERROR;
		pe->format = "Exception %.8X in kill routine";
		pe->param1 = GetExceptionCode ();
		pe->flags = 0;
	}

	if (pe->error)
	{
		//the plugin failed the kill request.  This, in theory, means that the plugin
		//has done everything possible and STILL can't get into a state where it is
		//safe to unload.  In this case, we put up a message box telling the user
		//what's up, and giving them the option to kill it anyway

		ParProcessErrorType (PF_SUCCESS, error, 1024, pe);
		memset (pe, 0, sizeof (parerror_t)); //clear everything out

		st = PfMessageBoxf (NULL, "The plugin %s has failed the request to kill it.  The error string returned from PluginKill is:\r\n\r\n%s\r\n\r\nThe plugin claims that there is nothing it can do to get into a state where it is safe to unload.  If you want, you can ignore what it says and forcibly remove it.  Note that this may cause all manner of bad times in p5, most likely resulting in a crash.  The decision is yours.\r\n\r\nDo you wish to unload \'%s\' anyway?", "PluginKill not successful", MB_ICONERROR | MB_YESNO, plugin, error, plugin);
		fail_if (st == IDNO, 0, PE_STR ("Plugin %s could not be safely killed", COPYSTRING (plugin), 0, 0, 0));
	}

	//ok, so if we got here, either the kill worked, or the user overrided the plugin's
	//objections to unloading.  In either case, anything left in the error object by the
	//plugin is gone, so just proceed with the unload as if everything is normal.

	//unlink it
	head = &PlugListHead;
	while (*head)
	{
		if (*head == plug)
		{
			*head = plug->next;
			break;
		}

		head = &(*head)->next;
	}

	//done with the list
	V (&PlugGuard);

	PlugFreePlugin (plug, 1);

	return;

failure:

	V (&PlugGuard);

	return;
}


void PlugDebugUnlock (parerror_t *pe, char *plugin)
{
	int st;
	plugin_t *plug;

	P (&PlugGuard);

	fail_if (!plugin, 0, PE_BAD_PARAM (1));

	plug = PlugListHead;
	while (plug)
	{
		if (strcmp (plug->info.name, plugin) == 0)
			break;

		plug = plug->next;
	}
	
	fail_if (!plug, 0, PE_STR ("Plugin %s not found", COPYSTRING (plugin), 0, 0, 0));

	fail_if (!plug->unloading, 0, PE_STR ("Plugin %s is not marked for unloading", COPYSTRING (plugin), 0, 0, 0));

	plug->unloading = 0;

	V (&PlugGuard);

	return;

failure:

	V (&PlugGuard);

	return;
}


void PlugDebugRemove (parerror_t *pe, char *plugin, int calldenit, int callkill)
{
	int st;
	plugin_t *plug, **head;
	parerror_t temperror = {0};
	int deniterror = 0, killerror = 0;
	int insection = 0;

	fail_if (!plugin, 0, PE_BAD_PARAM (1));
	fail_if (!PfIsMainThread () && (calldenit || callkill), 0, PE ("PlugDebugRemove must run in the main thread if denit or kill are being called", 0, 0, 0, 0));

	P (&PlugGuard);
	insection = 1;

	plug = PlugListHead;
	while (plug)
	{
		if (strcmp (plug->info.name, plugin) == 0)
			break;

		plug = plug->next;
	}
	
	fail_if (!plug, 0, PE_STR ("Plugin %s not found", COPYSTRING (plugin), 0, 0, 0));

	if (calldenit)
	{
		__try
		{
			plug->info.denit (&temperror);
			if (temperror.error)
				deniterror = 1;
			ParCleanupError (&temperror);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			deniterror = 2;
		}
	}

	if (callkill)
	{
		__try
		{
			plug->info.kill (&temperror);
			if (temperror.error)
				killerror = 1;
			ParCleanupError (&temperror);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			killerror = 2;
		}
	}

	//unlink it
	head = &PlugListHead;
	while (*head)
	{
		if (*head == plug)
		{
			*head = plug->next;
			break;
		}

		head = &(*head)->next;
	}

	//done in here
	V (&PlugGuard);
	insection = 0;

	//force it out of ram
	PlugFreePlugin (plug, 1);

	fail_if (deniterror || killerror, 0, PE ("Denit %s, Kill %s", 
		deniterror == 0 ? "succeeded" : deniterror == 1 ? "returned an error" : "threw an exception",
		killerror == 0 ? "succeeded" : killerror == 1 ? "returned an error" : "threw an exception", 0, 0));


	return;

failure:

	if (insection)
		V (&PlugGuard);

	return;
}


int PlugFunctionCount (plugfunc_t *f)
{
	int out = 0;

	if (!f)
		return 0;

	while (f->name)
	{
		out++;
		f++;
	}

	return out;
}

int PlugHeapCount (HANDLE heap)
{
	PROCESS_HEAP_ENTRY phe = {0};
	int count = 0;
	int busycount = 0;

	while (HeapWalk (heap, &phe))
	{
		if (phe.wFlags & PROCESS_HEAP_ENTRY_BUSY)
			busycount++;

		count++;
	}

	return busycount;
}

void PlugList (void)
{
	plugin_t *plug;

	P (&PlugGuard);

	ConWriteString ("Name:Description:Function count:Heap count:Base:Unloading\n");

	plug = PlugListHead;
	while (plug)
	{
		ConWriteStringf ("%s:%s:%i:%i:0x%.8X:%i\n", plug->info.name, plug->info.description, PlugFunctionCount (plug->info.functions), PlugHeapCount (plug->heap), plug->hmod, plug->unloading);

		plug = plug->next;
	}

	V (&PlugGuard);
}


void PlugListFunctions (parerror_t *pe, char *plugin)
{
	int st;
	plugin_t *plug;
	plugfunc_t *f;

	P (&PlugGuard);

	fail_if (!plugin, 0, PE_BAD_PARAM (1));

	plug = PlugListHead;
	while (plug)
	{
		if (strcmp (plug->info.name, plugin) == 0)
			break;

		plug = plug->next;
	}
	
	fail_if (!plug, 0, PE_STR ("Plugin %s not found", COPYSTRING (plugin), 0, 0, 0));
	fail_if (plug->unloading, 0, PE_STR ("Plugin %s is marked for unloading", COPYSTRING (plugin), 0, 0, 0));

	f = plug->info.functions;
	fail_if (!f, 0, PE_STR ("Plugin %s has become insane\n", COPYSTRING (plugin), 0, 0, 0));

	ConWriteString ("Name:Prototype:registered\n");
	
	while (f->name)
	{
		ConWriteStringf ("%s:%s:%i\n", f->name, f->proto, f->registered);
		f++;
	}
	

	V (&PlugGuard);

	return;

failure:

	V (&PlugGuard);

	return;
}







DERR PlugAddGlobalCommands (parser_t *root)
{
	DERR st;

	st = ParAddFunction (root, "p5.plg.load", "ses", PlugLoad, "Loads a p5 plugin", "(plug.load [s:dll filename]) = [s:plugin name]", "If some of the dll functions can not be added, because of name conflicts or memory issues, this function will still return success.  Use (plug.verify) to make sure all of the plugin's functions are exposed.  The return value from this function is the name that you will need to use to reference the plugin in all of the other functions.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.plg.verify", "ves", PlugVerify, "Checks whether a plugin was completely loaded", "(plug.verify [s:pligin name]) = [v]", "The name is the plugin's actual name, not its DLL name.  The function fails if the plugin does not have all of its functions registered.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.plg.unload", "ves", PlugUnload, "Unloads a p5 plugin", "(plug.unload [s:pligin name]) = [v]", "The name is the plugin's actual name, not its DLL name.  This function unregisters all plugin functions and then calls the plugin's denit routine.  The plugin can fail this routine, indicating that it is not safe to unload at this point, or that the unload cannot be completed without potential resource leaks.  If you want the plugin gone, leaks be damned, use (plugin.kill).");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.plg.kill", "ves", PlugKill, "Forcefully unloads a p5 plugin", "(plug.kill [s:pligin name]) = [v]", "This is the more severe form of .unload, to be used when .unload is not successful.  It should be avoided whenever possible.  The PluginKill routine may cause resources to be leaked; its job is to get the plugin into a state where it is safe to unload at any cost.  If the PluginKill routine fails, a message box will appear giving the option to unload anyway.  This is probably not a good idea, but it exists nonetheless.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.plg.list", "v", PlugList, "Lists loaded plugins", "(plug.list) = [v]", "The list is output to the console.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.plg.list", "ves", PlugListFunctions, "Lists the functions of a given plugin", "(plug.list [s:plugin]) = [v]", "The list is output to the console.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.plg.dbg.unlock", "ves", PlugDebugUnlock, "Mark a plugin as not unloading", "(plug.debug.unlock [s:pligin name]) = [v]", "This function violates all of the rules for concurrency and locking and sets the given plugin's unloading member to false.  There should be no situations where this needs to be used; if you find yourself needing it, you have probably also found a bug in p5.  Use of this function is at your own risk..");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.plg.dbg.drop", "vesii", PlugDebugRemove, "Drop a plugin from ram", "(plug.debug.drop [s:pligin name] [i:call denit?] [i:call kill?]) = [v]", "This function removes the given plugin from memory.  The plugin's denit and kill routines are called if you specify a nonzero integer for the respective parameter, but the return values from each are ignored.  After the denit/kill routines are called [or not], the plugin is unlinked and deleted.  This translates to a call to FreeLibrary on the module, so unless the plugin has taken out a reference on itself, it should disappear.  Functions are not deregistered.  This is not safe by any definition of that word.  Use of this function is akin to suicide; p5 is almost certain to die.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddAlias (root, "load",  "ses", "p5.plg.load");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "unload", "ves", "p5.plg.unload");	fail_if (!DERR_OK (st), st, 0);

	return PF_SUCCESS;
failure:
	return st;
}