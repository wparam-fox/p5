/* Copyright 2006 wParam, licensed under the GPL */


#include <windows.h>
#include "wp_layout.h"



#ifdef LmMalloc
void * LmMalloc (int);
#else
#define LmMalloc malloc
#endif

#ifdef LmFree
void LmFree (void *);
#else
#define LmFree free
#endif

#ifdef LmMemcpy
void *LmMemcpy (void *dest, void *src, int len);
#else
#define LmMemcpy memcpy
#endif

typedef struct layman_t
{
	int len;
	int count;

	int data[0];
} layman_t;

#define LAYMAN_DEFAULT_SIZE 15
#define LMSIZE(a) (sizeof (layman_t) + (sizeof (int) * (a)))

#define fail_if(cond, status, other) if (cond) { st = status ; other ; /*printf ("line: %i\n", __LINE__);*/ goto failure ; }


layman_t *LmCreate (void)
{
	layman_t *out = NULL;
	int st;

	out = LmMalloc (LMSIZE (LAYMAN_DEFAULT_SIZE));
	fail_if (!out, 0, 0);

	out->count = 0;
	out->len = LAYMAN_DEFAULT_SIZE;

	return out;

failure:
	if (out)
		LmFree (out);

	return NULL;
}

void LmDestroy (layman_t *lm)
{
	if (lm)
		LmFree (lm);
}

int LmAddCommand (layman_t **lmio, int command)
{
	layman_t *temp, *lm = *lmio;
	int st;

	if (lm->count > lm->len)
		return 0; //this is an error.

	if (lm->count == lm->len)
	{
		temp = LmMalloc (LMSIZE (lm->len + LAYMAN_DEFAULT_SIZE));
		fail_if (!temp, 0, 0);
		LmMemcpy (temp, lm, LMSIZE (lm->len));
		LmFree (lm);
		lm = temp;
		lm->len += LAYMAN_DEFAULT_SIZE;
		*lmio = lm;
	}

	lm->data[lm->count] = command;
	lm->count++;

	return 1;
failure:
	return 0;
}




/*

  We have "groups".  Each group should know in advance how many members it will have.
  The structure is considered to have one root entry, everything else should be within
  that one root element.  That element can be an HWND if literally one child window is 
  needed.

  First is the "command" byte, or 0xFFBDzzzz where zzzz is the command index.
  Next, if applicable, will be an int specifying the command data (border widths,
    whatever.)  These bytes must be read and passed by the function
  Next are the children.

  So basically, at the start of each function, lm->data[ofs] should be the first
  element relevent to what the function is going to do.

  So, a layout that just has two windows split vertically would look like this:
  0xFFBD0006 //LmApplyVertical
  0xEDFBFFCE // param - 50% split
  0xFFBD0001 //LmApplyHwnd
  0x00AA8787 //some random HWND value
  0xFFBD0001 //LmApplyHwnd
  0x00898D83 //some other random hwnd value









*/

#define IS_INVALID_COMMAND(a) (((a) & 0xFFFF0000) != 0xFFBD0000)
#define GET_COMMAND(a) ((a) & 0xFFFF)
#define COMMAND_COUNT (sizeof (LmActionFuncs) / sizeof (void *))
#define MAKE_COMMAND(a) (((a) & 0xFFFF) | 0xFFBD0000)

#define IS_INVALID_PARAM(a) (((a) & 0xFFFF0000) != 0xEDFB0000)
#define MAKE_PARAM(a) (((a) & 0xFFFF) | 0xEDFB0000)
#define GET_PARAM(a) ((a) & 0xFFFF)

int LmAddGroupSingle (layman_t **lm)
{
	int st;

	st = LmAddCommand (lm, MAKE_COMMAND (0));
	fail_if (!st, 0, 0);

	return 1;
failure:
	return 0;
}

int LmAddHwnd (layman_t **lm, HWND hwnd)
{
	int st;

	st = LmAddCommand (lm, MAKE_COMMAND (1));
	fail_if (!st, 0, 0);
	st = LmAddCommand (lm, (int)hwnd);
	fail_if (!st, 0, 0);

	return 1;
failure:
	return 0;
}

int LmAddGroupBorder (layman_t **lm, int width)
{
	int st;

	st = LmAddCommand (lm, MAKE_COMMAND (2));
	fail_if (!st, 0, 0);
	st = LmAddCommand (lm, MAKE_PARAM (width));
	fail_if (!st, 0, 0);

	return 1;
failure:
	return 0;
}

int LmAddGroupMulti (layman_t **lm, int number)
{
	int st;

	st = LmAddCommand (lm, MAKE_COMMAND (3));
	fail_if (!st, 0, 0);
	st = LmAddCommand (lm, MAKE_PARAM (number));
	fail_if (!st, 0, 0);

	return 1;
failure:
	return 0;
}

int LmAddNothing (layman_t **lm)
{
	int st;

	st = LmAddCommand (lm, MAKE_COMMAND (4));
	fail_if (!st, 0, 0);

	return 1;
failure:
	return 0;
}

int LmAddGroupTopDelta (layman_t **lm, int delta)
{
	int st;

	st = LmAddCommand (lm, MAKE_COMMAND (5));
	fail_if (!st, 0, 0);
	st = LmAddCommand (lm, 0xED000000 | (1 << 16) | (((short)delta) & 0xFFFF));
	fail_if (!st, 0, 0);

	return 1;
failure:
	return 0;
}

int LmAddGroupRightDelta (layman_t **lm, int delta)
{
	int st;

	st = LmAddCommand (lm, MAKE_COMMAND (5));
	fail_if (!st, 0, 0);
	st = LmAddCommand (lm, 0xED000000 | (2 << 16) | (((short)delta) & 0xFFFF));
	fail_if (!st, 0, 0);

	return 1;
failure:
	return 0;
}

int LmAddGroupBottomDelta (layman_t **lm, int delta)
{
	int st;

	st = LmAddCommand (lm, MAKE_COMMAND (5));
	fail_if (!st, 0, 0);
	st = LmAddCommand (lm, 0xED000000 | (3 << 16) | (((short)delta) & 0xFFFF));
	fail_if (!st, 0, 0);

	return 1;
failure:
	return 0;
}

int LmAddGroupLeftDelta (layman_t **lm, int delta)
{
	int st;

	st = LmAddCommand (lm, MAKE_COMMAND (5));
	fail_if (!st, 0, 0);
	st = LmAddCommand (lm, 0xED000000 | (4 << 16) | (((short)delta) & 0xFFFF));
	fail_if (!st, 0, 0);

	return 1;
failure:
	return 0;
}

int LmAddGroupVerticalSplit (layman_t **lm, int leftwidth)
{
	int st;

	st = LmAddCommand (lm, MAKE_COMMAND (6));
	fail_if (!st, 0, 0);
	st = LmAddCommand (lm, MAKE_PARAM ((short)leftwidth));
	fail_if (!st, 0, 0);

	return 1;
failure:
	return 0;
}

int LmAddGroupHorizontalSplit (layman_t **lm, int topheight)
{
	int st;

	st = LmAddCommand (lm, MAKE_COMMAND (7));
	fail_if (!st, 0, 0);
	st = LmAddCommand (lm, MAKE_PARAM ((short)topheight));
	fail_if (!st, 0, 0);

	return 1;
failure:
	return 0;
}









static int LmApplySingle (layman_t *lm, RECT *inrect, int ofs, int *usedcount);
static int LmApplyHwnd (layman_t *lm, RECT *inrect, int ofs, int *usedcount);
static int LmApplyBorder (layman_t *lm, RECT *ir, int ofs, int *usedcount);
static int LmApplyGroup (layman_t *lm, RECT *ir, int ofs, int *usedcount);
static int LmApplyNothing (layman_t *lm, RECT *ir, int ofs, int *usedcount);
static int LmApplyEdgeDelta (layman_t *lm, RECT *ir, int ofs, int *usedcount);
static int LmApplyVerticalSplit (layman_t *lm, RECT *ir, int ofs, int *usedcount);
static int LmApplyHorizontalSplit (layman_t *lm, RECT *ir, int ofs, int *usedcount);



//Input rect is not to be modified.  ofs should be sanity checked.
//usedcount MUST be set if you return 1
static int (*LmActionFuncs[])(layman_t *, RECT *, int ofs, int *usedcount) = 
{
	LmApplySingle,
	LmApplyHwnd,
	LmApplyBorder,
	LmApplyGroup,
	LmApplyNothing,
	LmApplyEdgeDelta,
	LmApplyVerticalSplit,
	LmApplyHorizontalSplit,

};

//format:
//0xFFBD0000
//1 subcommand
static int LmApplySingle (layman_t *lm, RECT *inrect, int ofs, int *usedcount)
{
	int st;
	int count;

	fail_if (ofs >= lm->count, 0, 0);
	fail_if (IS_INVALID_COMMAND (lm->data[ofs]), 0, 0);
	fail_if (GET_COMMAND (lm->data[ofs]) >= COMMAND_COUNT, 0, 0);

	st = LmActionFuncs[GET_COMMAND (lm->data[ofs])] (lm, inrect, ofs + 1, &count);
	fail_if (!st, 0, 0);

	*usedcount = count + 1;

	return 1;
failure:
	return 0;
}


//format:
//0xFFBD0001
//0x[some HWND]
static int LmApplyHwnd (layman_t *lm, RECT *inrect, int ofs, int *usedcount)
{
	int st;
	HWND h;

	fail_if (ofs >= lm->count, 0, 0);

	h = (HWND)lm->data[ofs];

	if (!IsWindow (h))
		return 1; //silently don't do anything

	st = SetWindowPos (h, NULL, inrect->left, inrect->top, inrect->right - inrect->left, inrect->bottom - inrect->top, SWP_NOZORDER | SWP_FRAMECHANGED);
	fail_if (!st, 0, 0);

	*usedcount = 1;

	return 1;
failure:
	return 0;
}

//format:
//0xFFBD0002
//0xEDFBwwww wwww = border width
//1 subcommand
static int LmApplyBorder (layman_t *lm, RECT *ir, int ofs, int *usedcount)
{
	int st;
	RECT r;
	int border;
	int count;

	fail_if (ofs >= lm->count, 0, 0);
	fail_if (IS_INVALID_PARAM (lm->data[ofs]), 0, 0);

	border = GET_PARAM (lm->data[ofs]);

	ofs += 1; //don't forget to add 2 to usedcount

	//calculate r
	r.left = ir->left + border;
	r.right = ir->right - border;
	r.top = ir->top + border;
	r.bottom = ir->bottom - border;

	if (r.right < r.left)
		r.right = r.left = (ir->right - ir->left) / 2;
	if (r.bottom < r.top)
		r.top = r.bottom = (ir->bottom - ir->top) / 2;

	fail_if (ofs >= lm->count, 0, 0);
	fail_if (IS_INVALID_COMMAND (lm->data[ofs]), 0, 0);
	fail_if (GET_COMMAND (lm->data[ofs]) >= COMMAND_COUNT, 0, 0);
	st = LmActionFuncs[GET_COMMAND (lm->data[ofs])] (lm, &r, ofs + 1, &count);
	fail_if (!st, 0, 0);

	*usedcount = count + 2;

	return 1;

failure:
	return 0;
}

//format:
//0xFFBD0003
//0xEDFBxxxx xxxx = Number of children
//xxxx subcommands
static int LmApplyGroup (layman_t *lm, RECT *ir, int ofs, int *usedcount)
{
	int st;
	int count;
	int outcount;
	int numchildren;
	int x;

	fail_if (ofs >= lm->count, 0, 0);
	fail_if (IS_INVALID_PARAM (lm->data[ofs]), 0, 0);

	numchildren = GET_PARAM (lm->data[ofs]);

	outcount = 1;
	ofs++;

	for (x=0;x<numchildren;x++)
	{
		fail_if (ofs >= lm->count, 0, 0);
		fail_if (IS_INVALID_COMMAND (lm->data[ofs]), 0, 0);
		fail_if (GET_COMMAND (lm->data[ofs]) >= COMMAND_COUNT, 0, 0);
		st = LmActionFuncs[GET_COMMAND (lm->data[ofs])] (lm, ir, ofs + 1, &count);
		fail_if (!st, 0, 0);

		ofs += (1 + count);
		outcount += 1 + count;
	}

	*usedcount = outcount;

	return 1;
failure:
	return 0;
}

//format:
//0xFFBD0004
static int LmApplyNothing (layman_t *lm, RECT *ir, int ofs, int *usedcount)
{
	*usedcount = 0;
	return 1;
}

//format:
//0xFFBD0005
//0xEDsswwww wwww = delta ss = side {top, right, bottom, left}
//1 subcommand
static int LmApplyEdgeDelta (layman_t *lm, RECT *ir, int ofs, int *usedcount)
{
	int st;
	RECT r;
	int delta;
	int side;
	int count;

	fail_if (ofs >= lm->count, 0, 0);
	fail_if ((lm->data[ofs] & 0xFF000000) != 0xED000000, 0, 0);

	delta = (short)(GET_PARAM (lm->data[ofs]));
	side = (lm->data[ofs] >> 16) & 0xFF;

	ofs += 1; //don't forget to add 1 to usedcount

	LmMemcpy (&r, ir, sizeof (RECT));
	//modify r

	switch (side)
	{
	case 1:
		r.top += delta;
		if (r.top > r.bottom) r.top = r.bottom;
		break;
	case 2:
		r.right += delta;
		if (r.right < r.left) r.right = r.left;
		break;
	case 3:
		r.bottom += delta;
		if (r.bottom < r.top) r.bottom = r.top;
		break;
	case 4:
		r.left += delta;
		if (r.left > r.right) r.left = r.right;
		break;
	default:
		fail_if (TRUE, 0, 0);
	}

	fail_if (ofs >= lm->count, 0, 0);
	fail_if (IS_INVALID_COMMAND (lm->data[ofs]), 0, 0);
	fail_if (GET_COMMAND (lm->data[ofs]) >= COMMAND_COUNT, 0, 0);
	st = LmActionFuncs[GET_COMMAND (lm->data[ofs])] (lm, &r, ofs + 1, &count);
	fail_if (!st, 0, 0);

	*usedcount = count + 2;

	return 1;

failure:
	return 0;
}

//format:
//0xFFBD0006
//0xEDFBwwww wwww = left pane width.  If >0, absolute, if <0, percent of whole
//2 subcommands
static int LmApplyVerticalSplit (layman_t *lm, RECT *ir, int ofs, int *usedcount)
{
	int st;
	RECT r;
	int param;
	int count;
	int outcount;

	fail_if (ofs >= lm->count, 0, 0);
	fail_if (IS_INVALID_PARAM (lm->data[ofs]), 0, 0);

	param = (short)(GET_PARAM (lm->data[ofs]));

	ofs += 1;
	outcount = 1;

	//calculate r
	r.top = ir->top;
	r.bottom = ir->bottom;

	//change param to the width of the left pane.
	if (param < 0) //percentage
		param = ((ir->right - ir->left) * (param * -1)) / 100;
	//else param is already the positive absolute width

	if (param > (ir->right - ir->left) - 1)
		param = (ir->right - ir->left) - 1; //don't let it be > the total width

	//first child
	r.left = ir->left;
	r.right = ir->left + param;

	fail_if (ofs >= lm->count, 0, 0);
	fail_if (IS_INVALID_COMMAND (lm->data[ofs]), 0, 0);
	fail_if (GET_COMMAND (lm->data[ofs]) >= COMMAND_COUNT, 0, 0);
	st = LmActionFuncs[GET_COMMAND (lm->data[ofs])] (lm, &r, ofs + 1, &count);
	fail_if (!st, 0, 0);

	outcount += (count + 1);
	ofs += (count + 1);

	r.left = ir->left + param + 1;
	r.right = ir->right;

	fail_if (ofs >= lm->count, 0, 0);
	fail_if (IS_INVALID_COMMAND (lm->data[ofs]), 0, 0);
	fail_if (GET_COMMAND (lm->data[ofs]) >= COMMAND_COUNT, 0, 0);
	st = LmActionFuncs[GET_COMMAND (lm->data[ofs])] (lm, &r, ofs + 1, &count);
	fail_if (!st, 0, 0);

	outcount += (count + 1);
	ofs += (count + 1);

	*usedcount = outcount;

	return 1;

failure:
	return 0;
}

//format:
//0xFFBD0007
//0xEDFBwwww wwww = top pane height.  If >0, absolute, if <0, percent of whole
//2 subcommands
static int LmApplyHorizontalSplit (layman_t *lm, RECT *ir, int ofs, int *usedcount)
{
	int st;
	RECT r;
	int param;
	int count;
	int outcount;

	fail_if (ofs >= lm->count, 0, 0);
	fail_if (IS_INVALID_PARAM (lm->data[ofs]), 0, 0);

	param = (short)(GET_PARAM (lm->data[ofs]));

	ofs += 1;
	outcount = 1;

	//calculate r
	r.left = ir->left;
	r.right = ir->right;

	//change param to the width of the left pane.
	if (param < 0) //percentage
		param = (int)((ir->bottom - ir->top) * (((double)(param * -1)) / 100.0));
	//else param is already the positive absolute width

	if (param > (ir->bottom - ir->top) - 1)
		param = (ir->bottom - ir->top) - 1; //don't let it be > the total width

	//first child
	r.top = ir->top;
	r.bottom = ir->top + param;

	fail_if (ofs >= lm->count, 0, 0);
	fail_if (IS_INVALID_COMMAND (lm->data[ofs]), 0, 0);
	fail_if (GET_COMMAND (lm->data[ofs]) >= COMMAND_COUNT, 0, 0);
	st = LmActionFuncs[GET_COMMAND (lm->data[ofs])] (lm, &r, ofs + 1, &count);
	fail_if (!st, 0, 0);

	outcount += (count + 1);
	ofs += (count + 1);

	r.top = ir->top + param + 1;
	r.bottom = ir->bottom;

	fail_if (ofs >= lm->count, 0, 0);
	fail_if (IS_INVALID_COMMAND (lm->data[ofs]), 0, 0);
	fail_if (GET_COMMAND (lm->data[ofs]) >= COMMAND_COUNT, 0, 0);
	st = LmActionFuncs[GET_COMMAND (lm->data[ofs])] (lm, &r, ofs + 1, &count);
	fail_if (!st, 0, 0);

	outcount += (count + 1);
	ofs += (count + 1);

	*usedcount = outcount;

	return 1;

failure:
	return 0;
}


int LmApplyLayout (layman_t *lm, HWND hwnd)
{
	RECT r;
	int st;
	int ofs;

	st = GetClientRect (hwnd, &r);
	fail_if (!st, 0, 0);

	st = LmApplySingle (lm, &r, 0, &ofs);
	fail_if (!st, 0, 0);
	fail_if (ofs != lm->count, 0, 0);

	return 1;
failure:
	return 0;
}