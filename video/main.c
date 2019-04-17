/* Copyright 2007 wParam, licensed under the GPL */

#include <windows.h>
#include "video.h"

HINSTANCE instance;
char *PluginName = "video";
char *PluginDescription = "Functions to manipulate the display";

//The command you should add to your project is /Zl


void VidChangeDisplaySettings (parerror_t *pe, char *device, int width, int height, int bpp, int frequency, int flags);
void VidReset (parerror_t *pe);
void VidResetAdapter (parerror_t *pe, char *devname);

char *VidGetAdapterName (parerror_t *pe, int index);
char *VidGetMonitorName (parerror_t *pe, char *display, int index);



plugfunc_t PluginFunctions[] = 
{
	//functions.  Example:
	//{"rb.show", "ie", RbShow, "Shows the runbox", "(rb.show) = [i:s/f]", "Must run in the main thread"},
	{"vid.settings", "vesiiiii", VidChangeDisplaySettings, "Changes (or resets) display settings.", "(vid.settings [s:device] [i:width] [i:height] [i:bits per pixel] [i:refresh rate] [i:flags]) = [v]", "This function interfaces with ChangeDisplaySettingsEx.  The device string can be NULL, which will update only the primary display device.  Any of width, height, bits per pixel, or refresh rate can be -1, if this is the case they will not be included in the DEVMODE structure.  Flags are passed directly to ChangeDisplaySettingsEx, see MSDN for usage.  The numerical values are:\r\nCDS_UPDATEREGISTRY  0x00000001\r\nCDS_TEST            0x00000002\r\nCDS_FULLSCREEN      0x00000004\r\nCDS_GLOBAL          0x00000008\r\nCDS_SET_PRIMARY     0x00000010\r\nCDS_VIDEOPARAMETERS 0x00000020\r\nCDS_RESET           0x40000000\r\nCDS_NORESET         0x10000000\r\n\r\nIt is HIGHLY recommended that you have a hotkey bound to vid.reset before you use this function to set a fullscreen (temporary) mode.  Similarly, if you are using the 'update registry' flags, having a hotkey bound to a known-good emergency mode is recommended.  This function does not try to stop you from setting a display mode that your monitor cannot display, use with care!"},
	{"vid.reset", "ve", VidReset, "Resets the display.", "(vid.reset) = [v]", "This function calls ChangeDisplaySettingsEx with a NULL devmode and CDS_RESET specified for the flags.  It will reset the display to the settings stored in the registry.  NOTE: If you used vid.settings to set a mode you can't see and told it to update the registry with the bad settings, this function will not save you.  Test it without saving to the registry first!"},
	{"vid.reset", "ves", VidResetAdapter, "Resets the display on a specific adapter", "(vid.reset [s:device]) = [v]", "This call is the same as vid.reset, except that it passes the given string to ChangeDisplaySettingsEx as the display name."},

	{"vid.adapter.enum", "sei", VidGetAdapterName, "Enumerates display adapters connected to the system", "(vid.adapter.enum [i:index]) = [s:adapter name]", "For the first call, set index to 0, and increment it until the function fails.  This function returns the names of the display devices in the system, use vid.adapter.info to get more information about them or vid,monitor.enum to get monitor information"},

	{"vid.monitor.enum", "sesi", VidGetMonitorName, "Enumerates display monitors connected to an adapter", "(vid.monitor.enum [s:adapter] [i:index]) = [s:monitor name]", "For the first call, set index to 0, and increment it until the function fails.  This function returns the names of the monitors connected to the adapter.  Use vid.adapter.enum to enumerate adapter names."},



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