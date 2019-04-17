/* Copyright 2006 wParam, licensed under the GPL */

#include <windows.h>

HINSTANCE instance;
char *PluginName = "systray";
char *PluginDescription = "System tray implementation";

//The command you should add to your project is /Zl

plugfunc_t PluginFunctions[] = 
{
	//functions.  Example:
	//{"rb.show", "ie", RbShow, "Shows the runbox", "(rb.show) = [i:s/f]", "Must run in the main thread"},

	{NULL},
};


//The init function.  If the plugin fails this, it will be unloaded and the 
//(plug.load) function will return whatever failure is specified in pe. Make
//sure that anything you halfway allocated is removed before returning
//failure.  This function should not block execution.
void PluginInit (parerror_t *pe)
{

}


//This function should not block execution, and should clean up everything
//cleanly.  This means NO CALLING TerminateThread!  TerminateThread causes
//a memory leak (the thread stack) on win2k and older.  Design your plugin so
//that everything can be stopped and closed without having to terminate a
//thread.  If it is not possible, for some reason, to safely denit everything
//when this function is called, it should return failure.  The plugin will
//not be unloaded.  If the user really means "unload, and I don't care if
//there are some leaks" then he will call (plug.kill), and the PluginKill
//function will be called instead.
void PluginDenit (parerror_t *pe)
{

}


//The PluginKill function is the more drastic version of PluginDenit.  It
//is not allowed to block, but restrictions on methods are otherwise much
//more lax.  The goal of this routine is to get the plugin into a state where
//it is safe to unload the DLL from memory, no matter what the consequences.
//TerminateThread is fair game, as are resource and handle leaks.  (But for
//heaven's sake, don't leak unless you have to!)  If you still can't get
//into a state where it is safe to unload, you can fail this function.  The
//user will be prompted for whether he wants to unload the DLL anyway.
//There is no way to prevent p5 from unloading your DLL if that's what the
//user demands.  Note that when (plug.kill) is called, PluginDenit is NOT
//called, so this function will often have to duplicate code from the
//PluginDenit function.  An easy implementation of this function is to
//just call PluginDenit.
void PluginKill (parerror_t *pe)
{
	PluginDenit (pe);
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
BOOL CALLBACK _DllMainCRTStartup (HINSTANCE hInstance, DWORD reason, void *other)
{
	instance = hInstance;
	return TRUE;
}