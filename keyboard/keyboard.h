/* Copyright 2006 wParam, licensed under the GPL */

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include "..\common\libplug.h"



LRESULT CALLBACK KbHotkeyCallback (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int KbForcedHotkeyCleanup (parerror_t *pe);

int KbTranslateVirtualKey (char *blah);
char *KbUntranslateVirtualKey (int vk);


int KbPauseShutdown (parerror_t *pe);


extern HINSTANCE instance;
extern int KbVerbose;










#endif
