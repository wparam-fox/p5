/* Copyright 2006 wParam, licensed under the GPL */


#include <windows.h>
#include "wp_mainwin.h"
//#include "..\common\wp_dyn.h"

typedef struct messagelist_t
{
	message_t data;

	struct messagelist_t *next;
} messagelist_t;


typedef struct
{
	messagelist_t *head;

	CRITICAL_SECTION cs;
} mw_t;


#ifdef MwMalloc
void * MwMalloc (int);
#else
#define MwMalloc malloc
#endif

#ifdef MwFree
void MwFree (void *);
#else
#define MwFree free
#endif



LRESULT CALLBACK MwWindowProc (HMAINWND, UINT, WPARAM, LPARAM);
int MwInit (HINSTANCE instance)
{
	WNDCLASS wc = {0};


	wc.hInstance = instance;
	wc.lpfnWndProc = MwWindowProc;
	wc.lpszClassName = MW_CLASS;
	
	if (!RegisterClass (&wc))
		return 0;

	return 1;
}

void MwDenit (HINSTANCE instance)
{
	UnregisterClass (MW_CLASS, instance);
}

static LRESULT MwCallMessage (message_t *m, HMAINWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT retur;
	DWORD extr = m->extra;
	FARPROC foo = m->callback;


	if (m->flags & FLAG_EXTRA)
		__asm push extr;
	if (m->flags & FLAG_LPARAM)
		__asm push lParam;
	if (m->flags & FLAG_WPARAM)
		__asm push wParam;
	if (m->flags & FLAG_MESSAGE)
		__asm push message;
	if (m->flags & FLAG_HWND)
		__asm push hwnd;
	
	
	__asm 
	{
		call foo
		mov retur, eax;		//get return value
	}

	return retur;
}


static LRESULT CALLBACK MwWindowProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT o;
	mw_t *mw = (mw_t *)GetWindowLong (hwnd, GWL_USERDATA);
	messagelist_t *walk;

	if (!mw)
		return DefWindowProc (hwnd, message, wParam, lParam); //happens on WM_CREATE, etc.

	EnterCriticalSection (&mw->cs);

	walk = mw->head;
	while (walk)
	{
		if (walk->data.message == message)
		{
			if ((wParam & walk->data.wMask) == walk->data.wParam &&
				(lParam & walk->data.lMask) == walk->data.lParam)
			{
				o = MwCallMessage (&walk->data, hwnd, message, wParam, lParam);
				LeaveCriticalSection (&mw->cs);

				return o;
			}
		}

		walk = walk->next;
	}

	LeaveCriticalSection (&mw->cs);

	return DefWindowProc (hwnd, message, wParam, lParam);
}

static int MwSaneInitCriticalSection (CRITICAL_SECTION *cs)
{
	int ret = 1;
	__try
	{
		InitializeCriticalSection (cs);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		ret = 0;
	}
	return ret;
}

HMAINWND MwCreate (HINSTANCE instance, char *title)
{
	HMAINWND out;
	mw_t *mw;

	mw = MwMalloc (sizeof (mw_t));
	if (!mw)
		return NULL;

	mw->head = NULL;

	if (!MwSaneInitCriticalSection (&mw->cs))
	{
		MwFree (mw);
		return NULL;
	}

	out = CreateWindow (MW_CLASS, title, 0, 0, 0, 0, 0, NULL, NULL, instance, NULL);
	if (!out)
	{
		DeleteCriticalSection (&mw->cs);
		MwFree (mw);
		return NULL;
	}

	SetWindowLong (out, GWL_USERDATA, (LONG)mw);

	return out;
}

void MwDestroy (HMAINWND hwnd)
{
	mw_t *mw = (mw_t *)GetWindowLong (hwnd, GWL_USERDATA);
	messagelist_t *walk, *temp;

	if (!mw)
		return;

	DestroyWindow (hwnd);

	DeleteCriticalSection (&mw->cs);

	walk = mw->head;
	while (walk)
	{
		temp = walk->next;
		MwFree (walk);
		walk = temp;
	}

	MwFree (mw);
}

int MwAddMessageIndirect (HMAINWND hwnd, message_t *mes)
{
	messagelist_t *n;
	mw_t *mw = (mw_t *)GetWindowLong (hwnd, GWL_USERDATA);

	if (!mw)
		return 0;
	
	n = MwMalloc (sizeof (messagelist_t));
	if (!n)
		return 0;

	memcpy (&n->data, mes, sizeof (message_t));

	EnterCriticalSection (&mw->cs);

	n->next = mw->head;
	mw->head = n;

	LeaveCriticalSection (&mw->cs);

	return 1;
}


int MwDeleteMessageIndirect (HMAINWND hwnd, message_t *mes)
{
	mw_t *mw = (mw_t *)GetWindowLong (hwnd, GWL_USERDATA);
	messagelist_t **walk, *temp;
	int res;
	
	if (!mw)
		return 0;

	EnterCriticalSection (&mw->cs);

	walk = &mw->head;
	while (*walk)
	{
		if (memcmp (mes, &(*walk)->data, sizeof (message_t)) == 0)
			break;

		walk = &(*walk)->next;
	}

	if (*walk)
	{
		//unlink and delete
		temp = *walk;
		*walk = (*walk)->next;
		MwFree (temp);

		res = 1;
	}
	else
	{
		res = 0;
	}

	LeaveCriticalSection (&mw->cs);

	return res;
}



int MwAddMessageFullCall (HMAINWND hwnd, UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask, FARPROC callback, int flags, DWORD extra)
{
	message_t m = {message, wParam, wMask, lParam, lMask, callback, flags, extra};

	return MwAddMessageIndirect (hwnd, &m);
}

int MwAddMessageBasic (HMAINWND hwnd, UINT message, FARPROC call, DWORD extra) //FLAG_EXTRA assumed
{
	message_t m = {message, 0, 0, 0, 0, call, FLAG_EXTRA, extra}; //accepts all w/l Param

	return MwAddMessageIndirect (hwnd, &m);
}

int MwAddMessageSimple (HMAINWND hwnd, UINT message, FARPROC call, int flags, DWORD extra)
{
	message_t m = {message, 0, 0, 0, 0, call, flags, extra};

	return MwAddMessageIndirect (hwnd, &m);
}

int MwDeleteMessageFullCall (HMAINWND hwnd, UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask, FARPROC callback, int flags, DWORD extra)
{
	message_t m = {message, wParam, wMask, lParam, lMask, callback, flags, extra};

	return MwDeleteMessageIndirect (hwnd, &m);
}

int MwDeleteMessageBasic (HMAINWND hwnd, UINT message, FARPROC call, DWORD extra) //FLAG_EXTRA assumed
{
	message_t m = {message, 0, 0, 0, 0, call, FLAG_EXTRA, extra}; //accepts all w/l Param

	return MwDeleteMessageIndirect (hwnd, &m);
}

int MwDeleteMessageSimple (HMAINWND hwnd, UINT message, FARPROC call, int flags, DWORD extra)
{
	message_t m = {message, 0, 0, 0, 0, call, flags, extra};

	return MwDeleteMessageIndirect (hwnd, &m);
}

/*
LRESULT CALLBACK foo1 (HMAINWND hwnd, DWORD extra)
{
	return extra - (int)hwnd;
}

int main (void)
{
	int x;
	message_t m = {0, 0, 0, 0, 0, (FARPROC)foo1, FLAG_HMAINWND | FLAG_EXTRA, 14};

	x = MW_CallMessage (&m, 6, 0, 0, 0);

	return 1;
}
*/