/* Copyright 2008 wParam, licensed under the GPL */

#include <windows.h>
#include "govern.h"
#include "resource.h"

HINSTANCE instance;
char *PluginName = "governor";
char *PluginDescription = "Rule other windows with an iron fist";

void GovSetDesktopsDifferent (int newval);
int GovAreDesktopsDifferent (void);
void GovSetDebugLevel (int level);
int GovGetDebugLevel (void);
void GovSetPrintGoverns (int newval);
char *GovScriptGetName (parerror_t *pe);
void GovScriptSetName (parerror_t *pe, char *newname);
void GsScriptStartGovernor (parerror_t *pe);
void GsScriptContinueGovernor (parerror_t *pe);
void GsScriptStartShared (parerror_t *pe);
void GsScriptStopGovernor (parerror_t *pe);
void GsScriptKillGovernor (parerror_t *pe);
int GsScriptGetPrint (void);
void GsScriptSetPrint (int val);
void GsFlushGovernors (parerror_t *pe);
void GsScriptDumpBuffer (parerror_t *pe);

void GsScriptThreadStatus (parerror_t *pe);

#define STD_PROTO(nicename) \
	void GsScriptAdd##nicename (parerror_t *pe, int, char *key, char *action);\
	void GsScriptDelete##nicename (parerror_t *pe, int, char *key);\
	char *GsScriptGet##nicename (parerror_t *pe, int, char *key)

#define PROTO_2(name) \
	STD_PROTO (name##Class); \
	STD_PROTO (name##Title)

#define PROTO_6(name) \
	STD_PROTO (name##Class); \
	STD_PROTO (name##Title); \
	STD_PROTO (name##All); \
	STD_PROTO (name##ClassDeny); \
	STD_PROTO (name##TitleDeny); \
	STD_PROTO (name##AllDeny)

PROTO_6 (Activate);
PROTO_2 (ActivateOne);
PROTO_6 (Create);
PROTO_6 (Destroy);
PROTO_6 (MoveSize);
PROTO_6 (MinMax);
PROTO_6 (SetFocus);

STD_PROTO (ZorderTitle);
STD_PROTO (ZorderClass);

plugfunc_t PluginFunctions[] = 
{
	//functions.  Example:
	//{"rb.show", "ie", RbShow, "Shows the runbox", "(rb.show) = [i:s/f]", "Must run in the main thread"},
	{"gov.cfg.size", "ie", GbGetSize, "Gets the size of the governing buffer", "(gov.cfg.size) = [i:size]", "Reads from the running data.  Governor must be enabled.  See (info \"governer\")"},
	{"gov.cfg.defsize", "i", GbGetDefaultSize, "Gets the default size of the governing buffer", "(gov.cfg.defsize) = [i:value]", "See (info \"govern\")"},
	{"gov.cfg.defsize", "vei", GbSetDefaultSize, "Sets the default size of the governing buffer", "(gov.cfg.defsize [i:newsize]) = [v]", "The size cannot be set once the governor is enabled.  See (info \"govern\")"},
	{"gov.cfg.freespace", "ie", GbGetFreeSpace, "Gets the free space remaining in the governing buffer", "(gov.cfg.freespace) = [i:space left]", "Returns the size (in bytes) remaining in the buffer."},

	{"gov.cfg.name", "se", GovScriptGetName, "Gets the base name used by the governor", "(gov.cfg.name) = [s:current name]", "See (info \"govern\")"},
	{"gov.cfg.name", "ves", GovScriptSetName, "Sets the base name used by the governor", "(gov.cfg.name [s:current name]) = [v]", "See (info \"govern\").  Update:  Do not use this function.  It is not properly communicated to the hooked processes, and using shared memory would make it useless."},

	{"gov.cfg.debug", "i", GovGetDebugLevel, "Gets the debug level used throughout the system", "(gov.cfg.debug) = [i:level]", "The debug level is shared by all instances of p5 running in the system.  See (info \"govern\")"},
	{"gov.cfg.debug", "vi", GovSetDebugLevel, "Sets the debug level used throughout the system", "(gov.cfg.debug [i:newlevel]) = [v]", "The debug level is shared by all instances of p5 running in the system.  0 = off, 1 = errors, 2 = warnings, 3 = noise.  See (info \"govern\")"},

	{"gov.cfg.usedesks", "i", GovAreDesktopsDifferent, "Gets the desktop difference value", "(gov.cfg.usedesks) = [i:value]", "This is a boolean that decides whether the governor puts the name of the current desktop into object names.  Default is yes.  You can turn this off if you want, but I wouldn't recommend it."},
	{"gov.cfg.usedesks", "vi", GovSetDesktopsDifferent, "Sets the desktop difference value", "(gov.cfg.usedesks [i:value]) = [v]", "This is a boolean that decides whether the governor puts the name of the current desktop into object names.  Default is yes.  You can turn this off if you want, but I wouldn't recommend it."},

	{"gov.debug.buffer", "ve", GbTestBuffer, "Tests the internal buffer functions", "(gov.debug.buffer) = [v]", "Look for dump output on the console"},

	{"gov.start", "ve", GsScriptStartGovernor, "Starts the governor", "(gov.start) = [v]", "Opens the section, sets the hook, and starts the message listener.  This function fails if another instance is running with the same name."},
	{"gov.startshare", "ve", GsScriptStartShared, "Starts the governor, trying to share with a running instance", "(gov.startshared) = [v]", "Opens the section, using an existing section if found, and hooks the desktop.  Will not start a message parser.  This will almost certainly fail in a multiple desktop environment unless you have set gov.cfg.usedesks to zero."},
	{"gov.continue", "ve", GsScriptContinueGovernor, "Starts the governor, using the governor section if it exists", "(gov.continue) = [v]", "This function will use an existing section if it is already open.  This differs from gov.startshared in that it will always attempt to create a receiving mailslot for parse messages, whether it creates the section or not.  This is the ideal function to use in a startup script.  Probably should be followed by gov.flush."},

	{"gov.stop", "ve", GsScriptStopGovernor, "Stops the governor", "(gov.stop) = [v]", "Will only stop the message thread if it can do so gracefully.  If it's wedged or stuck parsing something, this function fails; try gov.kill instead"},
	{"gov.kill", "ve", GsScriptKillGovernor, "Stops the governor, forcefully", "(gov.kill) = [v]", "Uses TerminateThread to stop the message thread.  This results in a memory leak of the thread stack.  Avoid this function unless gov.stop is failing."},

	{"gov.flush", "ve", GsFlushGovernors, "Clears all governors", "(gov.flush) = [v]", "Use this to clear out all running governors and revert to a clean slate, as if you had just started it fresh.  This is the recommended command to run at the start of any script that adds governors; it is much more efficient than calling gov.stop/gov.start."},
	{"gov.dump", "ve", GsScriptDumpBuffer, "Dumps the governor shared buffer to the console", "(gov.dump) = [v]", "This gives a full picture of what's in the buffer.  The flag value 1 signals a governor that is disabled."},

	{"gov.cfg.print", "i", GsScriptGetPrint, "Gets the governor print value", "(gov.cfg.print) = [i:value]", "If the print value is nonzero, the governor will print commands it receives to the console."},
	{"gov.cfg.print", "vi", GsScriptSetPrint, "Sets the governor print value", "(gov.cfg.print [i:newval]) = [v]", "Sets the print value.  If nonzero, the governor will print commands it receives to the console."},

	{"gov.cfg.showhits", "vi", GovSetPrintGoverns, "Sets the governor hit printing value", "(gov.cfg.showhits [i:newval]) = [v]", "This value controls whether OutputDebugString will be called for each potential hit.  The string printed will show the class and text that would be tested.  This can be helpful to determine what the title of a window is as it's being created.  This value is shared between all instances of p5 running."},
	{"gov.cfg.showhits", "i", GovPrintGoverns, "Gets the governor hit printing value", "(gov.cfg.showhits) = [i:curval]", "This value is shared between all instances of p5 running on one system."},

	{"gov.debug.thread", "ve", GsScriptThreadStatus, "Checks the status of the governor message thread", "(gov.debug.thread) = [v]", "This function fails in various ways if the governor thread is not as it should be.  Failures returned: Governor not on, Thread not started, Couldn't check thread state, Thread not running."},

#define STD_ACTIONS_ALL_NODENY(nicename, s1, s2, governortype, governorkey) \
	{"gov.add." s1 ".run." s2, "vecsa", GsScriptAdd##nicename, "Adds a new " governortype " governor keyed on " governorkey, "(gov.add." s1 ".run." s2 " [s:empty string] [a:action to take]) = [v]", "See (info \"govern\").  You *MUST* pass \"\" as the first parameter or it will not work."},\
	{"gov.del." s1 ".run." s2, "vec", GsScriptDelete##nicename, "Deletes an existing " governortype " governor keyed on " governorkey, "(gov.del." s1 ".run." s2 ") = [v]", "See (info \"govern\")"},\
	{"gov.get." s1 ".run." s2, "aec", GsScriptGet##nicename, "Gets the action associated with a " governortype " governor", "(gov.get." s1 ".run." s2 ") = [s:action]", "The return value is an atom."},

#define STD_ACTIONS_ALL(nicename, s1, s2, governortype, governorkey) \
	STD_ACTIONS_ALL_NODENY(nicename, s1, s2, governortype, governorkey) \
	{"gov.add." s1 ".deny." s2, "vec", GsScriptAdd##nicename##Deny, "Adds a new " governortype " governor keyed on " governorkey, "(gov.add." s1 ".deny." s2 ") = [v]", "See (info \"govern\")"},\
	{"gov.del." s1 ".deny." s2, "vec", GsScriptDelete##nicename##Deny, "Deletes an existing " governortype " governor keyed on " governorkey, "(gov.del." s1 ") = [v]", "See (info \"govern\")"},\
	{"gov.get." s1 ".deny." s2, "aec", GsScriptGet##nicename##Deny, "Gets the action associated with a " governortype " governor", "(gov.get." s1 ".deny." s2 ") = [s:action]", "The return value is an atom."},


#define STD_ACTIONS_NODENY(nicename, s1, s2, governortype, governorkey) \
	{"gov.add." s1 ".run." s2, "vecsa", GsScriptAdd##nicename, "Adds a new " governortype " governor keyed on " governorkey, "(gov.add." s1 ".run." s2 " [s:keyname] [a:action to take]) = [v]", "See (info \"govern\")"},\
	{"gov.del." s1 ".run." s2, "vecs", GsScriptDelete##nicename, "Deletes an existing " governortype " governor keyed on " governorkey, "(gov.del." s1 ".run." s2 " [s:keyname]) = [v]", "See (info \"govern\")"},\
	{"gov.get." s1 ".run." s2, "aecs", GsScriptGet##nicename, "Gets the action associated with a " governortype " governor", "(gov.get." s1 ".run." s2 " [s:keyname]) = [s:action]", "The return value is an atom."},

#define STD_ACTIONS(nicename, s1, s2, governortype, governorkey) \
	STD_ACTIONS_NODENY (nicename, s1, s2, governortype, governorkey) \
	{"gov.add." s1 ".deny." s2, "vecs", GsScriptAdd##nicename##Deny, "Adds a new " governortype " denial governor keyed on " governorkey, "(gov.add." s1 ".deny." s2 " [s:keyname]) = [v]", "See (info \"govern\")"},\
	{"gov.del." s1 ".deny." s2, "vecs", GsScriptDelete##nicename##Deny, "Deletes an existing " governortype " denial governor keyed on " governorkey, "(gov.del." s1 ".deny." s2 " [s:keyname]) = [v]", "See (info \"govern\")"},\
	{"gov.get." s1 ".deny." s2, "aecs", GsScriptGet##nicename##Deny, "Gets the action associated with a " governortype " denial governor", "(gov.get." s1 ".deny." s2 " [s:keyname]) = [s:action]", "The return value is an atom."},

#define STD_ACTIONS_OTHER(nicename, s1, s2, governortype, governorkey, thingie) \
	{"gov.add." s1 "." thingie "." s2, "vecsa", GsScriptAdd##nicename, "Adds a new " governortype " governor keyed on " governorkey, "(gov.add." s1 "." thingie "." s2 " [s:key] [a:action to take]) = [v]", "See (info \"govern\")."},\
	{"gov.del." s1 "." thingie "." s2, "vecs", GsScriptDelete##nicename, "Deletes an existing " governortype " governor keyed on " governorkey, "(gov.del." s1 "." thingie "." s2 " [s:key]) = [v]", "See (info \"govern\")"},\
	{"gov.get." s1 "." thingie "." s2, "aecs", GsScriptGet##nicename, "Gets the action associated with a " governortype " governor", "(gov.get." s1 "." thingie "." s2 " [s:key]) = [s:action]", "The return value is an atom."},


	STD_ACTIONS (ActivateClass, "activate", "class", "window activation", "window class")
	STD_ACTIONS (ActivateTitle, "activate", "title", "window activation", "window title")
	STD_ACTIONS_ALL (ActivateAll, "activate", "all", "window activation", "(all windows)")

	STD_ACTIONS_NODENY (ActivateOneClass, "actone", "class", "one-shot window activation", "window class")
	STD_ACTIONS_NODENY (ActivateOneTitle, "actone", "title", "one-shot window activation", "window title")

	STD_ACTIONS (CreateClass, "create", "class", "window creation", "window class")
	STD_ACTIONS (CreateTitle, "create", "title", "window creation", "window title")
	STD_ACTIONS_ALL (CreateAll, "create", "all", "window creation", "(all windows)")

	STD_ACTIONS (DestroyClass, "destroy", "class", "window destruction", "window class")
	STD_ACTIONS (DestroyTitle, "destroy", "title", "window destruction", "window title")
	STD_ACTIONS_ALL (DestroyAll, "destroy", "all", "window destruction", "(all windows)")

	STD_ACTIONS (MoveSizeClass, "movesize", "class", "window movement", "window class")
	STD_ACTIONS (MoveSizeTitle, "movesize", "title", "window movement", "window.title")
	STD_ACTIONS_ALL (MoveSizeAll, "movesize", "all", "window movement", "(all windows)")

	STD_ACTIONS (MinMaxClass, "minmax", "class", "window min/maximize", "window class")
	STD_ACTIONS (MinMaxTitle, "minmax", "title", "window min/maximize", "window title")
	STD_ACTIONS_ALL (MinMaxAll, "minmax", "all", "window min/maximize", "(all windows)")

	STD_ACTIONS (SetFocusClass, "setfocus", "class", "window set focus", "window class")
	STD_ACTIONS (SetFocusTitle, "setfocus", "title", "window set focus", "window title")
	STD_ACTIONS_ALL (SetFocusAll, "setfocus", "all", "window set focus", "(all windows)")

	STD_ACTIONS_OTHER (ZorderClass, "create", "class", "window create Z-order", "window class", "zorder")
	STD_ACTIONS_OTHER (ZorderTitle, "create", "title", "window create Z-order", "window title", "zorder")

	{NULL},
};


void PluginInit (parerror_t *pe)
{
	DERR st;
	// Tell the rest of the setup that we're a plugin.
	GovSetPlugin ();

	st = PlAddInfoBlock ("governor", instance, IDR_GOVERNINFO);
	fail_if (!DERR_OK (st), st, PE ("Could not add info block \"governor\", {%s,%i}", PlGetDerrString (st), GETDERRINFO (st), 0, 0));
failure:
	return;

}


void PluginDenit (parerror_t *pe)
{
	DERR st;

	PlRemoveInfoBlock ("governor");

	st = GsShutdownGovernor (0, pe);
	fail_if (!st, 0, 0);

failure:
	return;

}


void PluginKill (parerror_t *pe)
{
	//PluginDenit (pe);
	DERR st;

	PlRemoveInfoBlock ("governor");

	st = GsShutdownGovernor (1, pe);
	fail_if (!st, 0, 0);

failure:
	return;
}


//PluginShutdown is called for the special case when the system is being
//shut down.  It is called when a WM_ENDSESSION message is received, so
//the system shutdown is already a foregone conclusion at this point.  All
//this function has to do (and indeed, all that it SHOULD do) is save any
//information that the user would expect to be saved across reboots, and
//cleanup anything that would not be cleaned up by an ExitProcess call.
//No dlls will be unloaded, and the majority of the p5 shutdown procedure
//will be skipped.  Saving the position of a window to the registry is
//an example of the sort of settings that might be saved by this routine.
void PluginShutdown (void)
{
	//the system is shutting down, nothing really important to do.
}




//DllMain, standard entry procedure.  If at all possible, NO work should
//be done by this routine, except for saving the instance handle.  Please
//do not do anything crazy like install Detours hooks in the p5 process!
//If you are using DllMain to install detours, do a check to make sure
//that the exe name of the process you're in isn't p5.exe, or something
//along those lines.
BOOL CALLBACK DllMain (HINSTANCE hInstance, DWORD reason, void *other)
{
	instance = hInstance;
	if (reason == DLL_PROCESS_DETACH)
	{
		GovDprintf (NOISE, "Detach from PID %i, plugin=%i\n", GetCurrentProcessId (), GovIsPlugin ());
		/* Technically, we should only need to do this if GovIsPlugin() is
		 * false; the plugin denit system should be handling it in that
		 * case.  But if we're not a plugin, then we're running as just
		 * a bare DLL in a process, and we need to clean this stuff up.
		 * Calling it unnecessarily is harmless */
		GbCloseHandles ();
	}

	return TRUE;
}