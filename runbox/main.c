/* Copyright 2006 wParam, licensed under the GPL */

#include <windows.h>

#include "runbox.h"
#include "Resource.h"

HINSTANCE instance;
char *PluginName = "runbox";
char *PluginDescription = "The standard p5 runbox";



//typedef (WINAPI * PFNMSRUN) (HWND, int, int, char*, char*, int);
void RbMsRun (parerror_t *pe)
{
	HMODULE hmod = NULL;
	FARPROC func;
	DERR st;

	hmod = LoadLibrary ("shell32.dll");
	fail_if (!hmod, 0, PE ("Could not load shell32.dll, %i", GetLastError (), 0, 0, 0));

	func = GetProcAddress (hmod, (char*)((long)0x3D));
	fail_if (!func, 0, PE ("Could not GetProcAddress of msrun function, %i", GetLastError (), 0, 0, 0));

	((void (WINAPI *)(HWND, int, int, WCHAR *, char *, int))func) (NULL, 0, 0, L"p5 run", NULL, 0);
	
failure:
	if (hmod)
		FreeLibrary (hmod);

	return;
	
}


int RbShow (parerror_t *pe);
int RbHide (parerror_t *pe);
int RbToggle (parerror_t *pe);
int RbSetHardLimit (int count, int newcount);
int RbSetLineHeight (int count, int newcount);
void RbAddHistoryLine (parerror_t *pe, char *line);
void RbScriptClearHistory (void);
void RbWriteHistory (parerror_t *pe, char *filename);
int RbSetOnTop (parerror_t *pe, int count, int newval);
int RbSetAltTab (int count, int newval);
int RbSetFloating (int count, int newval);
void RbActivate (parerror_t *pe);
int RbIsOpen (void);
int RbSetAllowClose (int count, int newval);
int RbSetMaxHistory (int count, int newval);
int RbSetMultiline (int count, int newval);


plugfunc_t PluginFunctions[] = 
{
	{"rb.show", "ie", RbShow, "Shows the runbox", "(rb.show) = [i:s/f]", "Must run in the main thread"},
	{"rb.hide", "ie", RbHide, "Hides the runbox", "(rb.hide) = [i:s/f]", "Must run in the main thread"},
	{"rb.toggle", "ie", RbToggle, "Toggles visibility of the runbox", "(rb.toggle) = [i:s/f]", "Shows if hidden, and hides if shown.  Must be run in the main thread."},
	{"rb.limit", "ic", RbSetHardLimit, "Gets the current runbox hard limit", "(rb.limit) = [i:hard limit]", ""},
	{"rb.limit", "ici", RbSetHardLimit, "Sets the current runbox hard limit", "(rb.limit [i:new value]) = [i:prev. value]", "The runbox hard limit is the maximum allowed length of a command in the runbox.  It can be set to anything; it exists mostly as a relic of when the buffer to get the data from the edit box was stack based.  It is set at 8192 bytes by default.  I left it in the code because I think it's a good idea to have a guard against accidentally pasting a huge text file into the box and growing the runbox heap unnecessarily."},
	{"rb.height", "ic", RbSetLineHeight, "Gets the current height of lines in the runbox", "(rb.height) = [i:height value]", ""},
	{"rb.height", "ici", RbSetLineHeight, "Sets the height of lines in the runbox", "(rb.height [i:new value]) = [i:prev. value]", "This is given in pixels, but the actual height of the window will be 4 greater than the value given here.  The default value is set to fit properly in the caption bar when caption bars are set to 18, the only size that should ever be considered valid.  Note that you can change this anytime, and the height of the runbox will be altered accordingly, but the font used in the runbox will not change until you close and reopen it.  If you need to change this value, I recommend changing it in your startup script."},
	{"rb.addhistory", "ves", RbAddHistoryLine, "Adds a line to the runbox history", "(rb.addhistory [s:new line]) = [v]", "This is the command written to the saved history file.  The command is added to the end of the history list."},
	{"rb.clearhistory", "ve", RbScriptClearHistory, "Clears the runbox history", "(rb.clearhistory) = [v]", ""},
	{"rb.savehistory", "ves", RbWriteHistory, "Saves the runbox history to a file", "(rb.savehistory [s:filename]) = [v]", "Use this command in your shutdown script to save out the history.  It will create a script full of (rb.addhistory) commands that you can read in with a (script) command in the startup script."},
	{"rb.maxhistory", "ic", RbSetMaxHistory, "Gets the current max history setting", "(rb.maxhistory) = [i:current value]", ""},
	{"rb.maxhistory", "ici", RbSetMaxHistory, "Sets the current max history setting", "(rb.maxhistory [i:new value]) = [i:prev. value]", "If you set this to a smaller value than it was previously, the amount of entries in the history list will not decrease until you clear the history."},
	{"rb.ontop", "iec", RbSetOnTop, "Gets whether the runbox is always on top", "(rb.ontop) = [i:ontop value]", "TRUE implies on top, FALSE implies not on top."},
	{"rb.ontop", "ieci", RbSetOnTop, "Sets whether the runbox is always on top", "(rb.ontop [i:new value]) = [i:old value]", "TRUE implies on top, FALSE implies not on top.  If the runbox is open, and this function is run in the main thread, the setting will be applied immediately."},
	{"rb.inalttab", "ic", RbSetAltTab, "Gets the current show in alt tab setting", "(rb.inalttab) = [i:current value]", ""},
	{"rb.inalttab", "ici", RbSetAltTab, "Sets the current show in alt tab setting", "(rb.inalttab [i:new value]) = [i:prev. value]", "This setting is only used when creating the runbox; you will need to hide and show it again for this to take effect."},
	{"rb.floating", "ic", RbSetFloating, "Gets the current floating setting", "(rb.floating) = [i:current value]", ""},
	{"rb.floating", "ici", RbSetFloating, "Sets the current floating setting", "(rb.floating [i:new value]) = [i:prev. value]", "This setting is only used when creating the runbox; you will need to hide and show it again for this to take effect.  If the runbox is floating, it is recommended that you also show it in alt+tab."},
	{"rb.ms", "ve", RbMsRun, "Opens the standard MS runbox (the one on the start menu)", "(rb.ms) = [v]", ""},
	{"rb.activate", "ve", RbActivate, "Activates the runbox", "(rb.activate) = [v]", ""},
	{"rb.isopen", "i", RbIsOpen, "Checks to see if the runbox is open", "(rb.isopen) = [i:open/closed]", ""},
	{"rb.allowclose", "ic", RbSetAllowClose, "Gets the current allow close setting", "(rb.allowclose) = [i:current value]", ""},
	{"rb.allowclose", "ici", RbSetAllowClose, "Sets the current allow close setting", "(rb.allowclose [i:new value]) = [i:prev. value]", "If this is set to nonzero, the runbox cannot be closed accidentally by pressing alt+f4.  If you depend on the runbox to run your system, you can use this to ensure you don't get stuck with no usable shell.  The rb.hide and rb.toggle command will still work, even if this is set."},
	{"rb.multiline", "ic", RbSetMultiline, "Gets the current multi line setting", "(rb.multiline) = [i:current value]", ""},
	{"rb.multiline", "ici", RbSetMultiline, "Sets the current multi line setting", "(rb.multiline [i:new value]) = [i:prev. value]", "If this is set to nonzero, the runbox will expand to multiple lines when an incomplete p5 expression is entered.  (This is the default setting).  Because it can introduce some strange behaviors (specifically, pressing enter only runs the command if the cursor is at the end), it can be disabled.  This must be set before the runbox is created.  Note that if you have commands in your history that span multiple lines and you set this to single line, interesting things can happen."},

	{NULL},
};


extern parser_t *RbParser;
HBRUSH RbBackgroundBrush = NULL; //need to save it so we can delete it
void PluginInit (parerror_t *pe)
{
	DERR st;
	WNDCLASS wc = {0};
	LOGBRUSH lb = {0}; //WHY THE FUCK IS THERE NO CreateBrush FUNCTION????

	st = PlCreateParser (&RbParser);
	fail_if (!DERR_OK (st), 0, PE ("PlCreateParser returned {%s,%i}", PlGetDerrString (st), GETDERRINFO (st), 0, 0));

	st = PlAddInfoBlock ("runbox", instance, IDR_RUNBOXINFO);
	fail_if (!DERR_OK (st), 0, PE ("PlAddInfoBlock failed, returned {%s,%i}", PlGetDerrString (st), GETDERRINFO (st), 0, 0));

	wc.hCursor = LoadCursor (NULL, IDC_ARROW);
	wc.hInstance = instance;
	wc.lpszClassName = RUNBOX_CLASS;
	wc.lpfnWndProc = RbProc;

	lb.lbColor = GetSysColor (COLOR_3DFACE);
	wc.hbrBackground = RbBackgroundBrush = CreateBrushIndirect (&lb); //fuck you win32
	fail_if (!RbBackgroundBrush, 0, PE ("CreateBrushIndirect failed, %i", GetLastError (), 0, 0, 0));

	//do this last so we don't have to undo it on failure
	st = RegisterClass (&wc);
	fail_if (!st, 0, PE ("RegisterClass failed, %i", GetLastError (), 0, 0, 0));

	return;

failure:
	if (RbParser)
	{
		PlDestroyParser (RbParser);
		RbParser = NULL;
	}

	if (RbBackgroundBrush)
	{
		DeleteObject (RbBackgroundBrush);
		RbBackgroundBrush = NULL;
	}

	return;
}


//THis function should not block execution, it should clean up everything neatly.
//(i.e. NO TerminateThread.)
void PluginDenit (parerror_t *pe)
{
	RbDestroyAll ();
	RbClearHistory ();


	UnregisterClass (RUNBOX_CLASS, instance);

	DeleteObject (RbBackgroundBrush);
	RbBackgroundBrush = NULL;

	PlRemoveInfoBlock ("runbox");

	PlDestroyParser (RbParser);
	RbParser = NULL;

	return;

}

void PluginKill (parerror_t *pe)
{
	PluginDenit (pe);
}

void PluginShutdown (void)
{
	//the system is shutting down, nothing really important to do.
}




//Try to avoid doing anything in here if you can.
BOOL CALLBACK _DllMainCRTStartup (HINSTANCE hInstance, DWORD reason, void *other)
{
	instance = hInstance;
	return TRUE;
}