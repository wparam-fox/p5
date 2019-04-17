/* Copyright 2007 wParam, licensed under the GPL */

#include <windows.h>
#include "video.h"



void VidChangeDisplaySettings (parerror_t *pe, char *device, int width, int height, int bpp, int frequency, int flags)
{
	DEVMODE dm = {0};
	DERR st;

	dm.dmSize = sizeof (DEVMODE);

	if (width != -1)
	{
		dm.dmFields |= DM_PELSWIDTH;
		dm.dmPelsWidth = width;
	}
	
	if (height != -1)
	{
		dm.dmFields |= DM_PELSHEIGHT;
		dm.dmPelsHeight = height;
	}

	if (bpp != -1)
	{
		dm.dmFields |= DM_BITSPERPEL;
		dm.dmBitsPerPel = bpp;
	}

	if (frequency != -1)
	{
		dm.dmFields |= DM_DISPLAYFREQUENCY;
		dm.dmDisplayFrequency = frequency;
	}

	st = ChangeDisplaySettingsEx (device, &dm, NULL, flags, NULL);
	fail_if (st != DISP_CHANGE_SUCCESSFUL, st, PE ("Could not change display settings, %i", st, 0, 0, 0));

	//return;
failure:
	return;
}

void VidResetAdapter (parerror_t *pe, char *devname)
{
	DERR st;

	st = ChangeDisplaySettingsEx (devname, NULL, NULL, CDS_RESET, NULL);
	fail_if (st != DISP_CHANGE_SUCCESSFUL, st, PE ("Could not reset display settings, %i", st, 0, 0, 0));

	//return;
failure:
	return;
}

void VidReset (parerror_t *pe)
{
	VidResetAdapter (pe, NULL);
}


typedef struct _REAL_DISPLAY_DEVICE 
{
	DWORD cb;
	TCHAR DeviceName[32];
	TCHAR DeviceString[128];
	DWORD StateFlags;
	TCHAR DeviceID[128];
	TCHAR DeviceKey[128];
} REAL_DISPLAY_DEVICE;

BOOL (WINAPI * EnumDisplayDevicesA)(
  LPSTR lpDevice,                // device name
  DWORD iDevNum,                   // display device
  REAL_DISPLAY_DEVICE *DisplayDevice, // device information
  DWORD dwFlags                    // reserved
) = NULL;


char *VidGetAdapterName (parerror_t *pe, int index)
{
	REAL_DISPLAY_DEVICE dd = {0};
	DERR st;
	char *out = NULL;
	HMODULE hmod;
	int len;

	dd.cb = sizeof (dd);

	if (!EnumDisplayDevicesA)
	{
		hmod = GetModuleHandle ("user32.dll");
		fail_if (!hmod, 0, PE ("Could not get module handle for user32.dll, %i", GetLastError (), 0, 0, 0));

		EnumDisplayDevicesA = (void *)GetProcAddress (hmod, "EnumDisplayDevicesA");
		fail_if (!EnumDisplayDevicesA, 0, PE ("Could not get proc address for EnumDisplayDevicesA, %i", GetLastError (), 0, 0, 0));
	}

	st = EnumDisplayDevicesA (NULL, index, &dd, 0);
	fail_if (!st, 0, PE ("Device index %i invalid", index, 0, 0, 0));

	//TODO: should we force a null character at the end of dd.DeviceName?  If it changes
	//in the future, we'll be screwed.  Though we'll be screwed anyway just because we've
	//had to hardcode the damn thing in here because my SDK is so old.

	len = PlStrlen (dd.DeviceName) + 1;
	out = ParMalloc (len);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (len));
	PlStrcpy (out, dd.DeviceName);

	return out;
failure:

	if (out)
		ParFree (out);

	return NULL;

}


char *VidGetMonitorName (parerror_t *pe, char *display, int index)
{
	REAL_DISPLAY_DEVICE dd = {0};
	DERR st;
	char *out = NULL;
	HMODULE hmod;
	int len;

	fail_if (!display, 0, PE_BAD_PARAM (1));

	dd.cb = sizeof (dd);

	if (!EnumDisplayDevicesA)
	{
		hmod = GetModuleHandle ("user32.dll");
		fail_if (!hmod, 0, PE ("Could not get module handle for user32.dll, %i", GetLastError (), 0, 0, 0));

		EnumDisplayDevicesA = (void *)GetProcAddress (hmod, "EnumDisplayDevicesA");
		fail_if (!EnumDisplayDevicesA, 0, PE ("Could not get proc address for EnumDisplayDevicesA, %i", GetLastError (), 0, 0, 0));
	}

	st = EnumDisplayDevicesA (display, index, &dd, 0);
	fail_if (!st, 0, PE ("Monitor index %i invalid", index, 0, 0, 0));

	//TODO: should we force a null character at the end of dd.DeviceName?  If it changes
	//in the future, we'll be screwed.  Though we'll be screwed anyway just because we've
	//had to hardcode the damn thing in here because my SDK is so old.

	len = PlStrlen (dd.DeviceName) + 1;
	out = ParMalloc (len);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (len));
	PlStrcpy (out, dd.DeviceName);

	return out;
failure:

	if (out)
		ParFree (out);

	return NULL;

}