/* Copyright 2008 wParam, licensed under the GPL */

#include <windows.h>

#include "instrument.h"
#include "resource.h"


HINSTANCE instance;
char *PluginName = "instrument";
char *PluginDescription = "API and window hooking";

//The command you should add to your project is /Zl


void MnTestFunction (parerror_t *pe, char *actions);
char *ActnParserRun (parerror_t *pe);
char *ActnParserSetReturn (parerror_t *pe, int val);
char *ActnParserNop (parerror_t *pe);
char *ActnParserNopstring (parerror_t *pe, char *param);
char *ActnParserNopintint (parerror_t *pe, int val, int val2);
char *ActnParserNopvar (parerror_t *pe, char *atom);
char *ActnParserLabel (parerror_t *pe, char *atom);
char *ActnParserPrint (parerror_t *pe, char *param);
char *ActnParserMfpar (parerror_t *pe, char *var, int parindex);
char *ActnParserMtpar (parerror_t *pe, int parindex, char *var);
char *ActnParserInc (parerror_t *pe, char *atom);
char *ActnParserMfret (parerror_t *pe, char *atom);
char *ActnParserMtret (parerror_t *pe, char *atom);
char *ActnParserMov (parerror_t *pe, char *atom1, char *atom2);
char *ActnParserSet (parerror_t *pe, char *atom1, int val);
char *ActnParserAdd (parerror_t *pe, char *atom1, char *atom2);
char *ActnParserSub (parerror_t *pe, char *atom1, char *atom2);
char *ActnParserMul (parerror_t *pe, char *atom1, char *atom2);
char *ActnParserDiv (parerror_t *pe, char *atom1, char *atom2);
char *ActnParserCallDll (parerror_t *pe, char *param1, char *param2);
char *ActnParserCallDllSlow (parerror_t *pe, char *param1, char *param2);
char *ActnParserCmpns (parerror_t *pe, char *atom1, char *atom2, int n);
char *ActnParserCmps (parerror_t *pe, char *atom1, char *atom2);
char *ActnParserCmpi (parerror_t *pe, char *atom1, char *atom2);
char *ActnParserCmpsWide (parerror_t *pe, char *atom1, char *atom2);
char *ActnParserCmpnsWide (parerror_t *pe, char *atom1, char *atom2, int n);
char *ActnParserCmpnsAnsiWide (parerror_t *pe, char *atom1, char *atom2, int n);
char *ActnParserCmpsAnsiWide (parerror_t *pe, char *atom1, char *atom2);
char *ActnParserJmp (parerror_t *pe, char *atom);
char *ActnParserJeq (parerror_t *pe, char *atom);
char *ActnParserJne (parerror_t *pe, char *atom);
char *ActnParserJgt (parerror_t *pe, char *atom);
char *ActnParserJlt (parerror_t *pe, char *atom);
char *ActnParserJge (parerror_t *pe, char *atom);
char *ActnParserJle (parerror_t *pe, char *atom);
char *ActnParserLoad (parerror_t *pe, char *atom1, char *atom2);
char *ActnParserStore (parerror_t *pe, char *atom1, char *atom2);
char *ActnParserSetString (parerror_t *pe, char *atom1, char *string);
char *ActnParserMfglob (parerror_t *pe, char *var, int parindex);
char *ActnParserMtglob (parerror_t *pe, int parindex, char *var);
char *ActnParserNumGlobals (parerror_t *pe, int numg);
char *ActnParserBufferSize (parerror_t *pe, int buf);
char *ActnParserGetbuf (parerror_t *pe, char *atom);
char *ActnParserStrncat (parerror_t *pe, char *target, char *source, int maxlen);
char *ActnParserStrncatWide (parerror_t *pe, char *target, char *source, int maxlen);
char *ActnParserIntncat (parerror_t *pe, char *target, int svar, int maxlen);
char *ActnParserAtow (parerror_t *pe, char *target, int tlen, char *source);
char *ActnParserWtoa (parerror_t *pe, char *target, int tlen, char *source);
char *ActnParserStrstr (parerror_t *pe, char *atom1, char *atom2, char *atom3);
char *ActnParserStrstrWide (parerror_t *pe, char *atom1, char *atom2, char *atom3);
char *ActnParserPrintsWide (parerror_t *pe, char *atom);
char *ActnParserPrints (parerror_t *pe, char *atom);
char *ActnParserP5Parse (parerror_t *pe, char *atom);
char *ActnParserMsgbox (parerror_t *pe, char *atom1, char *atom2, char *atom3, int props);
char *ActnParserMsgboxWide (parerror_t *pe, char *atom1, char *atom2, char *atom3, int props);
char *ActnParserGetGlobals (parerror_t *pe, char *atom);
char *ActnParserAnd (parerror_t *pe, char *atom1, char *atom2);
char *ActnParserOr (parerror_t *pe, char *atom1, char *atom2);
char *ActnParserNot (parerror_t *pe, char *atom);
char *ActnParserSetLastError (parerror_t *pe, int val);
char *ActnParserSetLastErrorVar (parerror_t *pe, char *atom);
char *ActnParserUpcaseString (parerror_t *pe, char *atom);
char *ActnParserUpcaseStringWide (parerror_t *pe, char *atom);



int MnIsProcessHooked (DWORD pid);
int MnScriptForceHookProcess (DWORD pid, parerror_t *pe);
int MnScriptHookProcess (DWORD pid, parerror_t *pe);
int MnSetFunctionInstrument (parerror_t *pe, int pid, char *lib, char *func, int stdcall, int numparams, char *action);
int MnRemoveFunctionInstrument (parerror_t *pe, int pid, char *lib, char *func, int stdcall, int numparams);

int MnSetWindowInstrumentFull (parerror_t *pe, int pid, HWND hwnd, UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask, char *action);
int MnSetWindowInstrumentBasic (parerror_t *pe, int pid, HWND hwnd, UINT message, char *actions);
int MnRemoveWindowInstrumentFull (parerror_t *pe, int pid, HWND hwnd, UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask);
int MnRemoveWindowInstrumentBasic (parerror_t *pe, int pid, HWND hwnd, UINT message);

int MnSetAddressInstrument (parerror_t *pe, int pid, int addy, int stdcall, int numparams, char *action);
int MnRemoveAddressInstrument (parerror_t *pe, int pid, int addy, int stdcall, int numparams);

int MnTestHookie (parerror_t *pe);


plugfunc_t PluginFunctions[] = 
{
	//functions.  Example:
	//{"i.test", "ves", MnTestFunction, "", "", ""},
	//{"i.test2", "ie", MnTestHookie, "", "", ""},

	{"i.hook.check", "ii", MnIsProcessHooked, "Check to see if a process is hooked.", "(i.hook.check [i:pid]) = [i:val]", "Returns 1 if the process is hooked already, 0 otherwise.  This function checks for the instrument-pid-xxx event existing or not.  In no circumstances will an error be raised."},
	{"i.hook.force", "iie", MnScriptForceHookProcess, "Hook a process, whether it has already been hooked or not", "(i.hook.force [i:pid]) = [i:pid]", "This function returns the same pid that it was passed.  It is here just in case the hook detection logic fails.  It will attempt to inject the instrument dll into the target process, even if it appears to already be there.  You should always use i.hook.set unless there's something wrong with it."},
	{"i.hook.set", "iie", MnScriptHookProcess, "Hook a process if it isn't already", "(i.hook.set [i:pid]) = [i:pid]", "This function returns the same pid it was passed, so it can be used as the pid argument for one of the i.set.* functions.  If the process is already hooked (as detected by i.hook.check), the function does nothing and returns success.  If it is not already hooked, the function will attempt to hook it.  This function also waits a short time to give the target process time to get everything set up before returning.  If the process hook does not appear to be active after the timeout period (currently 1 second) an error is raised."},

	{"i.set.function", "ieissiis", MnSetFunctionInstrument, "Set a hook on a function by dll and export name", "(i.set.function [i:pid] [s:dll name] [s:function name] [i:bool stdcall] [i:numparams] [s:actions]) = [i:ack/no ack]", "The function returns 1 if the instrument was aknowleged, or 0 if it was not.  (Note that lack of an ACK response does not always mean the operation was unsuccessful.)  Stdcall refers to the calling convention, the parameter should be 1 if __stdcall is used (the case for almost all win32 api functions) or 0 if __cdecl is used.  Numparams is the number of 32 bit parameters the function takes.  The dll given will be loaded if it is not resident in the target process.  Actions should come from the i.op functions.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.set.address", "ieiiiis", MnSetAddressInstrument, "Set a hook on a function by address", "(i.set.address [i:pid] [i:address] [i:bool stdcall] [i:numparams] [s:actions]) = [i:ack/no ack]", "The function returns 1 if the instrument was aknowleged, or 0 if it was not.  (Note that lack of an ACK response does not always mean the operation was unsuccessful.)  Stdcall refers to the calling convention, the parameter should be 1 if __stdcall is used (the case for almost all win32 api functions) or 0 if __cdecl is used.  Numparams is the number of 32 bit parameters the function takes.  No DLL loading will take place.  The address given must be the start of a function in the target process or bad things will occur.  Actions should come from the i.op functions.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.set.fullwindow", "ieiiiiiiis", MnSetWindowInstrumentFull, "Sets a hook on a window procedure based on the message, wParam, and lParam", "(i.set.fullwindow [i:pid] [i:hwnd] [i:message] [i:wParam] [i:wMask] [i:lParam] [i:lMask] [s:actions]) = [i:ack/no ack]", "The function returns 1 if the instrument was aknowleged, or 0 if it was not.  (Note that lack of an ACK response does not always mean the operation was unsuccessful.).  This function can filter the instrument based on wParam and lParam.  The test used to decide whether the actions will be run is:\r\n\r\nif (msg.wParam & wMask == wParam)\r\n\r\n.  So, if you want to match any wParam, pass 0 for both wParam and wMask, if you want to match only when wParam == 5, pass 5 for wParam and 0xFFFFFFFF for wMask.  Actions should come from the i.op functions.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.set.window", "ieiiis", MnSetWindowInstrumentBasic, "Sets a hook on a window procedure based on the message", "(i.set.window [i:pid] [i:hwnd] [i:message] [s:actions]) = [i:ack/no ack]", "The function returns 1 if the instrument was aknowleged, or 0 if it was not.  (Note that lack of an ACK response does not always mean the operation was unsuccessful.).  Actions should come from the i.op functions.  See (info \"instrument\") for some help in making sense of all this."},

	{"i.unset.function", "ieissii", MnRemoveFunctionInstrument, "Remove a hook on a function by dll and export name", "(i.unset.function [i:pid] [s:dll name] [s:function name] [i:book stdcall] [i:numparams]) = [i:ack/no ack]", "The function returns 1 if the requst was aknowleged, or 0 if it was not.  All of the parameters given here must match those given to i.set.function, including the case of the filename, or the unset will not work.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.unset.address", "ieiiii", MnRemoveAddressInstrument, "Remove a hook on a function by address", "(i.unset.address [i:pid] [i:address] [i:book stdcall] [i:numparams]) = [i:ack/no ack]", "The function returns 1 if the requst was aknowleged, or 0 if it was not.  All of the parameters given here must match those given to i.set.address, or the unset will not work.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.unset.fullwindow", "ieiiiiiii", MnRemoveWindowInstrumentFull, "Removes a hook on a window procedure based on the message, wParam, and lParam", "(i.unset.fullwindow [i:pid] [i:hwnd] [i:message] [i:wParam] [i:wMask] [i:lParam] [i:lMask]) = [i:ack/no ack]", "The function returns 1 if the requst was aknowleged, or 0 if it was not.  The parameters given must match those given to i.set.fullwindow exactly or it will not work.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.unset.window", "ieiii", MnRemoveWindowInstrumentBasic, "Removes a hook on a window procedure based on the message", "(i.unset.fullwindow [i:pid] [i:hwnd] [i:message]) = [i:ack/no ack]", "The function returns 1 if the requst was aknowleged, or 0 if it was not.  The parameters given must match those given to i.set.fullwindow exactly or it will not work.  See (info \"instrument\") for some help in making sense of all this."},

	{"i.op.run", "se",			ActnParserRun, "Instrument op: Run target function or message", "(i.op.run) = [s:op string]", "The return value of the instrument is automatically set to the return value of the run.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.setreturn", "sei",	ActnParserSetReturn, "Instrument op: Set return value", "(i.op.setreturn [i:new val]) = [s:op string]", "Set the return value of the instrument.  (This refers to what will be returned to the source application after the instrument is done).  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.nop", "se",			ActnParserNop, "Instrument op: No operation", "(i.op.nop) = [s:op string]", "No operation.  Intended mainly for debugging."},
	{"i.op.nops", "ses",		ActnParserNopstring, "Instrument op: No operation", "(i.op.nops [s]) = [s:op string]", "No operation.  Intended mainly for debugging.  The string parameter is ignored."},
	{"i.op.nopii", "seii",		ActnParserNopintint, "Instrument op: No operation", "(i.op.nopii [i] [i]) = [s:op string]", "No operation.  Intended mainly for debugging.  The parameters are ignored."},
	{"i.op.nopv", "sea",		ActnParserNopvar, "Instrument op: No operation", "(i.op.nopv [a]) = [s:op string]", "No operation.  Intended mainly for debugging.  The parameters are ignored."},
	{"i.op.label", "sea",		ActnParserLabel, "Instrument op: Set label", "(i.op.label [a:label name]) = [s:op string]", "Mark a label in the action stream.  The label name can be any atom, however, only the first letter will be considered.  (So \'Abba\' and \'Aerosmith\' are the same label).  Unlike variables, however, you can use any character you want (except for a null terminator.  Do not pass (stoa "") as the atom for your label.)  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.print", "ses",		ActnParserPrint, "Instrument op: Print string (to debugview)", "(i.op.print [s:string to print]) = [s:op string]", "The given string is printed to debugview.  Null strings are treated as empty strings.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.mfpar", "seai",		ActnParserMfpar, "Instrument op: Move from parameter", "(i.op.mfpar [a:variable name] [i:parameter index]) = [s:op string]", "Sets the given variable equal to the parameter.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.mtpar", "seia",		ActnParserMtpar, "Instrument op: Move to parameter", "(i.op.mtpar [i:parameter index] [a:variable name]) = [s:op string]", "Sets the given parameter equal to the given var.  It only makes sense to call this before the call to i.op.run is made.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.inc", "sea",			ActnParserInc, "Instrument op: Increment variable", "(i.op.inc [a:var]) = [s:op string]", "Increments the given variable by one.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.mfret", "sea",		ActnParserMfret, "Instrument op: Move from return value", "(i.op.mfret [a:target var]) = [s:op string]", "Sets the given variable to the instrument's return value.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.mtret", "sea",		ActnParserMtret, "Instrument op: Move to return value", "(i.op.mtret [a:source var]) = [s:op string]", "Moves the given variable to the instrument's return value.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.mov", "seaa",		ActnParserMov, "Instrument op: Move variable", "(i.op.mov [a:target] [a:source]) = [s:op string]", "Set one variable equal to another.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.set", "seai",		ActnParserSet, "Instrument op: Set variable", "(i.op.set [a:target var] [i:new value]) = [s:op string]", "Set one variable to an integer value.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.add", "seaa",		ActnParserAdd, "Instrument op: Add to variable", "(i.op.add [a:var A] [a:var B]) = [s:op string]", "Sets variable A equal to (A + B).  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.sub", "seaa",		ActnParserSub, "Instrument op: Subtract from variable", "(i.op.sub [a:var A] [a:var B]) = [s:op string]", "Sets variable A equal to (A - B).  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.mul", "seaa",		ActnParserMul, "Instrument op: Multiply by variable", "(i.op.mul [a:var A] [a:var B]) = [s:op string]", "Sets variable A equal to (A * B).  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.div", "seaa",		ActnParserDiv, "Instrument op: Divide by variable", "(i.op.div [a:var A] [a:var B]) = [s:op string]", "Sets variable A equal to (A / B).  A is set to 0x7FFFFFFF if B is zero.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.calldll", "sess",	ActnParserCallDll, "Instrument op: Load and call DLL function", "(i.op.calldll [s:dll name] [s:function name]) = [s:op string]", "The dll is loaded into memory if necessary.  It will not be unloaded after the call is made, for efficency reasons (the address of the function is saved on the first execution to speed things up in subsequent calls.  If you really want the dll loaded and unloaded from the address space every time the instrument is run, use loadcalldll.)  The function in the dll must have a prototype that matches:\r\n\r\nint __cdecl funcname (void **params, int numparams, int *vars, int numvars, int *retvalue, int *globals, int numglobals, char *scratch);\r\n\r\nIt should return 1 if all is well, or 0 to abort the hook processing.  (If it returns 0, whatever is currently in the return value will be returned.)  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.loadcalldll", "sess",ActnParserCallDllSlow, "Instrument op: Load and call DLL function, always unloading the dll upon completion", "(i.op.loadcalldll [s:dll name] [s:function name]) = [s:op string]", "Loadcalldll works the same way as calldll, except that loadcalldll will always use LoadLibrary, and will always call FreeLibrary after calling the function.  This will result in a lot of unnecessary loads and unloads, and should not be used unless there is some reason why the dll needs to be unloaded from the address space every time it is used.  i.op.calldll is MUCH more efficient.  The function in the dll must have a prototype that matches:\r\n\r\nint __cdecl funcname (void **params, int numparams, int *vars, int numvars, int *retvalue, int *globals, int numglobals, char *scratch);\r\n\r\nIt should return 1 if all is well, or 0 to abort the hook processing.  (If it returns 0, whatever is currently in the return value will be returned.)  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.cmpi", "seaa",		ActnParserCmpi, "Instrument op: Integer compare", "(i.op.cmpi [a:var A] [a:var B]) = [s:op string]", "Sets the equality register to the appropriate result.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.cmps", "seaa",		ActnParserCmps, "Instrument op: String compare", "(i.op.cmps [a:var A] [a:var B]) = [s:op string]", "Sets the equality register to the appropriate result.  The two variables must contain pointers to strings.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.cmpns", "seaai",		ActnParserCmpns, "Instrument op: Limited string compare", "(i.op.cmpns [a:var A] [a:var B] [i:max chars]) = [s:op string]", "Sets the equality register to the appropriate result.  The two variables must contain pointers to strings.  Internally this works the same way as the ANSI C strncmp function.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.cmpsw", "seaa",		ActnParserCmpsWide, "Instrument op: Unicode string compare", "(i.op.cmpsw [a:var A] [a:var B]) = [s:op string]", "Sets the equality register to the appropriate result.  The two variables must contain pointers to wide strings.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.cmpnsw", "seaai",	ActnParserCmpnsWide, "Instrument op: Limited unicode string compare", "(i.op.cmpnsw [a:var A] [a:var B] [i:max chars]) = [s:op string]", "Sets the equality register to the appropriate result.  The two variables must contain pointers to wide strings.  Internally this works the same way as the ANSI C strncmp function.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.cmpsaw", "seaa",		ActnParserCmpsAnsiWide, "Instrument op: Ansi/unicode string compare", "(i.op.cmpsaw [a:var A] [a:var B]) = [s:op string]", "Sets the equality register to the appropriate result.  Variable A should contain a pointer to an ansi string, and variable B should contain a pointer to a wide string.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.cmpnsaw", "seaai",	ActnParserCmpnsAnsiWide, "Instrument op: Limited ansi/unicode string compare", "(i.op.cmpnsaw [a:var A] [a:var B] [i:max chars]) = [s:op string]", "Sets the equality register to the appropriate result.  Variable A should contain a pointer to an ansi string, and variable B should contain a pointer to a wide string.  Internally this works the same way as the ANSI C strncmp function.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.jmp", "sea",			ActnParserJmp, "Instrument op: Unconditional branch", "(i.op.jmp [a:target label]) = [s:op string]", "Transfers execution to the given label, defined by i.op.label.  (The commands must be searched sequentially, starting over from the beginning, so if you have multiple i.op.label commands with the same label, the first one found will always be jumped to.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.jeq", "sea",			ActnParserJeq, "Instrument op: Branch if equal", "(i.op.jeq [a:target label]) = [s:op string]", "Transfers execution to the given label, if the equality register shows equality.  (Labels are defined by i.op.label.)  (The commands must be searched sequentially, starting over from the beginning, so if you have multiple i.op.label commands with the same label, the first one found will always be jumped to.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.jne", "sea",			ActnParserJne, "Instrument op: Branch if not equal", "(i.op.jne [a:target label]) = [s:op string]", "Transfers execution to the given label, if the equality register shows inequality.  (Labels are defined by i.op.label.)  (The commands must be searched sequentially, starting over from the beginning, so if you have multiple i.op.label commands with the same label, the first one found will always be jumped to.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.jgt", "sea",			ActnParserJgt, "Instrument op: Branch if greater than", "(i.op.jgt [a:target label]) = [s:op string]", "Transfers execution to the given label, if the equality register shows greater than.  (Labels are defined by i.op.label.)  (The commands must be searched sequentially, starting over from the beginning, so if you have multiple i.op.label commands with the same label, the first one found will always be jumped to.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.jlt", "sea",			ActnParserJlt, "Instrument op: Branch if less than", "(i.op.jlt [a:target label]) = [s:op string]", "Transfers execution to the given label, if the equality register shows less than.  (Labels are defined by i.op.label.)  (The commands must be searched sequentially, starting over from the beginning, so if you have multiple i.op.label commands with the same label, the first one found will always be jumped to.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.jge", "sea",			ActnParserJge, "Instrument op: Branch if greater than or equal to", "(i.op.jge [a:target label]) = [s:op string]", "Transfers execution to the given label, if the equality register shows greater than or equal to.  (Labels are defined by i.op.label.)  (The commands must be searched sequentially, starting over from the beginning, so if you have multiple i.op.label commands with the same label, the first one found will always be jumped to.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.jle", "sea",			ActnParserJle, "Instrument op: Branch if less than or equal to", "(i.op.jle [a:target label]) = [s:op string]", "Transfers execution to the given label, if the equality register shows less than or equal to.  (Labels are defined by i.op.label.)  (The commands must be searched sequentially, starting over from the beginning, so if you have multiple i.op.label commands with the same label, the first one found will always be jumped to.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.load", "seaa",		ActnParserLoad, "Instrument op: Load value from ram", "(i.op.load [a:target var] [a:source pointer]) = [s:op string]", "Sets the target variable equal to the memory at the address given by the source variable.  In C, this is \"target = *source;\".  If an exception occurs, the target variable is set to zero.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.store", "seaa",		ActnParserStore, "Instrument op: Load value from ram", "(i.op.store [a:target pointer] [a:source var]) = [s:op string]", "Sets the memory at the address given by the target variable equal to the source variable.  In C, this is \"*target = source;\".  If an exception occurs, No action is taken.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.sets", "seas",		ActnParserSetString, "Instrument op: String literal", "(i.op.sets [a:target var] [s:string]) = [s:op string]", "Sets the variable to a pointer to the given string.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.mfglob", "seai",		ActnParserMfglob, "Instrument op: Move from global variable", "(i.op.mfglob [a:target var] [i:global index]) = [s:op string]", "The contents of the global variable at the given index are moved to the target variable.  Global in this context means that the value persists across multuple calls to the instrument and is visible from all threads that call into it.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.mtglob", "seia",		ActnParserMtglob, "Instrument op: Move to global variable", "(i.op.mtglob [i:global index] [a:target var]) = [s:op string]", "The contents of the given variable variable are moved to the given global index.  Global in this context means that the value persists across multuple calls to the instrument and is visible from all threads that call into it.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.numglobals", "sei",	ActnParserNumGlobals, "Instrument op: Set number of global variables", "(i.op.numglobals [i:num globals]) = [s:op string]", "Putting this operation in your sequence will force the system to allocate the given number of global variables, even if the other operations don't access them.  You only need to use this if you are using a routine in an external DLL and need to allocate extra slots for it; the system automatically detects the number of globals used by the i.op.* functions and allocates enough storage automatically.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.bufsize", "sei",		ActnParserBufferSize, "Instrument op: Set scratch buffer size", "(i.op.bufsize [i:buffer size]) = [s:op string]", "This function sets the size of the \"scratch\" buffer to the given value.  The scratch buffer is available for use as a way to construct string arguments to pass back to p5 (or whatever else you need it for.)  The default size is zero; when the size is zero, i.op.getbuf will return NULL.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.getbuf", "sea",		ActnParserGetbuf, "Instrument op: Get scratch buffer", "(i.op.getbuf [a:target var]) = [s:op string]", "Fills the target variable with the address of the scratch buffer.  The scratch buffer is allocated on the stack, and does not persist across action calls.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.strncat", "seaai",	ActnParserStrncat, "Instrument op: String concatenate (strcat)", "(i.op.strncat [a:target var] [a:source var] [i:max length]) = [s:op string]", "The max length parameter should specify the size (in bytes) of the target buffer.  This operation is the same as strcat (target, source), except that it checks that the target buffer has enough space before performing the operation.  (If it doesn't, the instrument fails).  Both the source and target variables must contain pointers to ansi strings.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.strncatw", "seaai",	ActnParserStrncat, "Instrument op: Unicode string concatenate (wcscat)", "(i.op.strncatw [a:target var] [a:source var] [i:max length]) = [s:op string]", "The max length parameter should specify the size (in bytes) of the target buffer.  This operation is the same as strcat (target, source), except that it checks that the target buffer has enough space before performing the operation.  (If it doesn't, the instrument fails).  Both the source and target variables must contain pointers to unicode strings.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.intncat", "seaii",	ActnParserIntncat, "Instrument op: Append integer to string", "(i.op.intncat [a:target var] [i:int to append] [i:max len]) = [s:op string]", "The max length parameter should be the size (in bytes) of the target buffer.  The given integer is converted into an ansi string and appended to the target.  The target variable must contain a pointer to an ansi string.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.atow", "seaia",		ActnParserAtow, "Instrument op: Convert ansi string to unicode string", "(i.op.atow [a:target var] [i:target length] [a:source var]) = [s:op string]", "The target var must contain a pointer to a buffer that receives the converted string.  The target length is the length (in bytes) of the target buffer.  The source var should point to an ansi string.  If an error occurs in the conversion, the instrument fails.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.wtoa", "seaia",		ActnParserWtoa, "Instrument op: Convert unicode string to ansi string", "(i.op.wtoa [a:target var] [i:target length] [a:source var]) = [s:op string]", "The target var must contain a pointer to a buffer that receives the converted string.  The target length is the length (in bytes) of the target buffer.  The source var should point to a unicode string.  If an error occurs in the conversion, the instrument fails.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.strstr", "seaaa",	ActnParserStrstr, "Instrument op: Find substring", "(i.op.strstr [a:target var] [a:string] [a:substring to search for]) = [s:op string]", "This function works just like the standard C strstr function.  The string and substring vars must contain pointers to strings.  The function sets the target var to a pointer to the location of the first occurence of the substring, or NULL if none exists.  If you only want to test whether the substring exists in the string, call this function and use cmpi on the result.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.strstrw", "seaaa",	ActnParserStrstr, "Instrument op: Find substring (unicode version)", "(i.op.strstrw [a:target var] [a:string] [a:substring to search for]) = [s:op string]", "This function works just like the standard C strstr function.  The string and substring vars must contain pointers to unicode strings.  The function sets the target var to a pointer to the location of the first occurence of the substring, or NULL if none exists.  If you only want to test whether the substring exists in the string, call this function and use cmpi on the result.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.prints", "sea",		ActnParserPrints, "Instrument op: Print string (to debug view)", "(i.op.prints [a:var to print]) = [s:op string]", "The variable must point to a string.  The string is printed using the OutputDebugString API.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.printsw", "sea",		ActnParserPrintsWide, "Instrument op: Print unicode string (to debug view)", "(i.op.printsw [a:var to print]) = [s:op string]", "The variable must point to a unicode string.  The string is printed using the OutputDebugStringW API.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.p5", "sea",			ActnParserP5Parse, "Instrument op: Parse in p5", "(i.op.p5 [a:string var]) = [s:op string]", "This operation sends the variable, which must contain a pointer to an ansi string, to the p5 mailslot on the current desktop.  A copy of p5 must be running, and it must have the default mailslot name.  Errors in the parse are ignored.  This operations calls the functions See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.msgbox", "seaaai",	ActnParserMsgbox, "Instrument op: Message box", "(i.op.msgbox [a:result var] [a:box text] [a:box title] [i:box type]) = [s:op string]", "This operation pops up a message box with the given parameters.  The title and text must be ansi strings.  The instrument (and thus the hooked function or message) is blocked until the user answers the box.  The results of the box are placed in the given target variable, which can then be used for decision making.  This operation may not always work, depending on what function or message has been hooked.  (For example, if you have hooked the MessageBoxA function, calling this will cause all manner of bad things to happen.)  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.msgboxw", "seaaai",	ActnParserMsgboxWide, "Instrument op: Message box (unicode)", "(i.op.msgboxw [a:result var] [a:box text] [a:box title] [i:box type]) = [s:op string]", "This operation pops up a message box with the given parameters.  The title and text must be unicode strings.  The instrument (and thus the hooked function or message) is blocked until the user answers the box.  The results of the box are placed in the given target variable, which can then be used for decision making.  This operation may not always work, depending on what function or message has been hooked.  (For example, if you have hooked the MessageBoxW function, calling this will cause all manner of bad things to happen.)  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.and", "seaa",		ActnParserAnd, "Instrument op: Bitwise and variable", "(i.op.and [a:var A] [a:var B]) = [s:op string]", "Sets variable A equal to (A & B).  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.or", "seaa",			ActnParserOr, "Instrument op: Bitwise or variable", "(i.op.or [a:var A] [a:var B]) = [s:op string]", "Sets variable A equal to (A | B).  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.not", "seaa",		ActnParserNot, "Instrument op: Bitwise not variable", "(i.op.not [a:var A]) = [s:op string]", "Sets variable A equal to (~A).  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.sle", "sea",			ActnParserSetLastErrorVar, "Instrument op: Set last error", "(i.op.sle [a:var]) = [s:op string]", "Calls SetLastError() with the value contained in the given variable as the parameter.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.sle", "sei",			ActnParserSetLastError, "Instrument op: Set last error", "(i.op.sle [i:literal value]) = [s:op string]", "Calls SetLastError() with the value given.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.getglob", "sea",		ActnParserGetGlobals, "Instrument op: Get global variable pointer", "(i.op.getglob [a:var]) = [s:op string]", "Fills the given variable with the address of the instrument's global variables.  This can be used, for example, to store a string in global space.  Make sure you reserve enough space with i.op.numglobals.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.upcase", "sea",		ActnParserUpcaseString, "Instrument op: Convert string to upper case", "(i.op.upcase [a:var]) = [s:op string]", "The variable must contain a pointer to a string.  The string is converted to upper case in place.  See (info \"instrument\") for some help in making sense of all this."},
	{"i.op.upcasew", "sea",		ActnParserUpcaseStringWide, "Instrument op: Convert unicode string to upper case", "(i.op.upcasew [a:var]) = [s:op string]", "The variable must contain a pointer to a unicode string.  The string is converted to upper case in place.  See (info \"instrument\") for some help in making sense of all this."},

	{NULL},
};




void PluginInit (parerror_t *pe)
{
	int st;
	int mademanager = 0;
	int mademessage = 0;

	fail_if (IsmCheckMode (MODE_AGENT), 0, PE ("Instrument.dll was loaded as an agent; cannot continue.", 0, 0, 0, 0));

	st = IsmSetMode (MODE_MANAGER);
	fail_if (!st, 0, PE ("Could not activate manager mode.", 0, 0, 0, 0));

	IsmSetPluginMode ();

	IsmManagerMutex = CreateMutex (NULL, FALSE, "Instrument-manager-mutex");
	fail_if (!IsmManagerMutex, 0, PE ("Could not create manager mutex, %i\n", GetLastError (), 0, 0, 0));
	mademanager = 1;

	IsmMessageMutex = CreateMutex (NULL, FALSE, "Instrument-msgbuf-mutex");
	fail_if (!IsmMessageMutex, 0, PE ("Could not create message mutex, %i\n", GetLastError (), 0, 0, 0));
	mademessage = 1;

	st = PlAddInfoBlock ("instrument", instance, IDR_INSTRUMENT);
	fail_if (!DERR_OK (st), st, PE ("Could not add info block \"instrument\", {%s,%i}", PlGetDerrString (st), GETDERRINFO (st), 0, 0));

	return;

failure:
	if (mademanager)
	{
		CloseHandle (IsmManagerMutex);
		IsmManagerMutex = NULL;
	}

	if (mademessage)
	{
		CloseHandle (IsmMessageMutex);
		IsmMessageMutex = NULL;
	}

}


void PluginDenit (parerror_t *pe)
{
	int st;
	fail_if (!IsmCheckMode (MODE_MANAGER), 0, PE ("Plugin is not in manager mode; cannot unload\n", 0, 0, 0, 0));

	PlRemoveInfoBlock ("instrument");

	CloseHandle (IsmManagerMutex);
	IsmManagerMutex = NULL;

	CloseHandle (IsmMessageMutex);
	IsmMessageMutex = NULL;

failure:
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




//DllMain, standard entry procedure.  If at all possible, NO work should
//be done by this routine, except for saving the instance handle.  Please
//do not do anything crazy like install Detours hooks in the p5 process!
//If you are using DllMain to install detours, do a check to make sure
//that the exe name of the process you're in isn't p5.exe, or something
//along those lines.
BOOL CALLBACK DllMain (HINSTANCE hInstance, DWORD reason, void *other)
{
	//int st;

	if (reason == DLL_PROCESS_ATTACH)
	{
		instance = hInstance;
/*
		IsmMessageMutex = CreateMutex (NULL, FALSE, "Instrument-msgbuf-mutex");
		fail_if (!IsmMessageMutex, 0, DP (ERROR, "Could not create/open message buffer mutex, %i\n", GetLastError ()));

		IsmManagerMutex = OpenMutex (MUTEX_ALL_ACCESS, FALSE, "Instrument-manager-mutex");
		if (IsmManagerMutex)
		{
			HANDLE thread;
			DWORD tid;

			CloseHandle (IsmManagerMutex);
			IsmManagerMutex = NULL; //agents don't need this.
			//also, if it isn't closed, then a manager will be unable to start up unless ALL
			//agents are dead; this is not the desired behavior.

			//a manager is running, that means we go into agent mode
			IsmSetMode (MODE_AGENT);

			thread = CreateThread (NULL, 0, AgnInstrumentationListener, NULL, 0, &tid);
			fail_if (!thread, 0, DP (ERROR, "Could not create agent listener thread, %i\n", GetLastError ()));
			CloseHandle (thread);
		}
*/
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		//each of these should technically imply the other
		if (!IsmCheckIsPlugin () || IsmCheckMode (MODE_AGENT))
		{
			//not running as a manager (p5 plugin), refuse to unload
			IsmDprintf (WARN, "Warning: Refusing to unload instrument.dll from process %i", GetCurrentProcessId ());
			return FALSE;
		}
/*
		//close the mutexes which we allocated in DLlMain
		if (IsmMessageMutex)
		{
			CloseHandle (IsmMessageMutex);
			IsmMessageMutex = NULL;
		}

		//we know this has already been freed because we are in manager mode.
		//(in manager (plugin) mode, this is handled by the p5 plugin init/denit routines
		//if (IsmManagerMutex)
		//{
		//	CloseHandle (IsmManagerMutex);
		//	IsmManagerMutex = NULL;
		//}
*/
	}

	
	return TRUE;
/*
failure:
	if (IsmMessageMutex)
		CloseHandle (IsmMessageMutex);

	if (IsmManagerMutex)
		CloseHandle (IsmManagerMutex);

	return FALSE;
*/
}



//some kind of child infection ability.