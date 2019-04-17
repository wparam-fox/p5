/* Copyright 2006 wParam, licensed under the GPL */
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include "keyboard.h"


typedef struct pause_t
{
	struct pause_t *next;

	char *key;

	char *origkeys; //the string passed to (kb.monitor), used by kb.mondump
	char *line;

	int numvars; //the number of # or * chars in the key
	int vars[0];

} pause_t;

pause_t *KbPauseHead = NULL;
HHOOK KbPauseHook = NULL;
char *KbPauseBuffer = NULL;

unsigned int KbPauseMax = 127; //max length of pause buffer, set in stone when pause is enabled
unsigned int KbPauseKey = VK_PAUSE; //the char that triggers parsing
unsigned int KbPauseAbortKey = VK_ESCAPE; //the key that always aborts parsing
int KbPauseActive = 0;
int KbPausePermits = 0;
int KbPauseDrawing = 1;


#define VK_HASH 0x0A
#define VK_STAR 0x0B

pause_t *KbSearchMonitor (char *key)
{
	pause_t *walk;

	walk = KbPauseHead;
	while (walk)
	{
		if (strcmp (walk->key, key) == 0)
			return walk;

		walk = walk->next;
	}

	return NULL;
}

void KbAddMonitor (parerror_t *pe, char *sequence, char *line)
{
	DERR st;
	char **vks = NULL;
	char *key = NULL;
	int size, keystrokes, x;
	pause_t *p = NULL;
	char *temp;
	int numvars;

	fail_if (!PlIsMainThread (), 0, PE ("kb.monitor must run in the main thread", 0, 0, 0, 0));
	fail_if (!sequence, 0, PE_BAD_PARAM (1));

	//first split the sequence and calculate the key
	st = PlSplitString (sequence, &size, NULL, ' ');
	fail_if (!DERR_OK (st), st, PE ("PlSplitString failed, first half, 0x%.8X", st, 0, 0, 0));

	vks = PlMalloc (size);
	fail_if (!vks, 0, PE_OUT_OF_MEMORY (size));

	st = PlSplitString (sequence, &keystrokes, vks, ' ');
	fail_if (!DERR_OK (st), st, PE ("PlSplitString failed, second half, 0x%.8X", st, 0, 0, 0));

	key = PlMalloc (keystrokes + 1);
	fail_if (!key, 0, PE_OUT_OF_MEMORY (keystrokes + 1));

	numvars = 0;
	for (x=0;x<keystrokes;x++)
	{
		if (strcmp (vks[x], "#") == 0)
		{
			numvars++;
			key[x] = VK_HASH;
		}
		else if (strcmp (vks[x], "*") == 0)
		{
			numvars++;
			key[x] = VK_STAR;
		}
		else
		{
			key[x] = (BYTE)KbTranslateVirtualKey (vks[x]);
			fail_if (key[x] == VK_HASH || key[x] == VK_STAR, 0, PE ("Keys %i and %i cannot be used; they are reserved as # and *.", VK_HASH, VK_STAR, 0, 0));
		}

		fail_if (!key[x], 0, PE_STR ("Unknown key %s", COPYSTRING (vks[x]), 0, 0, 0));
	}

	key[x] = '\0'; //add null terminator

	//ok, we're done with vks now.
	PlFree (vks);
	vks = NULL;

	fail_if (strlen (key) > KbPauseMax, 0, PE ("Length of key exceeds max key length (%i)", KbPauseMax, 0, 0, 0));

	//check to see if a pause with the given key already exists.  If it does, just replace
	//the line and end.
	p = KbSearchMonitor (key);
	if (p)
	{
		//get rid of key, we don't need it anymore
		PlFree (key);
		key = NULL;

		//sanity check
		fail_if (p->numvars != numvars, 0, PE ("Consistency failure forming new key", 0, 0, 0, 0));

		temp = PlMalloc (strlen (line) + 1);
		fail_if (!temp, 0, p = NULL ; PE_OUT_OF_MEMORY (strlen (line) + 1));
		//note the p = NULL in the fail line
		strcpy (temp, line);

		//all good.
		PlFree (p->line);
		p->line = temp;

		return;
	}

	//ok, so allocate a new p
	p = PlMalloc (sizeof (pause_t) + (sizeof (int) * numvars));
	fail_if (!p, 0, PE_OUT_OF_MEMORY (sizeof (pause_t) + (sizeof (int) * numvars)));
	PlMemset (p, 0, sizeof (pause_t) + (sizeof (int) * numvars));

	p->key = key;
	key = NULL; //prevent a double free on failure

	p->numvars = numvars;

	p->line = PlMalloc (strlen (line) + 1);
	fail_if (!p->line, 0, PE_OUT_OF_MEMORY (strlen (line) + 1));
	strcpy (p->line, line);

	p->origkeys = PlMalloc (strlen (sequence) + 1);
	fail_if (!p->origkeys, 0, PE_OUT_OF_MEMORY (strlen (line) + 1));
	strcpy (p->origkeys, sequence);
	
	//ok, we are successful.  Link and return.

	p->next = KbPauseHead;
	KbPauseHead = p;

	return;

failure:

	if (vks)
		PlFree (vks);

	if (key)
		PlFree (key);

	if (p)
	{
		if (p->line)
			PlFree (p->line);
		if (p->key)
			PlFree (p->key);
		if (p->origkeys)
			PlFree (p->origkeys);

		PlFree (p);
	}

	return;
}


void KbRemoveMonitor (parerror_t *pe, char *sequence)
{
	DERR st;
	char **vks = NULL;
	char *key = NULL;
	int size, keystrokes, x;
	pause_t **walk, *target;

	fail_if (!PlIsMainThread (), 0, PE ("kb.unmonitor must run in the main thread", 0, 0, 0, 0));
	fail_if (!sequence, 0, PE_BAD_PARAM (1));

	//first split the sequence and calculate the key
	st = PlSplitString (sequence, &size, NULL, ' ');
	fail_if (!DERR_OK (st), st, PE ("PlSplitString failed, first half, 0x%.8X", st, 0, 0, 0));

	vks = PlMalloc (size);
	fail_if (!vks, 0, PE_OUT_OF_MEMORY (size));

	st = PlSplitString (sequence, &keystrokes, vks, ' ');
	fail_if (!DERR_OK (st), st, PE ("PlSplitString failed, second half, 0x%.8X", st, 0, 0, 0));

	key = PlMalloc (keystrokes + 1);
	fail_if (!key, 0, PE_OUT_OF_MEMORY (keystrokes + 1));

	for (x=0;x<keystrokes;x++)
	{
		if (strcmp (vks[x], "#") == 0)
			key[x] = VK_HASH;
		else if (strcmp (vks[x], "*") == 0)
			key[x] = VK_STAR;
		else
			key[x] = (BYTE)KbTranslateVirtualKey (vks[x]);

		fail_if (!key[x], 0, PE_STR ("Unknown key %s", COPYSTRING (vks[x]), 0, 0, 0));
	}

	key[x] = '\0'; //add null terminator

	//ok, we're done with vks now.
	PlFree (vks);
	vks = NULL;

	target = NULL;
	walk = &KbPauseHead;
	while (*walk)
	{
		if (strcmp ((*walk)->key, key) == 0)
		{
			//this is the one, remove it from the list.
			target = *walk;
			*walk = (*walk)->next;
			break;
		}

		walk = &(*walk)->next;
	}

	fail_if (!target, 0, PE ("No such monitor found in list.", 0, 0, 0, 0));

	PlFree (target->origkeys);
	PlFree (target->key);
	PlFree (target->line);
	PlFree (target);

	PlFree (key);

	return;

failure:

	if (vks)
		PlFree (vks);

	if (key)
		PlFree (key);

}


void KbRemoveAllMonitors (parerror_t *pe)
{
	DERR st;
	pause_t *p, *temp;

	fail_if (!PlIsMainThread (), 0, PE ("kb.resetmonitor must run in the main thread", 0, 0, 0, 0));

	p = KbPauseHead;
	while (p)
	{
		temp = p->next;

		PlFree (p->key);
		PlFree (p->line);
		PlFree (p->origkeys);
		PlFree (p);

		p = temp;
	}

	KbPauseHead = NULL;

failure:
	return;
}

void KbPrintMonitors (parerror_t *pe)
{
	DERR st;
	pause_t *p;

	fail_if (!PlIsMainThread (), 0, PE ("kb.dumpmonitor must run in the main thread", 0, 0, 0, 0));

	p = KbPauseHead;
	while (p)
	{
		PlPrintf ("\"%s\":%s\n", p->origkeys, p->line);

		p = p->next;
	}

failure:
	return;
}





#define PD_NORMAL 1
#define PD_ACCEPTED 2
#define PD_REJECTED 3

int KbPauseCompareKeys (char *key, char *test, int *vars)
{
	char *k, *t;

	k = key;
	t = test;

	while (1)
	{
		if (!*k && !*t)
			return PD_ACCEPTED; //we hit a match

		if (!*k)
			return PD_REJECTED; //typed string is longer than the key, must not be valid

		if (!*t)
			return PD_NORMAL; //matches so far, so all good.

		if (*k == VK_STAR)
		{
			//Star means "match all characters up to the character after the star

			if (!*(k + 1) || *(k + 1) == VK_STAR || *(k + 1) == VK_HASH)
				return PD_REJECTED; //star must have a character after it

			*vars = (t - test) & 0xFFFF; //mark the start

			while (*t && *t != *(k + 1))
				t++;

			if (!*t) //a partial match, we're still waiting for the end char
				return PD_NORMAL;

			//ok, so *t now points to the end character, mark vars.
			//the hi word of vars is the length, and it's equal to
			//the position of t now minus the position when we 
			//encountered the star.  This line is computing that.
			*vars |= (((t - test) - *vars) << 16) & 0xFFFF0000;

			//so now T points to the char after the * string.  Increment K
			//so that it will point to the same character and continue the
			//comparison.  Also don't forget to increment vars.

			k++;
			vars++;
			continue;
		}

		if (*k == VK_HASH)
		{
			//hash means "match integers until we get a non-integer.
			//At least one char must follow # so that there is something
			//for it to match against.

			if (!*(k + 1) || *(k + 1) == VK_STAR || *(k + 1) == VK_HASH)
				return PD_REJECTED; //malformed key

			*vars = (t - test) & 0xFFFF; //mark start

			while (*t && (*t >= '0' && *t <= '9'))
				t++;

			if (!*t)
				return PD_NORMAL;

			//same as in the * case
			*vars |= (((t - test) - *vars) << 16) & 0xFFFF0000;

			//again, same as star, make K point to the char after #, increment vars, continue
			k++;
			vars++;
			continue;
		}

		//all other cases the chars must match
		if (*k != *t)
			return PD_REJECTED;

		k++;
		t++;
	}

	return -1;
}


//returns one of the above.  If it returns accepted or rejected, pause
//should be deactivated.  Match, if set, will point to the pause_t
//that should be executed.  I'm pretty sure *match != NULL and a return
//!= PD_ACCEPTED is an error.  We do this because the parsing needs to
//happen AFTER the drawing takes place, and the drawing doesn't happen
//until after this function returns.
int KbPauseInput (unsigned int vk, pause_t **match)
{
	BYTE vkbuffer[2];
	pause_t *p;
	int res;
	//int count;
	int *vars = NULL;

	if (!vk) //this is an error
		return PD_REJECTED;

	if (vk == KbPauseAbortKey)
		return PD_REJECTED;

	if (strlen (KbPauseBuffer) >= KbPauseMax) //it should never be greater than
		return PD_REJECTED;

	if (vk == KbPauseKey)
		return PD_NORMAL; //ignore this when we're already active.  Pressing pause twice by mistake
						  //should not mean that the whole things is aborted.  (That's what the abort key is for)

	vkbuffer[0] = (BYTE)vk;
	vkbuffer[1] = '\0';

	strcat (KbPauseBuffer, vkbuffer);

	//we have the updated buffer, look for a match
	p = KbPauseHead;
	while (p)
	{

		res = KbPauseCompareKeys (p->key, KbPauseBuffer, p->vars);

		if (res == PD_ACCEPTED) //direct hit
		{
			*match = p;
			return PD_ACCEPTED;
		}

		if (res == PD_NORMAL) //partial match, more chars needed
		{
			*match = NULL;
			return PD_NORMAL;
		}

		//if res == PD_REJECTED, keep looking

		p = p->next;
	}

	//got down here, no matches or partials, must be a reject
	return PD_REJECTED;
}


void KbPauseDraw (char *string, int res)
{
	HDC hdc = NULL;
	HFONT oldfont = NULL, myfont;
	char *fmt = NULL;
	DERR st;
	RECT r = {0};
	int sy;
	int x;
	char *str;
	char buffer[40];

	sy = GetSystemMetrics (SM_CXSCREEN);
	fail_if (!sy, 0, "Could not get screen width\n");

	hdc = GetDC (NULL);
	fail_if (!hdc, GetLastError (), fmt = "GetDC failed, %i\n");

	myfont = GetStockObject (ANSI_FIXED_FONT);
	fail_if (!myfont, GetLastError (), fmt = "GetStockObject (ANSI_FIXED_FONT) failed, %i\n");

	oldfont = SelectObject (hdc, myfont);
	fail_if (!oldfont, GetLastError (), fmt = "SelectObject failed, %i\n");

	if (res == PD_NORMAL)
		st = SetTextColor (hdc, RGB (255, 255, 255)); //white
	else if (res == PD_ACCEPTED)
		st = SetTextColor (hdc, RGB (0, 255, 0)); //green
	else if (res == PD_REJECTED)
		st = SetTextColor (hdc, RGB (255, 0, 0)); //red
	else
		fail_if (TRUE, res, fmt = "Somebody returned an invalid result code %i\n");
	fail_if (st == CLR_INVALID, GetLastError (), fmt = "SetTextColor failed, %i\n");

	st = SetBkColor (hdc, RGB (0, 0, 0));
	fail_if (!st, GetLastError (), fmt = "SetBkColor failed, %i\n");

	sy -= 5;
	for (x=strlen (string) - 1;x >= 0;x--)
	{
		//grab the string value of the key, or make one up
		str = KbUntranslateVirtualKey (((unsigned char *)string)[x]);
		if (str)
		{
			PlSnprintf (buffer, 39, strlen (str) == 1 ? "%s" : "[%s]", str);
			buffer[39] = '\0';
		}
		else
		{
			PlSnprintf (buffer, 39, "[~%i]", ((unsigned char *)string)[x]);
			buffer[39] = '\0';
		}

		memset (&r, 0, sizeof (RECT));
		//get the width
		st = DrawText (hdc, buffer, -1, &r, DT_CALCRECT | DT_NOCLIP | DT_NOPREFIX | DT_LEFT | DT_SINGLELINE);
		fail_if (!st, GetLastError (), fmt = "DrawText failed for getting the width, %i\n");

		sy = sy - (r.right - r.left);
		r.left += sy;
		r.right += sy;
		r.top += 5;
		r.bottom += 5;

		st = BitBlt (hdc, r.left - 8, r.top - 2, r.right - r.left + 10, r.bottom - r.top + 4, NULL, 0, 0, BLACKNESS);
		fail_if (!st, GetLastError (), fmt = "BitBlt failed, %i");

		st = DrawText (hdc, buffer, -1, &r, DT_NOCLIP | DT_NOPREFIX | DT_LEFT | DT_SINGLELINE);
		fail_if (!st, GetLastError (), fmt = "DrawText failed for printing, %i\n");
	}

	//done
	SelectObject (hdc, oldfont);
	ReleaseDC (NULL, hdc);

	return;

failure:

	if (oldfont)
		SelectObject (hdc, oldfont);

	if (hdc)
		ReleaseDC (NULL, hdc);

	if (fmt)
		PlPrintf (fmt, st);

}


void KbPauseRunCommand (char *test, pause_t *p)
{
	char error[1024];
	parser_t *par = NULL;
	DERR st;
	char *fmt = NULL;
	char *buffer;
	int x;
	unsigned int size;
	char paramname[40];

	//yes, this isn't exactly the pinnacle of efficency, but currently I
	//don't want to open up parser.c and add a way to run a parse with
	//a define_t stuck on the head of the list (which is really how this
	//should probably work).  I also don't want to drop a bunch of crap
	//atoms in the root parser.  So, yes, we create a parser, bind the
	//params, run the line, and then delete the parser.
	//
	//Really, creating a new parser is nothing more than a few memory
	//allocations, and when you compare it to the amount of work required
	//to bind the params and run the line (which would be the same even
	//if I tweaked parser.c to make it more efficent), the amount of time
	//required to make a new parser is fairly low.
	//
	//One improvement that could be somewhat easily made would be to use
	//PlMainThreadParse if p->numvars is zero.  But at the moment I'm too
	//lazy to do that.

	st = PlCreateParser (&par);
	fail_if (!DERR_OK (st), st, fmt = "PlCreateParser failed, 0x%.8X\n");

	buffer = PlMalloc (KbPauseMax + 1);
	fail_if (!buffer, KbPauseMax + 1, fmt = "Out of memory on %i bytes\n");

	//bind the params
	for (x=0;x<p->numvars;x++)
	{
		PlSnprintf (paramname, 39, "$%i", x + 1);
		paramname[39] = '\0';

		size = (p->vars[x] >> 16) & 0xFFFF;
		fail_if (size > KbPauseMax, size, fmt = "Parameter size %i is larger than KbPauseMax.  (sanity failure)\n");

		memcpy (buffer, test + (p->vars[x] & 0xFFFF), size);
		buffer[size] = '\0';

		st = PlParserAddAtom (par, paramname, buffer, 0);
		fail_if (!DERR_OK (st), st, fmt = "PlParserAddAtom failed, 0x%.8X\n");
	}

	//free the buffer, we'll re-use the parameter for the output of the parse
	PlFree (buffer);
	buffer = NULL;

	x = 0;
	*error = '\0';
	st = PlRunParser (par, p->line, &buffer, &x, error, 1024);
	//Make sure you don't fail between now and the call to ParFree (buffer)
	//below.  (The failure code assumes buffer comes from the plugin heap.

	if (KbVerbose)
	{
		if (!DERR_OK (st))
			PlPrintf ("Monitor parse error: %s\n", error);
		if (x) //x should really be called "nesting"
			PlPrintf ("Monitor parse error: Syntax error in parse line\n");
		if (DERR_OK (st))
			PlPrintf ("Monitor parse output: %s\n", buffer);
	}

	if (buffer)
	{
		ParFree (buffer);
		buffer = NULL;
	}

	//ok, done.
	PlDestroyParser (par);

	return;

failure:

	if (buffer)
		PlFree (buffer);

	if (par)
		PlDestroyParser (par);

}








	



LRESULT CALLBACK KbPauseHookCallback (int code, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT *kbd = (KBDLLHOOKSTRUCT *)lParam;

	if (code < 0)
		return CallNextHookEx (KbPauseHook, code, wParam, lParam);

	if (kbd->flags & (1<<7)) //ignore released keys
		return CallNextHookEx (KbPauseHook, code, wParam, lParam);

	if (!KbPauseActive) //we're just waiting to get a pause key
	{
		if (kbd->vkCode == KbPauseKey)
		{
			*KbPauseBuffer = '\0';
			KbPauseActive = 1;

			//don't let the pause go through if there are no permits
			if (!KbPausePermits)
				return 1;

			//There is a permit, so remove one and then fall through to the
			//CallNextHookEx call
			KbPausePermits--;
		}
	}
	else
	{
		pause_t *match;
		int res;

		//Pause is active, process the key, and definitely eat it.
		res = KbPauseInput (kbd->vkCode, &match);

		if (KbPauseDrawing)
			KbPauseDraw (KbPauseBuffer, res);

		if (res == PD_REJECTED)
		{
			//just deactivate
			KbPauseActive = 0;
		}
		else if (res == PD_ACCEPTED)
		{
			//run and deactivate
			KbPauseRunCommand (KbPauseBuffer, match);
			KbPauseActive = 0;
		}
		//else PD_NORMAL or an error, in either case we do nothing
	

		return 1;
	}
		
	return CallNextHookEx (KbPauseHook, code, wParam, lParam);
}


void KbEnableMonitor (parerror_t *pe)
{
	DERR st;

	fail_if (KbPauseHook, 0, PE ("Monitor already enabled", 0, 0, 0, 0));
	fail_if (!PlIsMainThread (), 0, PE ("Monitor must be enabled in the main thread", 0, 0, 0, 0));

	KbPauseBuffer = PlMalloc (KbPauseMax + 1);
	fail_if (!KbPauseBuffer, 0, PE_OUT_OF_MEMORY (KbPauseMax + 1));
	PlMemset (KbPauseBuffer, 0, KbPauseMax + 1);

	KbPauseHook = SetWindowsHookEx (WH_KEYBOARD_LL, KbPauseHookCallback, instance, 0);
	fail_if (!KbPauseHook, 0, PE ("SetWindowsHookEx failed, %i", GetLastError (), 0, 0, 0));

	KbPauseActive = 0;

	return;
failure:
	if (KbPauseBuffer)
	{
		PlFree (KbPauseBuffer);
		KbPauseBuffer = NULL;
	}

	return;
}

int KbDisableMonitor (parerror_t *pe)
{
	DERR st;

	fail_if (!KbPauseHook, 0, PE ("Monitor not enabled", 0, 0, 0, 0));
	fail_if (!PlIsMainThread (), 0, PE ("Monitor must be disabled from the main thread", 0, 0, 0, 0));

	st = UnhookWindowsHookEx (KbPauseHook);
	fail_if (!st, 0, PE ("Failed to remove monitor hook, %i", GetLastError (), 0, 0, 0));

	KbPauseHook = NULL;

	PlFree (KbPauseBuffer);
	KbPauseBuffer = NULL;

	KbPauseActive = 0;

	return 1;
failure:
	return 0;
}


int KbPauseShutdown (parerror_t *pe)
{
	DERR st;

	KbRemoveAllMonitors (pe);
	fail_if (pe->error, 0, 0);

	if (KbPauseHook)
	{
		//this func fails if monitor isn't enabled, so only call it if it is
		KbDisableMonitor (pe);
		fail_if (pe->error, 0, 0);
	}

	return 1;
failure:
	return 0;
}



int KbSetPauseMax (int count, parerror_t *pe, int newval)
{
	int old;
	DERR st;

	if (count == 2) //function only got 1 parameter, don't access newcount
		return KbPauseMax;

	fail_if (KbPauseHook, 0, PE ("KbPauseMax cannot be set while the monitor is enabled", 0, 0, 0, 0));

	old = KbPauseMax;
	KbPauseMax = newval;

	return old;
failure:
	return 0;
}

int KbSetPauseKey (int count, int newval)
{
	int old;

	if (count == 1) //function only got 1 parameter, don't access newcount
		return KbPauseKey;

	old = KbPauseKey;
	KbPauseKey = newval;

	return old;
}

int KbSetPauseAbortKey (int count, int newval)
{
	int old;

	if (count == 1) //function only got 1 parameter, don't access newcount
		return KbPauseAbortKey;

	old = KbPauseAbortKey;
	KbPauseAbortKey = newval;

	return old;
}

int KbPermitPause (int count, int newval)
{
	int old;

	if (count == 1) //function only got 1 parameter, don't access newcount
		return KbPausePermits;

	old = KbPausePermits;
	KbPausePermits = newval;

	return old;
}

int KbSetPauseDrawing (int count, int newval)
{
	int old;

	if (count == 1) //function only got 1 parameter, don't access newcount
		return KbPauseDrawing;

	old = KbPauseDrawing;
	KbPauseDrawing = newval;

	return old;
}