/* Copyright 2006 wParam, licensed under the GPL */

#include <windows.h>

#include "keyboard.h"
#include "resource.h"

HINSTANCE instance;
char *PluginName = "keyboard";
char *PluginDescription = "Input and hotkey related tasks";

int KbVerbose = 1;

//The command you should add to your project is /Zl

int KbAddHotKey (parerror_t *pe, char *mods, int vk, char *line);
void KbRemoveHotkeyById (parerror_t *pe, int id);
void KbRemoveHotkeyByKey (parerror_t *pe, char *mods, int vk);
int KbRemoveAllHotkeys (parerror_t *pe);
void KbDumpHotkeys (parerror_t *pe);
int KbVk (parerror_t *pe, char *str);
char *KbUnvk (parerror_t *pe, unsigned int vk);
void KbAddMonitor (parerror_t *pe, char *sequence, char *line);
void KbRemoveMonitor (parerror_t *pe, char *sequence);
void KbRemoveAllMonitors (parerror_t *pe);
void KbPrintMonitors (parerror_t *pe);
void KbEnableMonitor (parerror_t *pe);
int KbDisableMonitor (parerror_t *pe);

int KbSetPauseMax (int count, parerror_t *pe, int newval);
int KbSetPauseKey (int count, int newval);
int KbSetPauseAbortKey (int count, int newval);
int KbPermitPause (int count, int newval);
int KbSetPauseDrawing (int count, int newval);

void KbSetVerbose (int v);


plugfunc_t PluginFunctions[] = 
{
	//functions.  Example:
	{"kb.hotkey.add", "iesia", KbAddHotKey, "Registers a new system-wide hotkey.", "(kb.hotkey [s:modifiers] [i:vk code] [a:command]) = [i:identifier]", "The string should consist only of the letters a, c, s, and w, for the alt, control, shift, and windows keys, respectively.  The characters in the modifier string can occur in any order.  Modifier can be an empty string if you mean no modifiers, but it should not be NULL.  The function will fail if a key is already registered with the given modifiers and keys.  Use the (kb.vk) command to translate a string to an integer keycode.  See (info \"hotkeys\") for more information."},
	{"kb.hotkey.del", "vei", KbRemoveHotkeyById, "Removes a previously registered hotkey by id", "(kb.unhotkey [i:id]) = [v]", "The id is a value returned by kb.hotkey.  (You can also use kb.dumphotkeys to find one.)  See (info \"hotkeys\") for more information."},
	{"kb.hotkey.del", "vesi", KbRemoveHotkeyByKey, "Removes a previously registered hotkey by the modifiers and vkcode used to register it", "(kb.unhotkey [s:mods] [i:vk]) = [v]", "The mods and vk must be the same ones passed to (kb.hotkey).  (You can also use kb.dumphotkeys to find these values.)  See (info \"hotkeys\") for more information."},
	{"kb.hotkey.del", "ve", KbRemoveAllHotkeys, "Removes all hotkeys", "(kb.unhotkey) = [v]", "See (info \"hotkeys\") for more information."},
	{"kb.hotkey.dump", "ve", KbDumpHotkeys, "Lists all registered hotkeys", "(kb.dumphotkeys) = [v]", "See (info \"hotkeys\") for more information."},

	{"kb.vk", "ies", KbVk, "Translate a string into an integer keycode", "(kb.vk [s:key name]) = [i:vk code]", "The keys have names like A, 6, SPACE, NUMLOCK, NUMPAD5, PGUP, CTRL, etc.  They are case sensitive.  In general, a key is represented by what it prints when you press it without holding shift.  You can also prepend an integer with a ~ to have kb.vk return that integer, so (kb.vk \"~88\") will return 88.  This is a side effect of how the code is designed; it is useful internally."},
	{"kb.unvk", "sei", KbUnvk, "Translate an integer keycode into a string", "(kb.unvk [i:vk code]) = [s:key name]", "This uses the same table as kb.vk.  If the given integer isn't found, this function returns a ~xxx string."},
	{"kb.verbose", "vi", KbSetVerbose, "Toggles printing of hotkey and monitor output to the console", "(kb.verbose [i:on/off]) = [v]", "Nonzero enables the prints, zero disables them."},

	{"kb.mon.add", "vesa", KbAddMonitor, "Add a key sequence to the monitor database", "(kb.monitor [s:key sequence] [a:command]) = [v]", "The key sequence is a space-delimited list of keys that must be pressed after pause to trigger the command.  The keys should be in the same format as the kb.vk command accepts.  If you specify a key sequence that already exists, its command will be replaced by the new command.  Two special \"keys\" may be used, #, which matches any valid decimal number, and *, which matches as many characters as possible.  Neither # nor * can be the last character in the sequence.  The behavior is undefined if two commands overlap.  (For example, if you register both u and ui, or s#q and s234q.)  See (info \"monitor\") for a general overview."},
	{"kb.mon.del", "ves", KbRemoveMonitor, "Remove a key sequence from the monitor database", "(kb.unmonitor [s:key sequence]) = [v]", "The key sequence needs to match functionally with what you passed to kb.monitor; this does NOT necessarily mean that it is identical, e.g. \"Q SPACE\" and \"A ~32\" both resolve to the same sequence of virtual keys and thus would be considered a match for removal.  See (info \"monitor\") for a general overview."},
	{"kb.mon.reset", "ve", KbRemoveAllMonitors, "Clears all key sequences from the monitor database", "(kb.resetmonitor) = [v]", "See (info \"monitor\") for a general overview."},
	{"kb.mon.start", "ve", KbEnableMonitor, "Enables the monitor hook", "(kb.startmonitor) = [v]", "You need to use this command before any of your monitor's will trigger.  See (info \"monitor\") for a general overview."},
	{"kb.mon.stop", "ve", KbDisableMonitor, "Disables the monitor hook", "(kb.stopmonitor) = [v]", "See (info \"monitor\") for a general overview."},
	{"kb.mon.dump", "ve", KbPrintMonitors, "Prints a list of the monitor database", "(kb.dumpmonitor) = [v]", "See (info \"monitor\") for a general overview."},

	{"kb.mon.maxlen", "icei", KbSetPauseMax, "Sets the maximum length of a monitor sequence", "(kb.mon.maxlen [i:new max]) = [i:old len]", "The default length is 127.  The length refers to how many characters are typed, not how many are in the sequence, i.e. # and * count as the number of keys they catch, not just 1.  The max cannot be changed while the monitor is enabled."},
	{"kb.mon.maxlen", "ice", KbSetPauseMax, "Gets the maximum length of a monitor sequence", "(kb.mon.maxlen) = [i:current val]", ""},
	{"kb.mon.activekey", "ici", KbSetPauseKey, "Sets the key that activates monitor sequences", "(kb.mon.activekey [i:new key]) = [i:old key]", "This is the key that you push to signal that you're about to start a monitor sequence.  The default key is the pause key.  The value should be a virtual key identifier, (kb.vk) will translate string keys into identifiers."},
	{"kb.mon.activekey", "ic", KbSetPauseKey, "Gets the key that activates monitor sequences", "(kb.mon.activekey) = [i:current key]", ""},
	{"kb.mon.abortkey", "ici", KbSetPauseAbortKey, "Sets the key that aborts monitor sequences", "(kb.mon.abortskey [i:new key]) = [i:old key]", "This is the key that you push to stop monitor processing at any point.  The default key is escape.  The value should be a virtual key identifier, (kb.vk) will translate string keys into identifiers."},
	{"kb.mon.abortkey", "ic", KbSetPauseAbortKey, "Gets the key that aborts monitor sequences", "(kb.mon.abortskey) = [i:current key]", ""},
	{"kb.mon.permits", "ici", KbPermitPause, "Sets the current number of permits", "(kb.mon.permits [i:new value]) = [i:old value]", "\"Permits\" refers to the number of times the monitor activate key will be allowed to pass through.  Normally, the monitor activate key is eaten and applications do not see it.  Historically, however, the activator key was allowed to pass, so this option was added to allow it to pass when and if the user wants it to.  This value is decremented every time an activator is permitted through."},
	{"kb.mon.permits", "ic", KbPermitPause, "Gets the current number of permits", "(kb.mon.permits) = [i:current value]", ""},
	{"kb.mon.drawing", "ici", KbSetPauseDrawing, "Sets whether the monitor draws the keys you press on the screen", "(kb.mon.drawing [i:new value]) = [i:old value]", "By default, monitor draws the keys you press to the upper left corner of the screen.  This is done with complete disregard to any windows that might be there, and it is not erased when you are done entering the sequence.  If this bothers you, you can turn it off by passing 0 to this function."},
	{"kb.mon.drawing", "ic", KbSetPauseDrawing, "Gets the state of monitor drawing", "(kb.mon.drawing) = [i:current value]", ""},

	{NULL},
};



void PluginInit (parerror_t *pe)
{
	DERR st;

	st = PlAddMessageCallback (WM_HOTKEY, 0, 0xFFFFFC00, 0, 0, KbHotkeyCallback);
	fail_if (!st, 0, PE ("Could not grab WM_HOTKEY callback registration", 0, 0, 0, 0));

	st = PlAddInfoBlock ("monitor", instance, IDR_MONITOR);
	fail_if (!DERR_OK (st), st, PE ("Could not add info block, {%s,%i}", PlGetDerrString (st), GETDERRINFO (st), 0, 0));
	st = PlAddInfoBlock ("hotkeys", instance, IDR_MONITOR);
	fail_if (!DERR_OK (st), st, PE ("Could not add info block, {%s,%i}", PlGetDerrString (st), GETDERRINFO (st), 0, 0));

failure:
	return;

}



void PluginDenit (parerror_t *pe)
{
	DERR st;

	PlRemoveInfoBlock ("monitor");
	PlRemoveInfoBlock ("hotkeys");

	st = KbRemoveAllHotkeys (pe);
	fail_if (!st, 0, 0);

	st = KbPauseShutdown (pe);
	fail_if (!st, 0, 0);

	st = PlRemoveMessageCallback (WM_HOTKEY, 0, 0xFFFFFC00, 0, 0, KbHotkeyCallback);
	fail_if (!st, 0, PE ("Could not remove WM_HOTKEY callback; unloading is unsafe.", 0, 0, 0, 0));

failure:
	return;

}


void PluginKill (parerror_t *pe)
{
	DERR st;

	PlRemoveInfoBlock ("monitor");
	PlRemoveInfoBlock ("hotkeys");

	st = KbForcedHotkeyCleanup (pe);
	fail_if (!st, 0, 0);

	st = KbPauseShutdown (pe);
	fail_if (!st, 0, 0);

	st = PlRemoveMessageCallback (WM_HOTKEY, 0, 0xFFFFFC00, 0, 0, KbHotkeyCallback);
	fail_if (!st, 0, PE ("Could not remove WM_HOTKEY callback; unloading is unsafe.", 0, 0, 0, 0));

failure:
	return;
}



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