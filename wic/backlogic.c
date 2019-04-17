/* Copyright 2007 wParam, licensed under the GPL */

#include <windows.h>
#include "wic.h"

#define P(sem)
//globals:
struct bkglobals_t
{
	HANDLE thread;
	HANDLE stopevent;
	HANDLE addevent;

	int width;
	int height;
	HDC memdc;
	HBITMAP membit;
	HBITMAP oldbit;

	HANDLE sem; //clock semaphore
	clock_t *clocks;
} BkGlobals = {0};



int BkStartThread (parerror_t *pe, int w, int h)
{
	DERR st;

	if (BkGlobals.thread)
		return 1; //already started?  Nothing to do.




}