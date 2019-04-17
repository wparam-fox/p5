/* Copyright 2006 wParam, licensed under the GPL */

#ifndef _WP_MAINWIN_H_
#define _WP_MAINWIN_H_

#define MW_CLASS "MW_Class"


//these flags define what paramaters the callback function needs
//the paramaters come in the same order as a standard window procedure
#define FLAG_HWND 1
#define FLAG_MESSAGE 2
#define FLAG_WPARAM 4
#define FLAG_LPARAM 8
#define FLAG_EXTRA 16
typedef struct message_t
{
	UINT message;
	WPARAM wParam;		//wParam value to match
	WPARAM wMask;		//wParam of the message will be anded with this before testing against this->wParam.  (procedure receives original wParam)
	LPARAM lParam;
	LPARAM lMask;
	FARPROC callback; //i.e. must be __stdcall
	int flags;
	DWORD extra;
} message_t;

typedef HWND HMAINWND;



int MwInit (HINSTANCE instance);
void MwDenit (HINSTANCE instance);

HMAINWND MwCreate (HINSTANCE instance, char *title);
void MwDestroy (HMAINWND hwnd);

int MwAddMessageSimple (HMAINWND hwnd, UINT message, FARPROC call, int flags, DWORD extra);
int MwAddMessageBasic (HMAINWND hwnd, UINT message, FARPROC call, DWORD extra); //FLAG_EXTRA assumed
int MwAddMessageFullCall (HMAINWND hwnd, UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask, FARPROC callback, int flags, DWORD extra);
int MwAddMessageIndirect (HMAINWND hwnd, message_t *mes);


int MwDeleteMessageIndirect (HMAINWND hwnd, message_t *mes);
int MwDeleteMessageFullCall (HMAINWND hwnd, UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask, FARPROC callback, int flags, DWORD extra);
int MwDeleteMessageBasic (HMAINWND hwnd, UINT message, FARPROC call, DWORD extra); //FLAG_EXTRA assumed
int MwDeleteMessageSimple (HMAINWND hwnd, UINT message, FARPROC call, int flags, DWORD extra);










#endif