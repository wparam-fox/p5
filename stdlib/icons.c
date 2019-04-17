/* Copyright 2008 wParam, licensed under the GPL */

#include <windows.h>
#include "sl.h"

#undef small /* WTF was it defined as?? */
typedef struct
{
	HICON big;
	HICON small;
} dualicon_t;


char *IcProcessHicon (parerror_t *pe, HICON icon)
{
	DERR st;
	char *out;
	int wset = 0;

	if (icon)
	{
		st = PlAddWatcher ("HICON", icon);
		fail_if (!DERR_OK (st), st, PE_DERR ("Could not set watcher for icon:", st, 0, 0));
		wset = 1;
	}

	out = PlBinaryToType (pe, &icon, sizeof (HICON), "HICON");
	fail_if (!out, 0, 0);

	return out;
failure:
	if (out)
		ParFree (out);

	if (wset)
		PlDeleteWatcher ("HICON", icon);

	if (icon)
		DestroyIcon (icon);

	return NULL;
}

/* This returns a type string from the parser heap.  Generally it is
 * easiest to just return the value this function returns; it handles
 * all necessary cleanup of the values in *di, whether in success
 * or in failure. */
char *IcProcessDualIcon (parerror_t *pe, dualicon_t *di)
{
	DERR st;
	int bset = 0, sset = 0;
	char *out = NULL;

	/* Ok.  We've got our icons.  Now the fun part of actually returning them.
	 * Need to set watchers on each hicon, build the type, and return it.*/
	if (di->big)
	{
		st = PlAddWatcher ("HICON", di->big);
		fail_if (!DERR_OK (st), st, PE_DERR ("Could not set watcher for big icon:", st, 0, 0));
		bset = 1;
	}

	if (di->small)
	{
		st = PlAddWatcher ("HICON", di->small);
		fail_if (!DERR_OK (st), st, PE_DERR ("Could not set watcher for small icon:", st, 0, 0));
		sset = 1;
	}

	/* Awesome.  Done, basically. */
	out = PlBinaryToType (pe, di, sizeof (dualicon_t), "dicon");
	fail_if (!out, 0, 0);

	return out;
failure:
	if (out)
		ParFree (out);

	if (sset)
		PlDeleteWatcher ("HICON", di->small);

	if (bset)
		PlDeleteWatcher ("HICON", di->big);

	if (di->big)
		DestroyIcon (di->big);

	if (di->small)
		DestroyIcon (di->small);

	return NULL;
}

int IcUnprocessIcon (parerror_t *pe, char *typein, HICON *iconout, int takeownership)
{
	DERR st;
	HICON icon = NULL;
	int len;

	st = PlTypeToBinary (pe, typein, "HICON", &len, &icon, sizeof (HICON));
	fail_if (!st, 0, 0);

	if (icon)
		st = PlCheckWatcher ("HICON", icon);
	else
		st = 1;

	fail_if (!st, 0, PE ("Icon is not valid", 0, 0, 0, 0));

	if (icon && takeownership)
		PlDeleteWatcher ("HICON", icon);

	*iconout = icon;
	return 1;
failure:
	return 0;
}



/* If this function returns success, the caller assumes responsibility
 * for freeing the two icons.  No nulls allowed. */
int IcUnprocessDualIcon (parerror_t *pe, char *typein, dualicon_t *diout, int takeownership)
{
	DERR st;
	dualicon_t di = {0};
	int bvalid, svalid;
	int len;

	st = PlTypeToBinary (pe, typein, "dicon", &len, &di, sizeof (dualicon_t));
	fail_if (!st, 0, 0);

	if (di.big)
		bvalid = PlCheckWatcher ("HICON", di.big);
	else
		bvalid = 1; /* can't put a watcher on NULL, so this must be valid */

	if (di.small)
		svalid = PlCheckWatcher ("HICON", di.small);
	else
		svalid = 1;

	fail_if (!bvalid && !svalid, 0, PE ("Neither icon stored in the dicon is valid", 0, 0, 0, 0));
	fail_if (!bvalid, 0, PE ("Big icon in dicon is not valid", 0, 0, 0, 0));
	fail_if (!svalid, 0, PE ("Small icon in dicon is not valid", 0, 0, 0, 0));

	if (takeownership)
	{
		/* The only reason these should fail is if the thing doesn't exist (which
		 * we already checked for) or if they're NULL, which we also check. */
		if (di.big)
			PlDeleteWatcher ("HICON", di.big);
		if (di.small)
			PlDeleteWatcher ("HICON", di.small);
	}

	diout->big = di.big;
	diout->small = di.small;

	return 1;
failure:
	return 0;
}
	

/*************************************************************************************/


char *IcLoadIcon (parerror_t *pe, char *filename, int index)
{
	DERR st;
	dualicon_t di = {0};
	HICON temp = NULL;

	fail_if (!filename, 0, PE_BAD_PARAM (1));
	fail_if (index < 0, 0, PE_BAD_PARAM (2));

	st = ExtractIconEx (filename, index, &di.big, &di.small, 1);
	fail_if (st == 0, 0, PE ("ExtractIconEx failed, %i", GetLastError (), 0, 0, 0));

	/* I really don't know if I have to do this or not, but I've seen enough
	 * odd behavior because of icons, so I'm doing the copy to make SURE that
	 * all of the icons I return are the same type */
	if (di.big)
	{
		temp = di.big;
		di.big = CopyIcon (temp);
		fail_if (!di.big, 0, PE ("CopyIcon failed for the big icon, %i", GetLastError (), 0, 0, 0));
		DestroyIcon (temp);
		temp = NULL; /* needs to be done in case !di.small */
	}

	if (di.small)
	{
		temp = di.small;
		di.small = CopyIcon (temp);
		fail_if (!di.small, 0, PE ("CopyIcon failed for the small icon, %i", GetLastError (), 0, 0, 0));
		DestroyIcon (temp);
		temp = NULL;
	}

	return IcProcessDualIcon (pe, &di);
failure:
	if (di.big)
		DestroyIcon (di.big);

	if (di.small)
		DestroyIcon (di.small);

	if (temp)
		DestroyIcon (temp);

	return NULL;
}

static HICON IcGetWindowIcon (HWND hwnd, int big)
{
	HICON out;
	HICON temp;

	out = (HICON)SendMessage (hwnd, WM_GETICON, big ? ICON_BIG : ICON_SMALL, 0);

	if (out)
	{
		/* Try to copy the icon.  If it can be copied, then we know 'out' is a
		 * valid icon, and we'll return the copied icon.  Icons need to be 
		 * passed to copyicon before we can return them to the parse system.
		 * A window can return whatever it wants
		 * in response to WM_SETICON, so we have to play these games.  This
		 * becomes an issue for a window that had its icon set with WM_SETICON
		 * and said icon was then destroyed by the process that set it. */

		temp = CopyIcon (out);
		if (temp)
			return temp;

		out = NULL;
	}

	if (!out)
	{
		/* try to get it from the window class */
		out = (HICON)GetClassLong (hwnd, big ? GCL_HICON : GCL_HICONSM);
		
		if (out)
		{
			/* Same logic as before. */
			temp = CopyIcon (out);
			if (temp)
				return temp;

			out = NULL;
		}
	}

	return NULL;
}

char *IcGetIconFromWindow (parerror_t *pe, HWND hwnd)
{
	DERR st;
	dualicon_t di = {0};

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));
	fail_if (!IsWindow (hwnd), 0, PE ("Passed HWND (%x) is not a window.", hwnd, 0, 0, 0));

	di.big = IcGetWindowIcon (hwnd, 1);
	di.small = IcGetWindowIcon (hwnd, 0);
	fail_if (!di.big && !di.small, 0, PE ("Could not get any icons from window %x", hwnd, 0, 0, 0));

	return IcProcessDualIcon (pe, &di);
failure:
	if (di.big)
		DestroyIcon (di.big);
	if (di.small)
		DestroyIcon (di.small);

	return NULL;
}


void IcDestroyHicon (parerror_t *pe, char *icon)
{
	DERR st;
	HICON hic = NULL;


	fail_if (!icon, 0, PE_BAD_PARAM (1));

	st = IcUnprocessIcon (pe, icon, &hic, 1);
	fail_if (!st, 0, 0);

	DestroyIcon (hic);

	return;

failure:
	if (hic)
		DestroyIcon (hic); /* wouldn't be set if it wasn't real */

	return;
}

void IcDestroyDualIcon (parerror_t *pe, char *icon)
{
	DERR st;
	dualicon_t di = {0};

	fail_if (!icon, 0, PE_BAD_PARAM (1));

	st = IcUnprocessDualIcon (pe, icon, &di, 1);
	fail_if (!st, 0, 0);

	DestroyIcon (di.big);
	DestroyIcon (di.small);

	return;
failure:
	if (di.big)
		DestroyIcon (di.big);
	if (di.small)
		DestroyIcon (di.small);
}

void IcDrawDualIcon (parerror_t *pe, char *icon, int x, int y)
{
	DERR st;	
	dualicon_t di = {0};
	HDC hdc = NULL;

	fail_if (!icon, 0, PE_BAD_PARAM (1));

	st = IcUnprocessDualIcon (pe, icon, &di, 0);
	fail_if (!st, 0, 0);

	hdc = GetDC (NULL);
	fail_if (!hdc, 0, PE ("Could not GetDC, %i", GetLastError (), 0, 0, 0));

	if (di.big)
	{
		st = DrawIconEx (hdc, x, y, di.big, 0, 0, 0, NULL, DI_NORMAL);
		fail_if (!st, 0, PE ("Could not draw big icon, %i", GetLastError (), 0, 0, 0));
	}
	if (di.small)
	{
		st = DrawIconEx (hdc, x+32, y+32, di.small, 0, 0, 0, NULL, DI_NORMAL);
		fail_if (!st, 0, PE ("Could not draw small icon, %i", GetLastError (), 0, 0, 0));
	}

	ReleaseDC (NULL, hdc);

	return;
failure:
	if (hdc)
		ReleaseDC (NULL, hdc);
}



int IcGetIconSize (parerror_t *pe, ICONINFO *ii, int *xout, int *yout)
{
	DERR st; /* return value */
	HBITMAP temp;
	BITMAP bi = {0};

	if (ii->hbmColor)
		temp = ii->hbmColor;
	else
		temp = ii->hbmMask;

	st = GetObject (temp, sizeof (BITMAP), &bi);
	fail_if (!st, 0, PE ("Could not GetObject(), %i", GetLastError (), 0, 0, 0));

	if (temp == ii->hbmMask)
		bi.bmHeight = bi.bmHeight / 2;
	
	*xout = bi.bmWidth;
	*yout = bi.bmHeight;

	return 1;
failure:
	return 0;
}


int IcFrungeIconBits (parerror_t *pe, HICON *icon, int (*frunge)(parerror_t *, BYTE *bitmap, int x, int y, void *), void *param)
{
	DERR st; /* return value */
	ICONINFO ii = {0};
	int ix, iy;
	HDC screen = NULL;
	//HDC memdc = NULL;
	//HBITMAP oldbit = NULL;
	BITMAPINFO bmi = {0};
	BYTE *bits = NULL;
	HICON new;


	st = GetIconInfo (*icon, &ii);
	fail_if (!st, 0, PE ("Could not GetIconInfo(), %i", GetLastError (), 0, 0, 0));
	fail_if (!ii.hbmColor, 0, PE ("Only color icons are supported", 0, 0, 0, 0));

	st = IcGetIconSize (pe, &ii, &ix, &iy);
	fail_if (!st, 0, 0);

	screen = GetDC (NULL);
	fail_if (!screen, 0, PE ("Could not Get DC for screen, %i", GetLastError (), 0, 0, 0));

	//memdc = CreateCompatibleDC (screen);
	//fail_if (!memdc, 0, PE ("Could not Create mem DC for icon operation, %i", GetLastError (), 0, 0, 0));

	//ReleaseDC (NULL, screen);
	//screen = NULL;

	//oldbit = SelectObject (memdc, ii.hbmColor);
	//fail_if (!oldbit, 0, PE ("Could not select icon bitmap into dc, %i", GetLastError (), 0, 0, 0));

	bmi.bmiHeader.biSize = sizeof (bmi.bmiHeader);
	bmi.bmiHeader.biWidth = ix;
	bmi.bmiHeader.biHeight = iy;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biCompression = BI_RGB;

	bits = PlMalloc (3 * ix * iy + 1);
	fail_if (!bits, 0, PE_OUT_OF_MEMORY (3 * ix * iy + 1));
	bits[3 * ix * iy] = 0xFE;

	st = GetDIBits (screen, ii.hbmColor, 0, iy, bits, &bmi, DIB_RGB_COLORS);
	fail_if (!st, 0, PE ("Could not Get DI bits, %i", GetLastError (), 0, 0, 0));
	fail_if (bits[3 * ix * iy] != 0xFE, 0, PE ("GetDIBits ran over the end of the array... this is bad.", 0, 0, 0, 0));

	st = frunge (pe, bits, ix, iy, param);
	fail_if (!st, 0, 0);

	st = SetDIBits (screen, ii.hbmColor, 0, iy, bits, &bmi, DIB_RGB_COLORS);
	fail_if (!st, 0, PE ("Could not Set DI bits, %i", GetLastError (), 0, 0, 0));

	new = CreateIconIndirect (&ii);
	fail_if (!new, 0, PE ("Could not create new icon, %i", GetLastError (), 0, 0, 0));

	DestroyIcon (*icon);
	*icon = new;

	st = 1;
failure:

	if (bits)
		PlFree (bits);

	if (screen)
		ReleaseDC (NULL, screen);

	if (ii.hbmColor)
		DeleteObject (ii.hbmColor);

	if (ii.hbmMask)
		DeleteObject (ii.hbmMask);

	return st;
}



static int IcChangeIconColorFrunge (parerror_t *pe, BYTE *bits, int dx, int dy, void *param)
{
	int *delta = param;
	int x;
	BYTE *p1, *p2, *targ;

	p2 = (BYTE *)&delta[0];
	targ = (BYTE *)&delta[1];

	for (x=0;x<dx * dy;x++)
	{
		p1 = (bits + (x * 3));

		if (p1[0] == p2[0] && p1[1] == p2[1] && p1[2] == p2[2])
		{
			PlMemcpy (p1, targ, 3);
		}
	}

	return 1;
}


char *IcChangeDualIconColor (parerror_t *pe, char *iconin, int source, int target)
{
	DERR st;
	dualicon_t di = {0};
	int delta[2] = {source, target};

	fail_if (!iconin, 0, PE_BAD_PARAM (1));

	st = IcUnprocessDualIcon (pe, iconin, &di, 1);
	fail_if (!st, 0, 0);

	if (di.big)
	{
		st = IcFrungeIconBits (pe, &di.big, IcChangeIconColorFrunge, delta);
		fail_if (!st, 0, 0);
	}

	if (di.small)
	{
		st = IcFrungeIconBits (pe, &di.small, IcChangeIconColorFrunge, delta);
		fail_if (!st, 0, 0);
	}

	return IcProcessDualIcon (pe, &di);
failure:

	if (di.big)
		DestroyIcon (di.big);
	if (di.small)
		DestroyIcon (di.small);

	return NULL;
}


typedef struct frunge_s
{
	float start;
	float range;
	float delta;

	/* the func should return 1 if the color was changed and needs to be reapplied */
	int (*func)(double *h, double *s, double *v, struct frunge_s *);
} frunge_t;

#define LIMIT(s) if (s > 255) s = 255; if (s < 0) s = 0
static int IcRotateIconColorFrunge (parerror_t *pe, BYTE *bits, int dx, int dy, void *param)
{
	frunge_t *delta = param;
	int x;
	BYTE *p1;
	double r, g, b, h, s, v, p, q, t, f, min, max;
	int hit;
	double *map[6][3] = {	{&v, &t, &p},
							{&q, &v, &p},
							{&p, &v, &t},
							{&p, &q, &v},
							{&t, &p, &v},
							{&v, &p, &q}};
	int hindex;
	int temp;



	
	for (x=0;x<dx * dy;x++)
	{
		p1 = (bits + (x * 3));

		r = ((double)p1[2]) / 255.0;
		g = ((double)p1[1]) / 255.0;
		b = ((double)p1[0]) / 255.0;

		min = r < g ? (r < b ? r : b) : (b < g ? b : g);
		max = r > g ? (r > b ? r : b) : (b > g ? b : g);

		if (max == min)
			h = 0;
		else if (max == r)
			h = 60 * ((g - b) / (max - min)) + 0;
		else if (max == g)
			h = 60 * ((b - r) / (max - min)) + 120;
		else//if (max == b)
			h = 60 * ((r - g) / (max - min)) + 240;
		while (h > 360.0)
			h -= 360.0;
		h = PlFmod (h, 360.0);

		if (max == 0.0)
			s = 0;
		else
			s =	1 - (min / max);

		v = max;

		hit = delta->func (&h, &s, &v, delta);
		if (hit)
		{
			hindex = ((int)(h / 60.0)) % 6;
			if (hindex < 0)
				hindex = 0;
			if (hindex > 5) /* paranoia much? */
				hindex = 5;

			f = (h / 60.0) - PlFloor (h / 60.0);
			p = v * (1 - s);
			q = v * (1 - (f * s));
			t = v * (1 - (1 - f) * s);

			r = *map[hindex][0];
			g = *map[hindex][1];
			b = *map[hindex][2];

			temp = (int)(r * 255.0);
			LIMIT (temp);
			p1[2] = temp;

			temp = (int)(g * 255.0);
			LIMIT (temp);
			p1[1] = temp;

			temp = (int)(b * 255.0);
			LIMIT (temp);
			p1[0] = temp;
		}

	}

	return 1;
}

static int IcRotateIconColorFrungeNormal (double *hin, double *sin, double *vin, frunge_t *delta)
{
	double h = *hin;
	int hit = 0;
	double start, range, change;

	start = delta->start;
	range = delta->range;
	change = delta->delta;

	if (h >= start - range && h <= start + range)
		hit = 1;
	else if (start - range < 0 && ((h - 360.0) >= start - range && (h - 360.0) <= start + range))
		hit = 1;
	else if (start + range > 360 && ((h + 360.0) >= start - range && (h + 360.0) <= start + range))
		hit = 1;

	if (hit)
	{
		/* Apply the change */
		h += change;
		h = PlFmod (h, 360.0);
		*hin = h;
	}

	return hit;
}

static int IcRotateIconColorFrungeGetHue (double *hin, double *sin, double *vin, frunge_t *delta)
{
	if (*vin == 0.0) /* black; don't change */
		return 0;

	if (*sin == 0.0) /* shade of grey; don't change */
		return 0;

	*sin = 1.0;
	*vin = 1.0;
	return 1;
}

char *IcRotateDualIconColor (parerror_t *pe, char *iconin, float start, float range, float change)
{
	DERR st;
	dualicon_t di = {0};
	frunge_t frunge = {start, range, change, IcRotateIconColorFrungeNormal};

	fail_if (!iconin, 0, PE_BAD_PARAM (1));

	st = IcUnprocessDualIcon (pe, iconin, &di, 1);
	fail_if (!st, 0, 0);

	if (di.big)
	{
		st = IcFrungeIconBits (pe, &di.big, IcRotateIconColorFrunge, &frunge);
		fail_if (!st, 0, 0);
	}

	if (di.small)
	{
		st = IcFrungeIconBits (pe, &di.small, IcRotateIconColorFrunge, &frunge);
		fail_if (!st, 0, 0);
	}

	return IcProcessDualIcon (pe, &di);
failure:

	if (di.big)
		DestroyIcon (di.big);
	if (di.small)
		DestroyIcon (di.small);

	return NULL;
}

char *IcHuifyDualIconColor (parerror_t *pe, char *iconin)
{
	DERR st;
	dualicon_t di = {0};
	frunge_t frunge = {0, 0, 0, IcRotateIconColorFrungeGetHue};

	fail_if (!iconin, 0, PE_BAD_PARAM (1));

	st = IcUnprocessDualIcon (pe, iconin, &di, 1);
	fail_if (!st, 0, 0);

	if (di.big)
	{
		st = IcFrungeIconBits (pe, &di.big, IcRotateIconColorFrunge, &frunge);
		fail_if (!st, 0, 0);
	}

	if (di.small)
	{
		st = IcFrungeIconBits (pe, &di.small, IcRotateIconColorFrunge, &frunge);
		fail_if (!st, 0, 0);
	}

	return IcProcessDualIcon (pe, &di);
failure:

	if (di.big)
		DestroyIcon (di.big);
	if (di.small)
		DestroyIcon (di.small);

	return NULL;
}


/******************************************************************************/

typedef struct hwndicon_s
{
	HWND hwnd;

	HICON big;
	HICON small;

	struct hwndicon_s *next;
} hwndicon_t;

hwndicon_t *IcWindowList = NULL;
int IcWindowAutoCleanup = 0;

hwndicon_t *IcWindowFind (HWND hwnd)
{
	hwndicon_t *out;

	out = IcWindowList;
	while (out)
	{
		if (out->hwnd == hwnd)
			break;

		out = out->next;
	}
	return out;
}

void IcWindowDump (void)
{
	hwndicon_t *hi;

	PlPrintf ("Currently stored icons:\n");
	hi = IcWindowList;
	while (hi)
	{
		PlPrintf ("0x%.8X: 0x%.8X,0x%.8X\n", hi->hwnd, hi->big, hi->small);
		hi = hi->next;
	}
}

void IcWindowDelete (hwndicon_t *hi)
{
	DestroyIcon (hi->big);
	DestroyIcon (hi->small);
	PlFree (hi);
}

void IcWindowFinalCleanup (void)
{
	/* Clear *ALL* hwndicons.  This is called on plugin unload most of the time */
	hwndicon_t *walk, *temp;

	walk = IcWindowList;
	while (walk)
	{
		temp = walk->next;
		IcWindowDelete (walk);
		walk = temp;
	}
	IcWindowList = NULL;
}

void IcWindowCleanup (void)
{
	hwndicon_t **walk, *del;

	walk = &IcWindowList;
	while (*walk)
	{
		if (!((*walk)->hwnd) || !IsWindow ((*walk)->hwnd))
		{
			del = *walk;
			*walk = del->next; /* unlinks del */

			IcWindowDelete (del);
			continue;
		}

		walk = &((*walk)->next);
	}
}

int IcWindowSetAutoCleanup (int count, int newval)
{
	int old;

	if (count == 1) //function only got 1 parameter, don't access newcount
		return IcWindowAutoCleanup;

	old = IcWindowAutoCleanup;
	IcWindowAutoCleanup = newval;

	return old;
}

void IcWindowSetIcon (parerror_t *pe, HWND hwnd, char *diconin)
{
	DERR st;
	dualicon_t di = {0};
	hwndicon_t *hi = NULL;
	hwndicon_t *new = NULL;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));
	fail_if (!diconin, 0, PE_BAD_PARAM (2));
	fail_if (!IsWindow (hwnd), 0, PE ("Handle does not seem to be a window", 0, 0, 0, 0));

	st = IcUnprocessDualIcon (pe, diconin, &di, 1);
	fail_if (!st, 0, 0);

	hi = IcWindowFind (hwnd); /* we'll use this later */
	if (hi)
		hi->hwnd = NULL; /* This is a REALLY lazy way to be doing this... */

	if (hi || IcWindowAutoCleanup)
		IcWindowCleanup ();

	new = PlMalloc (sizeof (hwndicon_t));
	fail_if (!new, 0, PE_OUT_OF_MEMORY (sizeof (hwndicon_t)));

	new->big = di.big;
	new->small = di.small;
	di.big = di.small = NULL; /* ownership moved to 'new' */
	fail_if (!new->big && !new->small, 0, PE ("Crazy invalid dicon detected", 0, 0, 0, 0)); /* paranoia... */
	if (!new->big)
		new->big = CopyIcon (new->small);
	if (!new->small)
		new->small = CopyIcon (new->big);
	fail_if (!new->big || !new->small, 0, PE ("Could not duplicate icon to fill missing slot, %i", GetLastError (), 0, 0, 0));
	new->hwnd = hwnd;

	SendMessage (hwnd, WM_SETICON, ICON_BIG, (LPARAM)new->big);
	SendMessage (hwnd, WM_SETICON, ICON_SMALL, (LPARAM)new->small);

	

	new->next = IcWindowList;
	IcWindowList = new;

	return;
failure:
	if (new)
	{
		/* Can't use IcWindowDelete because these may be null */
		if (new->big)
			DestroyIcon (new->big);
		if (new->small)
			DestroyIcon (new->small);
		PlFree (new);
	}

	if (di.big)
		DestroyIcon (di.big);
	if (di.small)
		DestroyIcon (di.small);

	return;
}

void IcWindowClearIcon (parerror_t *pe, HWND hwnd)
{
	DERR st;
	hwndicon_t *hi;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	hi = IcWindowFind (hwnd);
	fail_if (!hi, 0, PE ("Window 0x%.8X not found in lists", hwnd, 0, 0, 0));

	hi->hwnd = NULL;
	IcWindowCleanup ();
	return;
failure:
	return;
}

int IcWindowIsSet (parerror_t *pe, HWND hwnd)
{
	DERR st;
	hwndicon_t *hi;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	hi = IcWindowFind (hwnd);

	if (hi)
		return 1;
	return 0;
failure:
	return 0;
}

void IcWindowReassertIcon (parerror_t *pe, HWND hwnd)
{
	DERR st;
	hwndicon_t *hi;

	fail_if (!hwnd, 0, PE_BAD_PARAM (1));

	hi = IcWindowFind (hwnd);
	fail_if (!hi, 0, PE ("Window 0x%.8X does not have a set icon to reassert", hwnd, 0, 0, 0));

	SendMessage (hi->hwnd, WM_SETICON, ICON_BIG, (LPARAM)hi->big);
	SendMessage (hi->hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hi->small);

	return;
failure:
	return;
}






			








#if 0


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define bail_if(cond, fmt, arg) do { if (cond) { printf (fmt, arg); goto fail; } } while (0)

int main (int argc, char **argv)
{
	HWND h;
	HICON hicon, newicon, dupe;
	ICONINFO ii;
	HDC memdc, screen;
	HBITMAP fuck;
	int st;
	BYTE *bits;
	BITMAPINFO bmi = {0};
	int x;



	h = (HWND)0xE0832; //strtoul (argv[1], NULL, 16);

	hicon = (HICON)SendMessage (h, WM_GETICON, ICON_BIG, 0);
	bail_if (!hicon, "no hicon, %i\n", GetLastError ());
	printf ("icon is %x %i\n", hicon, hicon);

	dupe = CopyIcon (hicon);
	
	if (!dupe)
	{
		/*char name[256];
		st = GetClassName (h, name, 255);
		name[255] = '\0';
		bail_if (!st, "no get class\n");*/

		hicon = (HICON)GetClassLong (h, GCL_HICON);
		printf ("new icon is %x\n", hicon);
		dupe = CopyIcon (hicon);
		bail_if (!dupe, "Still couldn't dupe icon\n", 0);
	}


	st = GetIconInfo (dupe, &ii);
	bail_if (!st, "No get info %i\n", GetLastError ());
	printf ("II is: %x %x\n", ii.hbmColor, ii.hbmMask);

	screen = GetDC (NULL);
	bail_if (!screen, "No screen DC\n", 0);

	memdc = CreateCompatibleDC (screen);
	bail_if (!memdc, "No mem Dc", 0);

	ReleaseDC (NULL, screen);

	fuck = SelectObject (memdc, ii.hbmColor);
	bail_if (!fuck, "Couldn't select %i\n", GetLastError ());

	//BitBlt (memdc, 0, 0, 32, 16, memdc, 0, 16, BLACKNESS);

	bmi.bmiHeader.biSize = sizeof (bmi.bmiHeader);
	bmi.bmiHeader.biWidth = 32;
	bmi.bmiHeader.biHeight = 32;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biCompression = BI_RGB;

	bits = malloc (3 * 32 * 32 + 1);
	bits[3 * 32 * 32] = 0xFE;

	st = GetDIBits (memdc, ii.hbmColor, 0, 32, bits, &bmi, DIB_RGB_COLORS);
	bail_if (!st, "no get di bits\n", 0);

	bail_if (bits[3 * 32 * 32] != 0xFE, "Ran over end of array\n", 0);

	for (x=0;x<32 * 32;x++)
	{
		BYTE *t = (bits + (x * 3));

		if (t[0] == 0xFF && t[1] == 0 && t[2] == 0)
		{
			t[0] = rand () % 255;
			t[1] = rand () % 255;
			t[2] = rand () % 255;
		}
	}

	st = SetDIBits (memdc, ii.hbmColor, 0, 32, bits, &bmi, DIB_RGB_COLORS);
	bail_if (!st, "Couldn't set bits back\n", 0);

	SelectObject (memdc, fuck);
	DeleteDC (memdc);

	newicon = CreateIconIndirect (&ii);

	SendMessage (h, WM_SETICON, ICON_BIG, (LPARAM)newicon);

	


	return 0;
fail:
	return 1;
}

#endif