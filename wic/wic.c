/* Copyright 2007 wParam, licensed under the GPL */

#include <windows.h>
#include "wic.h"

int WicDebugPrints = 0;

typedef struct wic_s
{
	HWND hwnd;

	struct
	{
		HDC hdc;
		HBITMAP bit;
		HBITMAP oldbit;
	} dc;

	int created;

	clock_t *clocks;


} wic_t;

#define WM_ADDCLOCK (WM_USER + 148)

//an array that holds the HWNDS.  We need to be able to destroy them all
//in case of unload.  Note that not all are guaranteed to stay valid; if
//a clock is closed it isn't necessarily removed from here.
HWND *WicWindows = NULL;
int WicNumWindows = 0;

unsigned int WicSentinel = 0;


//more fun with naked assembly.  I want to be able to enable or disable
//this function, but it's vararg, so we're playing games with stuff.
__declspec (naked) DERR WicDprintf (char *fmt, ...)
{
	if (WicDebugPrints)
	{
		__asm {jmp PlPrintf};
	}
	else
	{
		__asm 
		{
			mov eax, 0
			ret
		}
	}
}








int WicSetDebugPrint (int count, int newval)
{
	int old;

	if (count == 1) //function only got 1 parameter, don't access newcount
		return WicDebugPrints;

	old = WicDebugPrints;
	WicDebugPrints = newval;

	return old;
}




void WicFreeClocks (wic_t *wic)
{
	clock_t *temp, *walk;

	walk = wic->clocks;
	while (walk)
	{
		if (walk->destroy)
			walk->destroy (walk);
		temp = walk->next;
		PlFree (walk);
		walk = temp;
	}
}

void WicFreeDc (wic_t *wic)
{

	SelectObject (wic->dc.hdc, wic->dc.oldbit); //replace old bitmap
	DeleteObject (wic->dc.bit); //kill new bitmap
	DeleteDC (wic->dc.hdc); //kill DC (and old bitmap)

	wic->dc.bit = NULL;
	wic->dc.hdc = NULL;
	wic->dc.oldbit = NULL;
}

void WicDestroyWic (wic_t *wic)
{
	if (wic->dc.hdc)
		WicFreeDc (wic);

	WicFreeClocks (wic);

	PlFree (wic);
}

int WicUpdateMemdc (wic_t *wic)
{
	DERR st;
	RECT r;
	HDC hdc = NULL, memdc = NULL;
	HBITMAP oldbit = NULL;
	HBITMAP bit = NULL;

	if (wic->dc.hdc)
		WicFreeDc (wic);

	st = GetClientRect (wic->hwnd, &r);
	fail_if (!st, 0, DP ("GetClientRect failed, %i\n", GetLastError ()));

	hdc = GetDC (NULL);
	fail_if (!hdc, 0, DP ("GetDC (NULL) failed, %i\n", GetLastError ()));

	memdc = CreateCompatibleDC (hdc);
	fail_if (!memdc, 0, DP ("CreateCompatibleDC failed, %i\n", GetLastError ()));

	bit = CreateCompatibleBitmap (hdc, r.right, r.bottom);
	fail_if (!bit, 0, DP ("CreateCompatibleBitmap failed, %i\n", GetLastError ()));

	ReleaseDC (NULL, hdc);
	hdc = NULL;

	oldbit = SelectObject (memdc, bit);
	fail_if (!oldbit, 0, DP ("SelectObject failed, somehow\n"));

	//black it all out
	BitBlt (memdc, 0, 0, r.right, r.bottom, NULL, 0, 0, BLACKNESS);
	//not worrying if this fails

	//ok, we were successful.
	wic->dc.hdc = memdc;
	wic->dc.bit = bit;
	wic->dc.oldbit = oldbit;

	return 1;

failure:
	if (hdc)
		ReleaseDC (NULL, hdc);

	//if oldbit is non null (can't actually happen, currently) then we know bit is
	//in the memdc, and need to swap it out before bit or memdc is destroyed
	//(oldbit is destroyed by DleleteDC)
	if (oldbit)
		SelectObject (memdc, oldbit);

	if (bit)
		DeleteObject (bit);

	if (memdc)
		DeleteDC (memdc); //also kills oldbit if appropriate

	return 0;
}

int WicPaint (wic_t *wic)
{
	PAINTSTRUCT ps;
	DERR st;
	HDC hdc = NULL;
	RECT r;

	hdc = BeginPaint (wic->hwnd, &ps);
	fail_if (!hdc, 0, DP ("BeginPaint failed, %i\n", GetLastError ()));

	st = GetClientRect (wic->hwnd, &r);
	fail_if (!st, 0, DP ("GetClientRect failed, %i\n", GetLastError ()));

	st = BitBlt (hdc, 0, 0, r.right, r.bottom, wic->dc.hdc, 0, 0, SRCCOPY);
	fail_if (!st, 0, DP ("BitBlt failed, %i\n", GetLastError ()));

	EndPaint (wic->hwnd, &ps);

	return 1;
failure:
	if (hdc)
		EndPaint (wic->hwnd, &ps);

	return 0;
}


//called as a result of a timer message.
int WicRunClocks (wic_t *wic, int forceupdate)
{
	DERR st;
	__int64 curtime;
	__int64 mintime, timedelta;
	SYSTEMTIME systime;
	FILETIME *ft;
	clock_t *walk;
	int painted;

	//kill the old timer.  This applies whether we're triggered by a timer event or
	//a new clock addition.
	st = KillTimer (wic->hwnd, 42);
	//fail_if (!st, 0, DP ("KillTimer failed, %i\n", GetLastError ()));

	ft = (FILETIME *)&curtime;

	//get the current time
	GetSystemTime (&systime);
	st = SystemTimeToFileTime (&systime, ft);
	fail_if (!st, 0, DP ("SystemTimeToFileTime failed, %i\n", GetLastError ()));

	//run any clocks that have expired
	walk = wic->clocks;
	painted = 0;
	while (walk)
	{
		if (walk->nextupdate < curtime || forceupdate)
		{
			if (walk->paint) //not sure why it wouldn't, but...
				walk->paint (wic->dc.hdc, walk);
			painted = 1;
		}

		walk = walk->next;
	}

	//figure out the new nextupdate time.
	mintime = 0x7FFFFFFFFFFFF;
	walk = wic->clocks;
	while (walk)
	{
		if (walk->nextupdate < mintime)
			mintime = walk->nextupdate;

		walk = walk->next;
	}

	if (mintime < curtime)
		WicDprintf ("WARNING: Retarded clock detected\n");

	timedelta = mintime - curtime;

	//timedelta has units of 100 nanosecond intervals.
	timedelta = timedelta / 10; //now microseconds
	timedelta = timedelta / 1000; //now milliseconds.

	if (timedelta < 0)
		timedelta = 0;

	//add 10 milliseconds to the result, to help ensure that we definitely
	//get the timer message AFTER the perscribed time.
	timedelta += 10;

	st = SetTimer (wic->hwnd, 42, (UINT)timedelta, NULL);
	fail_if (!st, 0, DP ("SetTimer failed, %i\n", GetLastError ()));

	if (painted)
		InvalidateRect (wic->hwnd, NULL, FALSE);

	return 1;

failure:
	return 0;
}



int WicAddClockToList (wic_t *wic, clock_t *clock)
{
	clock_t **walk;

	walk = &wic->clocks;
	while (*walk)
	{
		walk = &(*walk)->next;
	}
	*walk = clock;

	return 1;
}



LRESULT CALLBACK WicProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DERR st;
	wic_t *wic;

	switch (message)
	{
	case WM_CREATE:
		wic = ((CREATESTRUCT *)lParam)->lpCreateParams;
		fail_if (!wic, -1, DP ("Create params are NULL\n"));

		wic->hwnd = hwnd;

		st = WicUpdateMemdc (wic);
		fail_if (!st, -1, DP ("Update MemDC failed\n"));

		SetLastError (0);
		st = SetWindowLong (hwnd, 0, (LONG)wic);
		fail_if (!st && GetLastError () != NO_ERROR, -1, DP ("SetWindowLong failed, %i\n", GetLastError ()));

		//now we are all good, mark the wic as successfully created.  This signals the
		//WM_DESTROY routine to destroy it.  I'm doing this because I don't know if returning
		//-1 from WM_CREATE will get me a WM_DESTROY or not.  Just in case it does, I'll have
		//this flag to make sure that WM_DESTROY doesn't do its thing.
		//If windows doesn't initialize the extra window bytes to zero, I'm going to fucking
		//KILL someone.  UPDATE: Looks like they are initted to zero.
		wic->created = 1;

		return 0;

	case WM_PAINT:
		wic = (wic_t *)GetWindowLong (hwnd, 0);
		fail_if (!wic, 0, ValidateRect (hwnd, NULL); DP ("GetWindowLong failed, %i\n", GetLastError ()));

		st = WicPaint (wic);
		fail_if (!st, 0, ValidateRect (hwnd, NULL); DP ("GetWindowLong failed, %i\n", GetLastError ()));

		return 0;

	case WM_TIMER:
		wic = (wic_t *)GetWindowLong (hwnd, 0);
		fail_if (!wic, 0, DP ("GetWindowLong failed, %i\n", GetLastError ()));

		st = WicRunClocks (wic, 0);
		fail_if (!st, 0, DP ("Clock 0x%.8X has died.\n", hwnd));

		return 0;
		

	case WM_DESTROY:
		wic = (wic_t *)GetWindowLong (hwnd, 0);
		fail_if (!wic, 0, DP ("No WIC in WM_DESTROY\n"));

		//if wic is null, either the create routine failed mid-way through, or something
		//else bad happened, and either way, there's nothing for us to do.

		if (!wic->created)
			return 0; //sentinel flag for failed WM_CREATE

		WicDestroyWic (wic);

		return 0;


	case WM_LBUTTONDOWN:
		return DefWindowProc (hwnd, WM_SYSCOMMAND, SC_MOVE + 2, lParam);

	case WM_SIZE:
		wic = (wic_t *)GetWindowLong (hwnd, 0);
		fail_if (!wic, 0, DP ("GetWindowLong failed, %i\n", GetLastError ()));

		st = WicUpdateMemdc (wic);
		fail_if (!st, 0, DP ("Could not update MemDC\n"));

		st = WicRunClocks (wic, 1);
		fail_if (!st, 0, DP ("Clock 0x%.8X has died\n", hwnd));

		return 0;

	case WM_ADDCLOCK:
		fail_if (wParam != WicSentinel, 0, DP ("Clock 0x%X received invalid WM_ADDCLOCK message\n", hwnd));
		wic = (wic_t *)GetWindowLong (hwnd, 0);
		fail_if (!wic, 1, DP ("GetWindowLong failed, %i\n", GetLastError ()));

		st = WicAddClockToList (wic, (clock_t *)lParam);
		fail_if (!st, 1, DP ("Could not add clock to wic\n"));

		//NOTE: we're returning 2, which signals success, even if this call fails.  Why?
		//Because the success or failure of this message is concerned with who owns the
		//buffer passed in lParam.
		st = WicRunClocks (wic, 0);
		fail_if (!st, 2, DP ("WicRunClocks failed\n"));

		return 2;


	}



	return DefWindowProc (hwnd, message, wParam, lParam);
failure:
	return st;
}

int WicAddClock (parerror_t *pe, HWND hwnd, clock_t *clock)
{
	DWORD tid;
	DERR st;

	fail_if (!IsWindow (hwnd), 0, PE ("WIC handle passed in is not a window", 0, 0, 0, 0));
	fail_if (!WicIsClock (NULL, hwnd), 0, PE ("WIC handle passed in is not valid", 0, 0, 0, 0));

	tid = GetWindowThreadProcessId (hwnd, NULL);
	
	fail_if (tid != GetCurrentThreadId (), 0, PE ("WIC handle is for a window in a different thread", 0, 0, 0, 0));

	st = SendMessage (hwnd, WM_ADDCLOCK, WicSentinel, (LPARAM)clock);
	fail_if (st == 1, 0, PE ("WM_ADDCLOCK message failed", 0, 0, 0, 0)); //definite failure
	fail_if (st != 2, 0, PE ("WIC window returned invalid response from WM_ADDCLOCK message", 0, 0, 0, 0)); //something else went wrong

	//ok, so it returned 2, and it now owns the buffer, so return 1
	return 1;
failure:
	return 0;
}




int WicAddToArray (HWND hwnd)
{
	int x;
	void *temp;

	for (x=0;x<WicNumWindows;x++)
	{
		if (!WicWindows[x])
		{
			WicWindows[x] = hwnd;
			return 1;
		}

		if (!IsWindow (WicWindows[x]))
		{
			WicWindows[x] = hwnd;
			return 1;
		}
	}

	if (!WicWindows)
		temp = PlMalloc (sizeof (HWND) * (WicNumWindows + 1));
	else
		temp = PlRealloc (WicWindows, sizeof (HWND) * (WicNumWindows + 1));
	if (!temp)
		return 0;

	WicWindows = temp;


	WicWindows[WicNumWindows++] = hwnd;

	return 1;
}

void WicRemoveFromArray (HWND hwnd)
{
	int x;

	for (x=0;x<WicNumWindows;x++)
	{
		if (WicWindows[x] == hwnd)
			WicWindows[x] = NULL;
	}


}

	


HWND WicCreate (parerror_t *pe, char *title, int x, int y, int w, int height)
{

	DERR st;
	wic_t *wic = NULL;
	HWND hwnd = NULL;

	fail_if (!PlIsMainThread (), 0, PE ("wic.create must be run in the main thread", 0, 0, 0, 0));
	fail_if (!title, 0, PE_BAD_PARAM (1));

	wic = PlMalloc (sizeof (wic_t));
	fail_if (!wic, 0, PE_OUT_OF_MEMORY (sizeof (wic_t)));
	PlMemset (wic, 0, sizeof (wic_t));

	hwnd = CreateWindowEx (WS_EX_APPWINDOW | WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE, WIC_CLASS, title, 
		WS_BORDER | WS_POPUP | WS_THICKFRAME, x, y, w, height, NULL, NULL, instance, wic);
	fail_if (!hwnd, 0, PE ("CreateWindow failed, %i", GetLastError (), 0, 0, 0));

	//at this point, the window owns the wic_t, so NULL it out.  If we have a failure from here on out,
	//destroying the window takes care of the wic.
	wic = NULL;

	//add it to the list
	st = WicAddToArray (hwnd);
	fail_if (!st, 0, PE_OUT_OF_MEMORY (sizeof (HWND) * (WicNumWindows + 1)));

	return hwnd;

failure:

	if (wic)
		WicDestroyWic (wic);

	if (hwnd)
		DestroyWindow (hwnd);

	return 0;
}

void WicDestroy (parerror_t *pe, HWND hwnd)
{
	DERR st;

	fail_if (!PlIsMainThread (), 0, PE ("wic.destroy must run in the main thread", 0, 0, 0, 0));
	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	//ok, great.

	st = DestroyWindow (hwnd);
	fail_if (!st, 0, PE ("DestroyWindow failed, %i\n", GetLastError (), 0, 0, 0));

	WicRemoveFromArray (hwnd);

	//return
failure:
	return;
}

int WicIsClock (parerror_t *pe, HWND hwnd)
{
	DERR st;
	int x;

	//this function is called from both script and code.  Code calls with pe NULL, because
	//the main thread has been verified through some other means.
	fail_if (pe && !PlIsMainThread (), 0, PE ("wic.verify must run in the main thread", 0, 0, 0, 0));

	for (x=0;x<WicNumWindows;x++)
	{
		if (WicWindows[x] == hwnd)
			return 1;
	}

	return 0;
failure:
	return 0;
}


void WicTestFunction (void)
{

	int foo;

	foo = 1;

	WicDprintf ("Bobadoo\n");

}