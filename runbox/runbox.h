/* Copyright 2006 wParam, licensed under the GPL */

#ifndef _RUNBOX_H_
#define _RUNBOX_H_

#include "..\common\libplug.h"

extern HINSTANCE instance;


#define RUNBOX_CLASS "p5.runbox"

LRESULT CALLBACK RbProc (HWND, UINT, WPARAM, LPARAM);


void RbDestroyAll (void);
void RbClearHistory (void);




#endif