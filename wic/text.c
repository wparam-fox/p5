/* Copyright 2007 wParam, licensed under the GPL */

#include <windows.h>
#include "wic.h"



typedef struct
{
	HFONT font;
	COLORREF color;
	int x;
	int y;
	char text[0];
} label_t;


void TxtLabelDestroy (clock_t *c)
{
	label_t *l = (label_t *)c->data;

	if (l->font)
	{
		DeleteObject (l->font);
		l->font = NULL;
	}

}

void TxtLabelPaint (HDC hdc, clock_t *c)
{
	label_t *l = (label_t *)c->data;
	HFONT oldfont = NULL;
	DERR st;

	oldfont = SelectObject (hdc, l->font);
	fail_if (!oldfont, 0, DP ("SelectObject failed, %i\n", GetLastError ()));

	st = SetBkMode (hdc, TRANSPARENT);
	fail_if (!st, 0, DP ("SetBkMode failed, %i\n", GetLastError ()));
	st = SetTextColor (hdc, l->color);
	fail_if (!st, 0, DP ("SetTextColor failed, %i\n", GetLastError ()));

	st = TextOut (hdc, l->x, l->y, l->text, PlStrlen (l->text));
	fail_if (!st, 0, DP ("TextOut failed, %i\n", GetLastError ()));

	SelectObject (hdc, oldfont);
	c->nextupdate = CLOCK_UPDATE_NEVER;
	return;
failure:

	if (oldfont)
		SelectObject (hdc, oldfont);

	return;
}


	

	


void TxtAddLabel (parerror_t *pe, HWND clock, int x, int y, char *text, char *font, COLORREF color)
{
	DERR st;
	clock_t *c = NULL;
	label_t *l = NULL;
	int len;

	fail_if (!PlIsMainThread (), 0, PE ("wic.add.label must be run in the main thread", 0, 0, 0, 0));
	fail_if (!clock, 0, PE_BAD_PARAM (1));
	fail_if (!text, 0, PE_BAD_PARAM (4));
	fail_if (!WicIsClock (NULL, clock), 0, PE ("Window 0x%X is not an infernal chronometer", clock, 0, 0, 0));

	len = sizeof (clock_t) + sizeof (label_t) + PlStrlen (text) + 1;
	c = PlMalloc (len);
	fail_if (!c, 0, PE_OUT_OF_MEMORY (len));
	PlMemset (c, 0, len);

	c->destroy = TxtLabelDestroy;
	c->paint = TxtLabelPaint;
	l = (label_t *)c->data;
	l->color = color;
	l->x = x;
	l->y = y;
	PlStrcpy (l->text, text);

	if (!font)
		l->font = GetStockObject (ANSI_FIXED_FONT);
	else
		l->font = CreateFont (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, font);
	fail_if (!l->font, 0, PE_STR ("Could not find font %s, %i", COPYSTRING (font), GetLastError (), 0, 0));

	//ok, that all looks good.
	st = WicAddClock (pe, clock, c);
	fail_if (!st, 0, 0);

	//ok, Add clock passed, so now the buffer is owned by the WIC.
	c = NULL;
	l = NULL;

	//actually, I guess we're done.
	return;
failure:
	if (l && l->font)
		DeleteObject (l->font);

	if (c)
		PlFree (c);

	return;
}
