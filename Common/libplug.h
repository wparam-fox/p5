/* Copyright 2008 wParam, licensed under the GPL */

#ifndef _LIBPLUG_H_
#define _LIBPLUG_H_



#include "..\common\plugin.h"



//PlParserMalloc is ONLY to be used for returning things to the parser,
//i.e. string returns, or allocations placed in parerror_t structures.
//(Make sure you set the PEF_* flag appropriate to the element in
//parerror that you allocated something for.)  All other memory allocation
//should come from PlMalloc.
void *ParMalloc (int size);

//There will probably be very few cases where you need to use this
//function, it is provided mostly for completeness.  Obviously only
//things you got from PlParserMalloc should go to this function.
void ParFree (void *block);

//There may be a case where something you call returns a DERR error code,
//use this to translate it to a string.  This function always returns
//a pointer to a statically allocated string, so you don't need to check
//it for NULL, and you shouldn't try to free it.
char *PlGetDerrString (DERR in);

//Simple printf function, prints to the p5 console, if one is open.
//Note: calling the actual printf function directly is NOT LIKELY TO
//WORK.  Use this instead.  It may be routed to OutputDebugString if
//the console is disabled at some point in the future.
DERR PlPrintf (char *in, ...);



//Basic allocation functions for the private plugin heap.  All memory
//allocated by these functions is destroyed when the plugin is unloaded,
//so be careful when you pass them around.
void * __stdcall PlMalloc (int size);
void * __stdcall PlRealloc (void *block, int size);
void * __stdcall PlFree (void *block);

//returns 1 if the current thread is the 'main' thread.  Functions that
//create windows should check this and fail if they are not in the main
//thread.  Functions that will block execution should check for this and
//fail if they ARE in the main thread.
int PlIsMainThread (void);


//Get a handle to a new parser.  The handle must be deleted with PlDestroyParser
//when it is no longer needed.
DERR PlCreateParser (parser_t **par);

//Destroys a handle returned by PlCreateParser.
DERR PlDestroyParser (parser_t *par);

//reset the parser to default state
DERR PlResetParser (parser_t *par);

//Add an atom to a parser you got from PlCreateParser
DERR PlParserAddAtom (parser_t *par, char *atom, char *value, int flags);

//Remove an atom added by PlParserAddAtom
DERR PlParserRemoveAtom (parser_t *par, char *name);

//The run parser function.  All parameters are required and should be provided.
//This function returns success for both success and "more data required".
//The way you tell the difference is through nesting--if nesting is nonzero, then
//the parse was not a complete s-expression and more data is required.  If you
//are not prepared for more data to come in (i.e. you were expecting this to be
//a complete expression) then you need to check for nesting != 0 and report it
//as an error.  (You probably want to call PlResetParser also).  Output is always
//set to NULL if an error is returned, however, it is recommended that you always
//test it to make sure.  If output is set to something not null, it MUST ALWAYS
//be passed to ParFree when you are done with it.  
DERR PlRunParser (parser_t *par, char *line, char **output, int *nesting, char *error, int errlen);

//Run something in the main parser.  Neither output nor pe should be null.
//Use this if you need to run something, but don't have any reason to get
//a full parser.  Something like a hotkey or an alarm would be a good
//candidate for using this function.  If you aren't being called from
//the parser (and thus don't have a parerror_t to use), create one on the
//stack, initialize it to all zero, and then pass it in.  When this 
//function returns, either call PlCleanupError if you don't care, or call
//PlProcessError to format it to a text string.  Don't forget to ParFree
//output if it is not null on return.
DERR PlMainThreadParse (char *line, char **output, parerror_t *pe);

//Cleanup an error type.  This frees any memory that's been allocated, and
//memsets the error back to 0.
void PlCleanupError (parerror_t *pe);

//Process a parerror_t into the string it represents.  This function also
//performs the duties of PlCleanupError, so keep in mind that pe will be
//freed and 0'd out.  The status value should be the value returned from
//a parse function, such as PlMainThreadParse or PlRunParser
void PlProcessError (int st, char *error, int errlen, parerror_t *pe);

//Gets the handle to the p5 main window.  You will need to use this if you're
//using PlRegisterMessage to register callbacks on the main window.
HWND PlGetMainWindow (void);

//Add a callback message.  The given function will be called when the main window
//gets a message matching the given criteria.  The test that is applied looks like
//this, where 'wParam' is the wParam received by the main window procedure, and 'wp'
//and 'wm' are the wParam and wMask given here, respectively:
//
// if ((wParam & wm) == wp)
//		call_callback ();
//
//The same test is applied to lParam.  The procedure will receive the same values
//the main window procedure gets.  So, to catch all messages regardless of
//wParam or lParam, you would set both wParam and wMask to 0 in the call.
int PlAddMessageCallback (UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask, WNDPROC proc);

//Remove a callback message.  This MUST BE DONE before a plugin is unloaded.  If
//this function returns 0, you DEFINITELY SHOULD FAIL your denit and kill
//routines.  It is not safe to unload the dll if there is still a pointer to
//one of its functions out there.  Note that EVERY PARAMETER MUST MATCH EXACTLY
//with the parameters you called PlAddMessageCallback with, or it won't work.
int PlRemoveMessageCallback (UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask, WNDPROC proc);


//A handy helper function designed to split a string into its component parts.
//It will turn a string like "foo1;foo2;foo3" into an array of strings. 
//There are two steps to using it, getting the size, then getting the data.
//Use it like this:
//
//To get the size required, call the function like this:
//  PlSplitString (foostring, &size, NULL, ';');
//Then allocate a buffer of at least size bytes, and call the function like this:
//  PlSplitString (foostring, &size, result, ';');
//Note that in this case size is the number of parameters, NOT the length in bytes.
DERR PlSplitString (char *string, int *outcount, char **outarray, char delim);


//Adds an info block to the p5 info system.  The info block is actually a resource
//of type "INFO" that you add to your .dll file.  To add the block, you give it a
//name, the HINSTANCE of your dll (save this when you get it in DllMain), and the
//id of the resource (IDR_RUNBOXINFO, or whatever you defined it as in the resource
//editor).  The resource itself should be just a text file.  (\r\n line endings is
//key; this text will be stuck in message and edit boxes, neither of which will be
//happy if you give them UNIX line endings.
DERR PlAddInfoBlock (char *name, HINSTANCE module, int resid);

//Remove an info block previously added.  This function is void because it is not
//critical if an info block is not properly removed.  It won't cause a crash, at
//any rate, and I don't think it's worth making the plugins have to deal with this
//function returning failure in their denit routines.  The function can't really
//fail, anyway, unless the name you give is invalid.
void PlRemoveInfoBlock (char *name);

//Run a command in old pixshell.  This looks for a copy of old pixshell running
//on the current desktop and passes the given string to its parser.  If async is
//nonzero, PostMessage is used, otherwise, SendMessage is used.
DERR PlPixshellThreeRun (char *command, int async);

//Execute a program, using ShellExecute.  This function splits out the args and
//directory and runs the process.  It can take things like URLs in addition to
//exe files.
DERR PlProperShell (char *line);

//Convert an int64 into a TYPE suitable for returning to the script world.  The
//buffer for this function comes from ParMalloc.  If this function returns NULL,
//the error object has already been set to something useful; callers of this
//function should simply throw it as is in this case.
char *PlLongIntToType (parerror_t *pe, __int64 in);

//Convert a TYPE string to a long int.  No parameters may be NULL.  The type of
//the input must be "i64", the same type returned from PlLongIntToType.  If the
//function returns 0, the error has already been set, simply throw it as is back
//at the script parser.
int PlTypeToLongInt (parerror_t *pe, char *in, __int64 *output);

//Convert arbitrary binary data into a TYPE suitable to return to script land.
//No parameters may be NULL.  The result comes from ParMalloc; be sure to use
//ParFree to free it if you need to abort later.  On NULL return, the error
//object is already prepared to throw back at the script parser.  The typename
//should only contain alphanumerics.
char *PlBinaryToType (parerror_t *pe, void *voidin, int inlen, char *typename);

//Convert a standard TYPE back to its binary data.  The data parameter (voiddata)
//may be NULL, if this is the case, the function checks to see what the length of
//the TYPE passed in would be if converted to binary and returns this information
//in lenout.  If the TYPE is invalid, either because of a type string mismatch or
//length error, an error will be returned.  (So be sure to check the return value
//both when getting the length and when getting the data.)  Callers may allocate
//storage for the output data whereever it is convenient.  Lenout may not be NULL
//in any case.
int PlTypeToBinary (parerror_t *pe, char *type, char *typeexpected, int *lenout, void *voiddata, int datalen);

//Add a "watcher" to the system.  Use this function when you need to return e.g.,
//an HICON variable to script space.  Right before the value is returned, use
//this function to add a watcher for it, such as PlAddWatcher ("HICON", var),
//so that if something goes wrong there will be a record of the leaked handle.
DERR __stdcall PlAddWatcher (char *type, void *value);

//Removes a watcher previously added.  This should be called immediately when
//a watched type is read in from script land.  If you call this function and
//remove a watcher, you MUST delete the value as appropriate before returning
//from your function, whether successfully or with error.  The idea is that
//watchers exist ONLY for the space of time a handle is being stored in text
//form in the mechanics of the parser.
DERR PlDeleteWatcher (char *type, void *value);

//Checks to see if a watcher exists.  The existance of a watcher for a value
//should ALWAYS be checked before accepting such a value from script land.
//The delete function will fail if the watcher doesn't exist, either that or
//this function should be checked, if a watcher wasn't there, assume the
//handle value is invalid.  If this function returns 1, it should be safe to
//assume that PlDeleteWatcher will also succeed.
int PlCheckWatcher (char *type, void *value);



//Some standard C library functions.  The goal being that we DON'T have
//to link a c runtime into most of our plugins
void *PlMemset (void *dest, int c, size_t count);
#define PlAlloca _alloca
size_t PlStrlen (const char *string);
char *PlStrcat (char *strDestination, const char *strSource);
char *PlStrcpy (char *strDestination, const char *strSource);
int PlAtoi(char *);
int PlSnprintf (char *buffer, size_t count, const char *format, ...);
int PlStrcmp (const char *string1, const char *string2);
char *PlStrstr (const char *string, const char *strCharSet);
int PlStrncmp (const char *string1, const char *string2, size_t count);
void *PlMemcpy (void *dest, const void *src, size_t count);
double PlFmod(double, double);
double PlSqrt(double);
double PlSin(double);
double PlCos(double);
double PlTan(double);
double PlAsin(double);
double PlAcos(double);
double PlAtan(double);
double PlCeil(double);
double PlFloor(double);
double PlLog(double);
double PlLog10(double);
double PlPow(double, double);
#endif