/* Copyright 2006 wParam, licensed under the GPL */

#include <windows.h>
#include "instrument.h"

typedef struct
{
	HWND hwnd;
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
	WNDPROC wp;
} msgsearch;

static int WhFindMatchCompare (instlist_t *item, msgsearch *data)
{
	instwindow_t *iw;
	if (item->inst->type != INSTRUMENT_WINDOW)
		return 0;

	iw = (instwindow_t *)(item->inst->data);

	if (iw->hwnd != data->hwnd)
		return 0;

	if (data->wp && data->wp != item->runtime[0])
		IsmDprintf (ERROR, "BAD: Multiple window procedures for one window!\n");
	data->wp = item->runtime[0];

	if (iw->message != data->message)
		return 0;

	if ((data->wParam & iw->wMask) != iw->wParam)
		return 0;

	if ((data->lParam & iw->lMask) != iw->lParam)
		return 0;

	return 1;
}

void WhFindMatch (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, instlist_t **match, WNDPROC *proc)
{
	msgsearch ms = {hwnd, message, wParam, lParam};
	instlist_t *out;

	out = InSearchInstruments (WhFindMatchCompare, &ms);

	if (proc)
		*proc = ms.wp;
	*match = out;
}


typedef struct
{
	HWND hwnd;
	WNDPROC wp;
	int count;
} iwhdata;

static int WhIsWindowHookedCompare (instlist_t *item, iwhdata *data)
{
	instwindow_t *iw;
	
	if (item->inst->type != INSTRUMENT_WINDOW)
		return 0;

	iw = (instwindow_t *)(item->inst->data);

	if (iw->hwnd == data->hwnd)
	{
		data->wp = item->runtime[0];
		data->count++;
	}

	return 0;
}

WNDPROC WhIsWindowHooked (HWND hwnd, int *count)
{
	instlist_t *out;
	iwhdata wh = {hwnd, NULL, 0};

	out = InSearchInstruments (WhIsWindowHookedCompare, &wh);

	if (out)
		IsmDprintf (ERROR, "Consistency failure 0x2834\n");

	if ((wh.count && !wh.wp) || (!wh.count && wh.wp))
		IsmDprintf (ERROR, "Consistency failure 0x2835\n");

	if (count)
		*count = wh.count;

	return wh.wp;
}


typedef struct
{
	HWND hwnd;
	UINT message;
	WPARAM wParam;
	LPARAM lParam;

	WNDPROC wp;
} proccall_t;

DWORD WhWindowProcCallback (proccall_t *pc)
{
	return CallWindowProc (pc->wp, pc->hwnd, pc->message, pc->wParam, pc->lParam);
}

LRESULT CALLBACK WhWindowProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	WNDPROC proc;
	instlist_t *match;
	//DWORD res;
	proccall_t pc;
	


	WhFindMatch (hwnd, message, wParam, lParam, &match, &proc);

	if (match && proc)
	{
		//technically we could just use pc for the params list, but let's randomly
		//use good code practice for once
		void *params[] = {(void *)hwnd, (void *)message, (void *)wParam, (void *)lParam};

		pc.hwnd = hwnd;
		pc.message = message;
		pc.wParam = wParam;
		pc.lParam = lParam;
		pc.wp = proc;
		//dprintf (NOISE, "Match, doin the shite\n");

		return ActnExecute (match->actions, WhWindowProcCallback, &pc, params, 4);
	}

	if (!proc)
	{
		IsmDprintf (WARN, "Warning: No entry in instrument list for 0h%X\n", hwnd);
		return DefWindowProc (hwnd, message, wParam, lParam);
	}

	return CallWindowProc (proc, hwnd, message, wParam, lParam);
}




int WhSetWindowHook (instlist_t *inst)
{
	WNDPROC wp;
	instwindow_t *win;
	DWORD pid;
	int st;

	//some sanity checks first
	fail_if (!IsmCheckMode (MODE_AGENT), 0, DP (ERROR, "Instruments can only be set by an agent\n"));
	fail_if (inst->inst->type != INSTRUMENT_WINDOW, 0, DP (ERROR, "Instrument is wrong type\n"));

	win = (instwindow_t *)inst->inst->data;

	GetWindowThreadProcessId (win->hwnd, &pid);
	fail_if (pid != GetCurrentProcessId (), 0, DP (ERROR, "Window %X is not from this process (%i should be %i)\n", win->hwnd, pid, GetCurrentProcessId ()));

	wp = WhIsWindowHooked (win->hwnd, NULL);

	if (wp)
	{
		IsmDprintf (NOISE, "Success, hook already installed\n");

		//The window is already hooked.  this is the easy case, just mark
		//this item list entry's runtime with the window procedure and
		//that's it.  The WhFindMatch function will find this new entry
		//as needed.
		inst->runtime[0] = wp;
		return 1;
	}

	SetLastError (0);
	wp = (WNDPROC)GetWindowLong (win->hwnd, GWL_WNDPROC);
	fail_if (!wp, 0, DP (ERROR, "GetWindowLong returned a NULL window proc (%i)\n", GetLastError ()));

	inst->runtime[0] = wp;

	SetLastError (0);
	st = SetWindowLong (win->hwnd, GWL_WNDPROC, (LONG)WhWindowProc);
	fail_if (!st, 0, DP (ERROR, "SetWindowLong failed to hook (%i)\n", GetLastError ()));

	IsmDprintf (NOISE, "Successfully hoooked a window\n");

	return 1;
failure:
	return 0;
}

int WhRemoveWindowHook (instlist_t *inst)
{
	WNDPROC wp;
	instwindow_t *win;
	int count;
	int st;

	//some sanity checks first
	fail_if (!IsmCheckMode (MODE_AGENT), 0, DP (ERROR, "Instruments can only be unset by an agent\n"));
	fail_if (inst->inst->type != INSTRUMENT_WINDOW, 0, DP (ERROR, "Instrument is wrong type\n"));

	win = (instwindow_t *)inst->inst->data;

	wp = WhIsWindowHooked (win->hwnd, &count);

	if (count == 0)
	{
		IsmDprintf (NOISE, "Unhooking\n");

		//only one left in this window is the one we're killing, unhook
		SetLastError (0);
		wp = (WNDPROC)SetWindowLong (win->hwnd, GWL_WNDPROC, (LONG)inst->runtime[0]);
		fail_if (!wp && GetLastError () != ERROR_SUCCESS, 0, DP (ERROR, "SetWindowLong failed to unhook (%i)\n", GetLastError ()));
	}

	IsmDprintf (NOISE, "Successfully unhooked a window\n");

	return 1;

failure:
	return 0;
}



instimpl_t WhWindowImplementation = 
{
	"window",
	1,
	WhSetWindowHook,
	WhRemoveWindowHook,
	NULL,//ParseWindowHook,
};