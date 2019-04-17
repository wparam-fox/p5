/* Copyright 2007 wParam, licensed under the GPL */

#ifndef _WIC_H_
#define _WIC_H_

#define IS_PLUGIN
#include "..\common\libplug.h"

#define WIC_CLASS "p5.wic"
#define DP WicDprintf


extern HINSTANCE instance;


struct clock_s;
typedef struct clock_s clock_t;

#define CLOCK_UPDATE_NEVER (0x7FFFFFFFFFFFFFFF)
struct clock_s
{
	clock_t *next;

	__int64 nextupdate; //ALL TIMES ARE IN UTC!

	void (*paint)(HDC, clock_t *); //draw into wic->dc.hdc, update nextupdate as appropriate
	void (*destroy)(clock_t *); //deallocate all resources, but do NOT destroy clock_t

	char data[0];
};

extern unsigned int WicSentinel;
DERR WicDprintf (char *fmt, ...);
int WicIsClock (parerror_t *pe, HWND hwnd);
int WicAddClock (parerror_t *pe, HWND hwnd, clock_t *clock);
LRESULT CALLBACK WicProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	


#endif