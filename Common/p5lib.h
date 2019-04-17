/* Copyright 2006 wParam, licensed under the GPL */

#ifndef _P5LIB_H_
#define _P5LIB_H_






//Get the string name that applies to a given desktop.
//The desktop is either a handle to a desktop (passed as
//the desktop parameter) or a string name of a desktop
//(passed as the deskname parameter).
//
//If nameout is NULL, the required buffer size is placed
//in *namelen and the function returns with no further
//processing.  If nameout is not null, *namelen should
//contain the length, in bytes, of the nameout buffer.
//In this case, *namelen is not updated with the length
//returned.
//
//Returns 0 on success, nonzero on failure
int PfGetDefaultPipeName (void *desktop, char *deskname, char *nameout, int *namelen);


//Send a command to a p5 process using the mailslot.
//name - mailslot name to use.  PfGetDefaultPipeName
//       is a good place to get this name.
//line - the command to parse.
//
//returns 0 on success, nonzero on failure
int PfSendMailCommand (char *name, char *line);



















#endif
