/* Copyright 2013 wParam, licensed under the GPL */

#include <windows.h>
#include "sl.h"

HINSTANCE instance;
int _fltused = 1;
char *PluginName = "stdlib";
char *PluginDescription = "Standard p5 library functions";

//The command you should add to your project is /Zl

/*
	{"!debug", CMD_DebugCommand},
*	{"!message", CMD_WIN_SendaMessage},
*	{"!workarea", CMD_AjustWorkArea},
	{"!shutdown", CMD_Shutdown},
*	{"!cascadewindows", CMD_CascadeWindows},
*	{"!arrangeicons", CMD_ArrangeIcons},
*	{"!tilevertical", CMD_TileVertical},
*	{"!tilehorizontal", CMD_TileHorizontal},
	{"!tasklist", CMD_DisplayTaskMenu},
	{"!startuprun", CMD_DoStartupRun},
	{"!registry", CMD_PerformRegistry},
	{"!msgbox", CMD_MessageBoxPixel},
*	{"!paintmouse", CMD_WIN_PaintAtMouseCursor},
x	{"!lowercase", CMD_WIN_MakeLower},
	{"!msshutdown", CMD_MsShutdown},
*	{"!desktop", CMD_Desktop},
*	{"!appactivate", CMD_WIN_BringToFront},
	{"!noaccel", CMD_MOUSE_KillAcceleration},
	{"!setmouse", CMD_MOUSE_SetMouseValues},
	{"!key", CMD_KEY_KeyboardEvent},
	{"!keydown", CMD_KEY_KeyboardEventDown},
	{"!keyup", CMD_KEY_KeyboardEventUp},
	{"!keyex", CMD_KEY_KeyboardEventEx},
*	{"!alpha", CMD_WIN_AlphaBlend},
*	{"!ontop", CMD_WIN_OnTop},
*	{"!setpos", CMD_WIN_SetPos},
	{"!sound", CMD_DoSound},
	{"!marquee", CMD_Marquee},
	{"!proclist", CMD_ProcMenu},
	{"!terminate", CMD_Terminate},
	{"!timer", CMD_AddTimer},
	{"!alarm", CMD_SetAlarm},
	{"!clock", CMD_Clock},
	{"!alttab", CMD_SwitchTo},
	{"!showkeycode", CMD_VKCode},
	{"!indicate", CMD_CreateIndicator},
	{"!unindicate", CMD_DeleteIndicator},
	{"!killallindicators", CMD_ClearAllIndicators},
	{"!setwindowlong", CMD_SetWindowLong},
	{"!setparent", CMD_SetParent},
	{"!terminatewin", CMD_TerminateWindow},
*	{"!postmessage", CMD_WIN_PostaMessage},
*	{"!trans", CMD_WIN_SetTrans},
	{"!hide", CMD_HideWindow},
	{"!show", CMD_ShowWindow},
	{"!toggleshow", CMD_ToggleShow},
	{"!volume", CMD_SetVolume},
	{"!setfocus", CMD_SetFocus},
	{"!seticon", CMD_SetIcon},
	{"!unseticon", CMD_UnsetIcon},
	{"!unsetallicons", CMD_UnsetAllIcons},
	{"!cleanupseticons", CMD_EmptySetTrash},
	{"!setwindowtext", CMD_SetWindowText},
	{"!windowshade", CMD_Windowshade},
	{"!mouseevent", CMD_MOUSE_MouseEvent},
	{"!cp", CMD_CreateProcess},
	{"!deprop", CMD_DeProp},
	{"!hardterminate", CMD_Terminate},
	{"!swapmouse", CMD_SwapMouse},
	{"!keyboardlight", CMD_KeyboardLight},
	{"!cprestrict", CMD_CreateProcessRestricted},
	{"!setpriv", CMD_SetPrivilege},
	if (strcmp (argv[0], "!std2parm") == 0)
	else if (strcmp (argv[0], "!indicate") == 0)
	else if (strcmp (argv[0], "!getdlgitem") == 0)
	else if (strcmp (argv[0], "!getwindowlong") == 0)
	else if (strcmp (argv[0], "!setwindowlong") == 0)
	else if (strcmp (argv[0], "!math") == 0)
	else if (strcmp (argv[0], "!getparent") == 0)
	else if (strcmp (argv[0], "!setparent") == 0)
	else if (strcmp (argv[0], "!msgbox") == 0)
	else if (strcmp (argv[0], "!getcolor") == 0)
	else if (strcmp (argv[0], "!rand") == 0)
	else if (strcmp (argv[0], "!alarm") == 0)
	else if (strcmp (argv[0], "!volume") == 0)
	else if (strcmp (argv[0], "!iswindowshade") == 0)
	else if (strcmp (argv[0], "!getcursorpos") == 0)
	else if (strcmp (argv[0], "!getwindowrect") == 0)
	else if (strcmp (argv[0], "!debug") == 0)
	else if (strcmp (argv[0], "!getwindowtext") == 0)
	else if (strcmp (argv[0], "!htod") == 0)
	else if (strcmp (argv[0], "!dtoh") == 0)
	else if (strcmp (argv[0], "!getdatetime") == 0)
*	else if (strcmp (argv[0], "!message") == 0)
	else if (strcmp (argv[0], "!strcat") == 0)
	else if (strcmp (argv[0], "!getcommandline") == 0)
	else if (strcmp (argv[0], "!getsidstring") == 0)
	else if (strcmp (argv[0], "!getcurrentuser") == 0)
*/



void DeskCreate (parerror_t *pe, char *deskname, char *commandline);
void DeskSwitch (parerror_t *pe, char *deskname);
void DeskCreateConsole (parerror_t *pe, char *deskname, char *commandline, int fillattrib, int fullscreen, char *workdir, char *title);
char *DeskCurrent (parerror_t *pe);
void DeskEnumDesktops (parerror_t *pe, char *winstation);

void WinBringToFront (parerror_t *pe, HWND win);
void WinRepaint (parerror_t *pe, HWND win);
void WinPostMessage (parerror_t *pe, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int WinSendMessage (parerror_t *pe, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void WinAlphaBlend (parerror_t *pe, HWND hwnd, int blend);
void WinSetTrans (parerror_t *pe, HWND hwnd, COLORREF color);
void WinOnTop (parerror_t *pe, HWND hwnd, int ontop);
void WinSetPos (parerror_t *pe, HWND hwnd, int left, int top, int width, int height);
void WinSetPosSingle (parerror_t *pe, HWND hwnd, int side, int val);
int WinGetPosSingle (parerror_t *pe, HWND hwnd, int side);
void WinWorkArea (parerror_t *pe, int left, int top, int right, int bottom);
void WinWorkAreaSingle (parerror_t *pe, int side, int newval);
int WinGetWorkArea (parerror_t *pe, int side);
int WinTileHorizontal (parerror_t *pe);
int WinTileVertical (parerror_t *pe);
void WinArrangeIcons (parerror_t *pe);
int WinCascadeWindows (parerror_t *pe);
char *WinGetThreadProdId (parerror_t *pe, HWND hwnd);

HWND HwndGetByClass (parerror_t *pe, char *classname);
HWND HwndGetByTitle (parerror_t *pe, char *title, int partial);
HWND HwndGetByMouse (parerror_t *pe);
HWND HwndGetByActive (parerror_t *pe);
HWND HwndGetByMouseDisabled (parerror_t *pe);
HWND HwndGetByFocus (parerror_t *pe);
HWND HwndGetByDlgId (parerror_t *pe, HWND hwnd, int id);
HWND HwndGetParent (parerror_t *pe, HWND hwnd);

HWND WinSetParent (parerror_t *pe, HWND hwnd, HWND newparent);
HWND WinGetParent (parerror_t *pe, HWND hwnd);
int WinGetWindowLong (parerror_t *pe, HWND hwnd, int index);
int WinSetWindowLong (parerror_t *pe, HWND hwnd, int index, int newval);

void WinShowWindow (parerror_t *pe, HWND hwnd);
void WinHideWindow (parerror_t *pe, HWND hwnd);
int WinIsVisible (parerror_t *pe, HWND hwnd);
HWND WinSetFocus (parerror_t *pe, HWND hwnd);
void WinRaise (parerror_t *pe, HWND hwnd);
int WinSetWindowText (parerror_t *pe, HWND hwnd, char *text);
char *WinGetWindowText (parerror_t *pe, HWND hwnd);
void WinSetWindowshade (parerror_t *pe, HWND hwnd);
int WinIsWindowshade (parerror_t *pe, HWND hwnd);

void WinSetForeground (parerror_t *pe, HWND hwnd);


char *SidGetByName (parerror_t *pe, char *machine, char *name);

char *AclCreateEmpty (parerror_t *pe);
int AclIsValid (parerror_t *pe, char *aclin);

void MiscCreateDosDevice (parerror_t *pe, char *devname, char *target);
void MiscDeleteDosDev (parerror_t *pe, char *devname);

char *IcLoadIcon (parerror_t *pe, char *filename, int index);
char *IcGetIconFromWindow (parerror_t *pe, HWND hwnd);
void IcDestroyDualIcon (parerror_t *pe, char *icon);
void IcDrawDualIcon (parerror_t *pe, char *icon, int x, int y);
char *IcChangeDualIconColor (parerror_t *pe, char *iconin, int source, int target);
char *IcRotateDualIconColor (parerror_t *pe, char *iconin, float start, float range, float change);
char *IcHuifyDualIconColor (parerror_t *pe, char *iconin);

void IcWindowClearIcon (parerror_t *pe, HWND hwnd);
void IcWindowSetIcon (parerror_t *pe, HWND hwnd, char *diconin);
int IcWindowSetAutoCleanup (int count, int newval);
void IcWindowCleanup (void);
void IcWindowFinalCleanup (void);
void IcWindowDump (void);
int IcWindowIsSet (parerror_t *pe, HWND hwnd);
void IcWindowReassertIcon (parerror_t *pe, HWND hwnd);




plugfunc_t PluginFunctions[] = 
{
	//functions.  Example:
	//{"rb.show", "ie", RbShow, "Shows the runbox", "(rb.show) = [i:s/f]", "Must run in the main thread"},

	{"sl.misc.dosdev.create", "vess", MiscCreateDosDevice, "Defines a new dos device, such as a drive letter", "(sl.misc.dosdev.create [s:device name] [s:device target]) = [v]", "The two parameters are passed to the DefineDosDevice function with the DDD_RAW_TARGET_PATH flag set.  This function is a gimped version of NtCreateSymbolicLink.  The link is placed in \\??."},
	{"sl.misc.dosdev.delete", "ves", MiscDeleteDosDev, "Deletes a previously defined dos device", "(sl.misc.dosdev.delete [s:device name]) = [v]", "Deletes a dos device created by sl.misc.dosdev.create.  Same as DefineDosDevice API with DDD_REMOVE_DEFINITION set and the third parameter set to NULL."},

	{"sl.desk.create", "vess", DeskCreate, "Creates a new NT desktop and starts a process on it", "(sl.desk.create [s:desktop name] [s:process to run]) = [v]", "If the desktop already exists, this command should still work; another copy of the given process will be started on said desktop.  Starting a process at the same time the desktop is created is required because of the way NT handles its desktops and the fact that I don't want to leave a handle open and return it to script land."},
	{"sl.desk.create", "vessiiss", DeskCreate, "Creates a new NT desktop and starts a process on it, with settings useful for console processes", "(sl.desk.create [s:desktop name] [s:process to run] [i:fill sttribute] [i:bool fullscreen] [s:starting directory] [s:window title]) = [v]", "If the desktop already exists, this command should still work; another copy of the given process will be started on said desktop.  Starting a process at the same time the desktop is created is required because of the way NT handles its desktops and the fact that I don't want to leave a handle open and return it to script land.  Fill attribute is in hex.  Each nibble is a color, MSB are background, LSB are foreground.  Standard 4 bit colors, i.e. blue=1, green=2, red=4, bright=8.  The default white on black is 0F.  Startup directory and window title may be NULL, the system default is used in these cases."},
	{"sl.desk.switch", "ves", DeskSwitch, "Switches the active NT desktop", "(sl.desk.switch [s:desktop name]) = [v]", "The desktop must exist or this function fails."},
	{"sl.desk.cur", "se", DeskCurrent, "Gets the name of the current active desktop", "(sl.desk.cur) = [s:desk name]", "This function is useful for having hotkeys that always run on the current desktop."},
	{"sl.desk.list", "ves", DeskEnumDesktops, "Lists the desktops of the given window station", "(sl.desk.list [s:window station]) = [v]", "If the window station name is NULL, the one owning the p5 process is used.  (This is almost always what you want, and is generally the same as giving \"WinSta0\" for this parameter)"},

	{"sl.sec.sid.getbyname", "tess", SidGetByName, "Gets the SID of a user by username", "(sl.sec.sid.getbyname [s:machine name] [s:user name]) = [s:sid]", "Machine name can be NULL; if this is the case, this function searches the local machine only."},
	{"sl.sec.acl.empty", "te", AclCreateEmpty, "Returns an empty ACL", "(sl.sec.acl.empty) = [t:ACL:empty acl]", "An empty ACL implicitly denies all access to everyone.  Use this function to get an ACL to pass to the ACL add functions if you need to create one from scratch."},
	{"sl.sec.acl.valid", "iet", AclIsValid, "Checks to see if an ACL is valid", "(sl.sec.acl.valid [t:ACL:acl]) = [i:validity]", "Returns nonzero if the ACL is valid, zero if it is not."},

	{"sl.win.activate", "vei", WinBringToFront, "Activates the given window (brings it to the foreground)", "(sl.win.activate [i:hwnd]) = [v]", "Works like clicking on a window in the taskbar."},
	{"sl.win.repaint", "vei", WinRepaint, "Forces the client area of the given window to be repainted", "(sl.win.repaint [i:hwnd]) = [v]", ""},
	{"sl.win.postmessage", "veiiii", WinPostMessage, "Posts a message to a window's message queue", "(sl.win.postmessage [i:hwnd] [i:message] [i:wParam] [i:lParam]) = [v]", "This is just a direct interface to PostMessage()"},
	{"sl.win.sendmessage", "ieiiii", WinSendMessage, "Sends a message to a window", "(sl.win.sendmessage [i:hwnd] [i:message] [i:wParam] [i:lParam]) = [i:message result]", "This is just a direct interface to SendMessage().  SendMessage may block, use it in the foreground thread with care.  (bg.sync) is recommended."},
	{"sl.win.alpha", "veii", WinAlphaBlend, "Applies an alpha blending effect to a window", "(sl.win.alpha [i:hwnd] [i:blend value]) = [v]", "The blend value is an integer ranging from 0 to 255."},
	{"sl.win.trans", "veii", WinSetTrans, "Sets the transparent color for a window", "(sl.win.trans [i:hwnd] [i:color]) = [v]", "The color should be of the 0xrrggbb form.  To clear the transparent color, call this function with color set to -1"},
	{"sl.win.ontop", "veii", WinOnTop, "Sets or clears the always-on-top status for a window", "(sl.win.ontop [i:hwnd] [i:bool ontop]) = [v]", "Always on top means the window stays visible even when it loses focus"},
	
	{"sl.win.setpos", "veiiiii", WinSetPos, "Sets the size and position of a window", "(sl.win.setpos [i:hwnd] [i:x] [i:y] [i:width] [i:height]) = [v]", "Setting any height or width value to -555 means \"no change\""},
	{"sl.win.setpos", "veiii", WinSetPosSingle, "Sets the position or size of a window", "(sl.win.setpos [i:hwnd] [i:side] [i:new value]) = [v]", "Side is 1, 2, 3, or 4 for x, y, width or height, respectively.  The -555 value is not special to this version of setpos.  Use sl.win.getpos if you want to set a position relative to the old one."},
	{"sl.win.getpos", "ieii", WinGetPosSingle, "Gets the position or size value for a window", "(sl.win.getpos [i:hwnd] [i:side]) = [i:value]", "Side is 1, 2, 3, or 4, for x, y, width, or height, respectively."},
	
	{"sl.win.workarea", "veiiii", WinWorkArea, "Sets the screen work area", "(sl.win.workarea [i:left] [i:top] [i:right] [i:bottom]) = [v]", "The work area is the area taken up by a maximized window.  The task bar uses this to stay visible all the time.  To restore the work area to full screen, use (sl.win.workarea 0 0)"},
	{"sl.win.workarea", "veii", WinWorkAreaSingle, "Sets one coordinate of the work area", "(sl.win.workarea [i:side] [i:value]) = [v]", "The side is 1, 2, 3, or 4 for left, top, right, or bottom, respectively.  If side is 0, the work area is restored to full screen.   In this case, the value given is ignored."},
	{"sl.win.getworkarea", "iei", WinGetWorkArea, "Gets one coordinate of the work area", "(sl.win.getworkarea [i:side]) = [i:value]", "Side is 1, 2, 3, or 4 for left, top, right, or bottom."},

	{"sl.win.tilehoriz", "ie", WinTileHorizontal, "Tiles desktop windows horizontally", "(sl.win.tilehoriz) = [i:num tiled]", "The function returns the number of windows tiled, because that's what the API call returns.  This function moves all of the top level windows on the desktop, and cannot be undone."},
	{"sl.win.tilevert", "ie", WinTileHorizontal, "Tiles desktop windows vertically", "(sl.win.tilevert) = [i:num tiled]", "The function returns the number of windows tiled, because that's what the API call returns.  This function moves all of the top level windows on the desktop, and cannot be undone."},
	{"sl.win.arrangeicons", "ve", WinArrangeIcons, "Arranges the minimized windows", "(sl.win.arrangeicons) = [v]", "This function moves the minimized windows back to the standard minimize locations."},
	{"sl.win.cascade", "ie", WinCascadeWindows, "Cascades the desktop windows", "(sl.win.cascade) = [i:num cascaded]", "The function returns the number of windows cascaded, because that's what the API call returns.  This function moves all of the top level windows on the desktop, and cannot be undone."},

	{"sl.win.getparent", "hei", WinGetParent, "Gets the parent of a window", "(sl.win.getparent [i:hwnd]) = [h:parent]", "Just a straight interface with GetParent."},
	{"sl.win.setparent", "heii", WinSetParent, "Sets the parent of a window", "(sl.win.setparent [i:hwnd] [i:new parent hwnd]) = [h:old parent]", "Just a straight interface with SetParent.  The function fails if you try to set the parent of NULL."},
	{"sl.win.getlong", "ieii", WinGetWindowLong, "Calls GetWindowLong to get window information", "(sl.win.getlong [i:hwnd] [i:long index]) = [i:long value]", "Calls straight to GetWindowLong.  Special values for the long index are the GWL_* values defined in the sdk, common ones are:\r\nStyle: -16\r\nExStyle: -20\r\nUser data: -21\r\nId: -12"},
	{"sl.win.setlong", "ieiii", WinSetWindowLong, "Calls SetWindowLong to set window information", "(sl.win.setlong [i:hwnd] [i:long index] [i:new value]) = [i:previous value]", "Calls straight to SetWindowLong.  Special values for the long index are the GWL_* values defined in the sdk, common ones are:\r\nStyle: -16\r\nExStyle: -20\r\nUser data: -21\r\nId: -12"},

	{"sl.win.show", "vei", WinShowWindow, "Shows a window", "(sl.win.show [i:hwnd]) = [v]", "Just a call to ShowWindow() with SW_SHOW."},
	{"sl.win.hide", "vei", WinHideWindow, "Hides a window", "(sl.win.hide [i:hwnd]) = [v]", "Just a call to ShowWindow() with SW_HIDE."},
	{"sl.win.visible", "iei", WinIsVisible, "Checks for window visibility", "(sl.win.visible [i:hwnd]) = [i:bool visible]", "This makes a call to IsWindowVisible.  It does NOT check to see whether it is covered by other windows or not, it simply checks whether it has WS_VISIBLE or not.  (It only checks visibility as controlled by sl.win.hide and sl.win.show.)"},
	{"sl.win.setfocus", "hei", WinSetFocus, "Sets the keyboard focus to the given window.", "(sl.win.setfocus [i:hwnd]) = [h:old focus]", "Just a call to SetFocus, with the requisite AttachThreadInput calls handled if the hwnd is a window in another process (which is almost certainly is).  This function returns the hwnd of the window that previously had focus.  NOTE: This function does not work as far as I can tell.  It may work for setting focus to p5 created windows.  Use of sl.win.setforeground instead is recommended."},
	{"sl.win.setfore", "vei", WinSetForeground, "Brings a window to the foreground and gives it keyboard focus", "(sl.win.setfore [i:hwnd]) = [v]", "This simply passes what you give in to SetForegroundWindow.  NULL is an invalid parameter.  Note that SetForegroundWindow is subject to many inane rules that may make this function fail for no obvious reason.  See MSDN for documentation."},
	{"sl.win.raise", "vei", WinRaise, "Raises a window to the top", "(sl.win.raise [i:hwnd]) = [v]", "Raises the given window to the top of the z-order without activating it or giving it keyboard focus.  NOTE: This function sets the window as topmost for an instant and then as not topmost.  This is necessary because just setting it to HWND_TOP does not seem to work, possibly because of the SetForegroundWindow-esque restrictions.  The function works, but if you use it on a window that's marked as always on top it will have the side effect of clearing that status and making the window normal again."},
	{"sl.win.settext", "ieis", WinSetWindowText, "Sets the text of a window", "(sl.win.settext [i:hwnd] [s:text]) = [i:return val]", "Sends WM_SETTEXT to the given window and returns whatever int the message returns"},
	{"sl.win.gettext", "sei", WinGetWindowText, "Gets the text of a window", "(sl.win.gettext [i:hwnd]) = [s:window text]", "Uses WM_GETTEXT to get a window title.  May return an empty string if the window has no title."},
	{"sl.win.shade", "vei", WinSetWindowshade, "Toggles the windowshade state of a window", "(sl.win.shade [i:hwnd]) = [v]", "Toggles a window between normal view and 'windowshade'.  This is like what you used to be able to do on a macintosh, reducing the window to just a titlebar.  Sometimes windows watch for this and prevent it from working.  Not recommended for use on maximized windows."},
	{"sl.win.isshade", "iei", WinIsWindowshade, "Checks if a window is shaded or not", "(sl.win.isshade [i:hwnd]) = [i:bool resule]", "Checks if a window has been shaded with sl.win.shade.  (Checks for the existence of the PIX_WSmode property.  Will not return true just because a window happens to be reduced to just a titlebar; it must have been shaded with sl.win.shade)."},
	{"sl.win.gettpid", "sei", WinGetThreadProdId, "Gets the thread and process id of a window", "(sl.win.gettpid [i:hwnd]) = [s:pidtid string]", "Direct call to GetWindowThreadProcessId.  Use p5.util.pid and p5.util.tid to split the string into the part you want"},

	{"sl.win.icon.set", "veit", IcWindowSetIcon, "Sets a window's icon", "(sl.win.icon.set [i:hwnd] [t:dicon:new icon]) = [v]", "sl.dicon.* functions give you the \"dual icon\" objects you need to pass in here.  They're \"dual\" because they contain both the large and the small icon.  The icons remain in p5's memory until they are cleaned up, either by deleting, or by calling sl.win.icon.cleanup.  If sl.win.icon.autoclean is enabled, cleanup will automatically happen whenever this function is called."},
	{"sl.win.icon.autoclean", "ic", IcWindowSetAutoCleanup, "Gets the auto cleanup value", "(sl.win.icon.autoclean) = [i:current val]", ""},
	{"sl.win.icon.autoclean", "ici", IcWindowSetAutoCleanup, "Sets the auto cleanup value", "(sl.win.icon.autoclean [i:new val]) = [i:old val]", "The autoclean value controls whether sl.win.icon.cleanup is called automatically when sl.win.icon.set is called.  The default is zero.  Setting it to 1 is recommended.  It defaults to zero for historical (read: stupid) reasons."},
	{"sl.win.icon.unset", "vei", IcWindowClearIcon, "Removes a previously set window icon", "(sl.win.icon.unset [i:hwnd]) = [v]", "This doesn't delete a window's icon, rather it undoes what sl.win.icon.set does.  Generally, the window will revert back to the default icon it had before."},
	{"sl.win.icon.cleanup", "v", IcWindowCleanup, "Cleans up dead icons.", "(sl.win.icon.cleanup) = [v]", "Because of the way windows works, some process has to keep HICON values around for WM_SETICON to work.  This means that p5 holds them after setting them on the target window.  Since there's no easy way to get notified when windows are destroyed, the icons can become \"dead\" when the window that uses them is destroyed.  Calling this function frees any resources associated with icons assigned to windows that no longer exist.  It should be called regularly, or, even better, use sl.win.icon.autoclean to have it be called automatically."},
	{"sl.win.icon.killall", "v", IcWindowFinalCleanup, "Cleans up ALL set icons", "(sl.win.icon.killall) = [v]", "Removes all icons, even those for windows that still exist.  Happens automatically on plugin unload."},
	{"sl.win.icon.dump", "v", IcWindowDump, "Dumps currently held icons", "(sl.win.icon.dump) = [v]", "Output goes to the p5 console"},
	{"sl.win.icon.isset", "iei", IcWindowIsSet, "Checks to see if a window has an icon set on it", "(sl.win.icon.isset [i:hwnd]) = [i:yes/no]", "Returns 1 if an icon was set, 0 if not."},
	{"sl.win.icon.reassert", "vei", IcWindowReassertIcon, "Re-asserts a window's set icon.", "(sl.win.icon.reassert [i:hwnd]) = [v]", "When you set an icon on a window, there is nothing to stop it from changing it to something else.  This function can be used to force the icon back to the one that you previously set, which p5 will still have stored in memory.  An error will be raised if nothing is known about the given window"},

	{"sl.h.class", "hes", HwndGetByClass, "Finds a window by classname", "(sl.h.class [s:classname]) = [h:hwnd]", "Internally this just calls FindWindow with the given classname."},
	{"sl.h.title", "hesi", HwndGetByTitle, "Finds a window by title", "(sl.h.title [s:title] [i:mode]) = [h:hwnd]", "This calls EnumWindows and searches the title one by one.  I've found that searching for titles with FindWindow is unreliable and stupid.  If mode is zero, an exact match is required.  If mode is 1, the title must match up to the number of characters given in the string.  If mode is 2, the string must be a substring of the title.  If mode is 3, then FindWindow is used.  If method 3 works for you, it is probably best to use that.  Enumerating all of the windows sometimes has unexpected failures, such as 'access denied' or 'module not found'.  I wish I was joking about that last one."},
	{"sl.h.mouse", "he", HwndGetByMouse, "Finds a window by the mouse pointer", "(sl.h.mouse) = [h:hwnd]", "This function uses WindowFromPoint to return the window under the mouse cursor.  It will NOT find disabled windows, use sl.h.dmouse for that.  Note that this function will not return the desktop-level window; you should use sl.h.parentmost if you need that."},
	{"sl.h.dmouse", "he", HwndGetByMouseDisabled, "Finds a window by the mouse pointer, including disabled windows", "(sl.h.dmouse) = [h:hwnd]", "This function uses a loop to find disabled child windows.  It works most of the time, but it less efficient than sl.h.mouse, and may work slightly differently (that's why there's two versions."},
	{"sl.h.active", "he", HwndGetByActive, "Finds the active window", "(sl.h.active) = [h:hwnd]", "This returns what GetForegroundWindow returns."},
	{"sl.h.focus", "he", HwndGetByFocus, "Finds the window with keyboard focus", "(sl.h.focus) = [h:hwnd]", "This is similar to sl.h.active, except that it will return a child window, if a child window is what has keyboard focus.  This is what you want to use if you need the window which will receive input from the keyboard (it is often different from GetForegroundWindow())"},
	{"sl.h.parent", "hei", HwndGetParent, "Returns the parent of a window", "(sl.h.parent [i:window]) = [h:hwnd]", "This just calls GetParent () on the window passed in.  It will return NULL if GetParent returns NULL.  An error is raised if you try to get the parent of NULL."},
	{"sl.h.dlgid", "heii", HwndGetByDlgId, "Returns a window handle by control id", "(sl.h.dlgid [i:window] [i:control id]) = [h:hwnd]", "This is a direct interface to GetDlgItem.  Calling this with NULL as the dialog handle results in an error."},

	{"sl.dicon.load", "tesi", IcLoadIcon, "Loads a dual icon from a given file", "(sl.dicon.load [s:filename] [i:index]) = [t:dicon:loaded icon]", "Loads an icon from a file.  The file can be an exe, dll, or ico file.  The index is zero-based; if it is an icon file, the index must be zero."},
	{"sl.dicon.hwnd", "tei", IcGetIconFromWindow, "Gets a dual icon from a window", "(sl.dicon.hwnd [i:hwnd]) = [t:dicon:icon]", "Gets a dicon from an HWND, which can be obtained by any method.  Tries WM_GETICON first, and reverts to GetClassLong if that fails."},
	{"sl.dicon.destroy", "vet", IcDestroyDualIcon, "Destroys a dual icon", "(sl.dicon.destroy [t:dicon:target]) = [v]", "Destroys a dicon returned from one of the other functions"},
	{"sl.dicon.draw", "vetii", IcDrawDualIcon, "Draws a dual icon to the screen", "(sl.dicon.draw [t:dicon] [i:x] [i:y]) = [v]", "Intended mainly for debugging.  Draws the big and little icons at the given coordinates.  Does *NOT* destroy the object."},
	{"sl.dicon.color", "tetii", IcChangeDualIconColor, "Changes one color in a dual icon", "(sl.dicon.color [t:dicon] [i:source color] [i:target color]) = [t:dicon]", "Changes the source color (0xRRGGBB format) to target color, returning the changed icon."},
	{"sl.dicon.rotatecolor", "tetfff", IcRotateDualIconColor, "Rotates the hue of a range of colors in an icon", "(sl.dicon.rotatecolor [t:dicon] [f:start angle] [f:range] [f:delta]) = [t:dicon]", "For each pixel in the icon that has a hue within range degrees of the start angle, delta is added to the hue angle, and the RGB value is recalculated."},
	{"sl.dicon.huify", "tet", IcHuifyDualIconColor, "Fully saturates the colors in an icon (saturation and value to the max)", "(sl.dicon.huify [t:dicon]) = [t:dicon]", "This is somewhat helpful for figuring out what the hue value you need to change is.  It will highlight colors that have a hue you didn't expect"},


	{NULL},
};


void PluginInit (parerror_t *pe)
{

}

void PluginDenit (parerror_t *pe)
{
	IcWindowFinalCleanup ();

}

void PluginKill (parerror_t *pe)
{
	PluginDenit (pe);
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