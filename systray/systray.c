/* Copyright 2006 wParam, licensed under the GPL */

#include <windows.h>
#include "systray.h"

#define NIM_SETFOCUS		0x03
#define NIM_SETVERSION		0x04

typedef struct {
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON hIcon;
    TCHAR szTip[64];
    DWORD dwState;
    DWORD dwStateMask;
    TCHAR szInfo[256];
    union {
        UINT uTimeout;
        UINT uVersion;
    };
    TCHAR szInfoTitle[64];
    DWORD dwInfoFlags;
    GUID guidItem;
} nex_t;
