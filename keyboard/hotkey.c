/* Copyright 2013 wParam, licensed under the GPL */


#include <windows.h>
#include "keyboard.h"


typedef struct
{
	int inuse;

	int mod;
	int vk;

	char *line;
} hotkey_t;

#define MAX_HOTKEYS 0x400
#define HOTKEY_STEP 10
hotkey_t *KbHotkeys = NULL;
int KbNumHotkeys = 0;

hotkey_t *KbFindFreeHotkey (void)
{
	int x;
	hotkey_t *temp;
	DERR st;

	for (x=0;x<KbNumHotkeys;x++)
	{
		if (!KbHotkeys[x].inuse)
			return KbHotkeys + x;
	}

	//need to reallocate
	
	fail_if (KbNumHotkeys + HOTKEY_STEP >= MAX_HOTKEYS, 0, 0);

	if (!KbHotkeys)
	{
		KbHotkeys = PlMalloc (sizeof (hotkey_t) * (KbNumHotkeys + HOTKEY_STEP));
		fail_if (!KbHotkeys, 0, 0);
	}
	else
	{
		temp = PlRealloc (KbHotkeys, sizeof (hotkey_t) * (KbNumHotkeys + HOTKEY_STEP));
		fail_if (!temp, 0, 0);
		KbHotkeys = temp;
	}

	KbNumHotkeys += HOTKEY_STEP;

	temp = KbHotkeys + x; //this is what we're going to return

	//but first, we need to set the newly allocated things to 0
	for (;x<KbNumHotkeys;x++)
	{
		PlMemset (KbHotkeys + x, 0, sizeof (hotkey_t));
	}

	return temp;

failure:
	//nothing to cleanup
	return NULL;
}


hotkey_t *KbFindExistingKey (int mod, int vk)
{
	int x;

	for (x=0;x<KbNumHotkeys;x++)
	{
		if (KbHotkeys[x].inuse)
		{
			if (KbHotkeys[x].mod == mod &&
				KbHotkeys[x].vk == vk)
				return KbHotkeys + x;
		}
	}

	return NULL;
}

LRESULT CALLBACK KbHotkeyCallback (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	parerror_t pe = {0};
	char *output = NULL;
	DERR st;
	int kbvk, kbmod;

	if (wParam >= MAX_HOTKEYS)
		return 0; //wtf?

	if (wParam >= (unsigned)KbNumHotkeys)
		return 0;

	if (!KbHotkeys[wParam].inuse) //again, wtf?
		return 0;

	if (!KbHotkeys[wParam].line)
		return 0;

	//Save these values, because if the command we parse is "kb.hotkey.del", the array
	//will have moved
	kbmod = KbHotkeys[wParam].mod;
	kbvk = KbHotkeys[wParam].vk;

	//ok.
	st = PlMainThreadParse (KbHotkeys[wParam].line, &output, &pe);
	if (DERR_OK (st))
	{
		if (KbVerbose)
			PlPrintf ("Hotkey %i{%i,%i} output: %s\n", wParam, kbmod, kbvk, output);

		if (output)
			ParFree (output);
		PlCleanupError (&pe);
	}
	else
	{
		char buffer[512];

		PlProcessError (st, buffer, 512, &pe);

		if (KbVerbose)
			PlPrintf ("Hotkey %i{%i,%i} error: %s\n", wParam, kbmod, kbvk, buffer);

		if (output)
			ParFree (output);
	}

	return 0;


}

int KbAddHotKey (parerror_t *pe, char *mods, int vk, char *line)
{
	DERR st;
	int mod = 0;
	hotkey_t *h = NULL;
	char *temp;

	fail_if (!PlIsMainThread (), 0, PE ("kb.hotkey must be run in the main thread", 0, 0, 0, 0));
	fail_if (!mods, 0, PE_BAD_PARAM (1));

	//calculate proper mods
	while (*mods)
	{
		switch (*mods)
		{
		case 'a': case 'A': mod |= MOD_ALT ; break;
		case 'c': case 'C': mod |= MOD_CONTROL ; break;
		case 's': case 'S': mod |= MOD_SHIFT ; break;
		case 'w': case 'W': mod |= MOD_WIN ; break;
		default:
			fail_if (TRUE, 0, PE ("Invalid character %c detected in modifier string", *mods, 0, 0, 0));
		}

		mods++;
	}

	//first, see if a hotkey with the given mod/vk is already registered.  If so,
	//just replace its line and return success.  If the realloc fails, then
	//this function fails.
	h = KbFindExistingKey (mod, vk);
	if (h)
	{
		temp = PlMalloc (strlen (line) + 1);
		fail_if (!temp, 0, h = NULL ; PE_OUT_OF_MEMORY (strlen (line) + 1));
		//note that the fail_if line above sets h to NULL, this is very important
		strcpy (temp, line);

		PlFree (h->line);
		h->line = temp;

		return h - KbHotkeys;
	}


	h = KbFindFreeHotkey ();
	fail_if (!h, 0, PE ("Unable to allocate new hotkey_t", 0, 0, 0, 0));

	h->mod = mod;
	h->vk = vk;

	h->line = PlMalloc (strlen (line) + 1);
	fail_if (!h->line, 0, PE_OUT_OF_MEMORY (strlen (line) + 1));
	strcpy (h->line, line);

	st = RegisterHotKey (PlGetMainWindow (), h - KbHotkeys, mod, vk);
	fail_if (!st, 0, PE ("RegisterHotKey failed, %i", GetLastError (), 0, 0, 0));
	//don't do anything after here; I don't want to have to unwind it on failure

	//all good, mark it in use and return.

	h->inuse = 1;

	return h - KbHotkeys;

failure:
	if (h)
	{
		if (h->line)
			PlFree (h->line);

		memset (h, 0, sizeof (hotkey_t));
	}

	return -1;
}


void KbRemoveHotkeyById (parerror_t *pe, int id)
{
	DERR st;
	hotkey_t *h;

	fail_if (!PlIsMainThread (), 0, PE ("kb.unhotkey must be run in the main thread", 0, 0, 0, 0));
	fail_if (id  < 0 || id >= KbNumHotkeys, 0, PE ("Given ID %i out of range", id, 0, 0, 0));
	fail_if (!KbHotkeys[id].inuse, 0, PE ("Hotkey %i is not defined", id, 0, 0, 0));

	h = KbHotkeys + id;

	st = UnregisterHotKey (PlGetMainWindow (), id);
	fail_if (!st, 0, PE ("UnregisterHotKey failed, %i", GetLastError (), 0, 0, 0));

	PlFree (h->line);
	memset (h, 0, sizeof (hotkey_t));

	return;
failure:

	return;
}

void KbRemoveHotkeyByKey (parerror_t *pe, char *mods, int vk)
{
	int mod = 0;
	DERR st;
	hotkey_t *h;

	fail_if (!PlIsMainThread (), 0, PE ("kb.unhotkey must be run in the main thread", 0, 0, 0, 0));
	fail_if (!mods, 0, PE_BAD_PARAM (1));

	//calculate proper mods
	while (*mods)
	{
		switch (*mods)
		{
		case 'a': case 'A': mod |= MOD_ALT ; break;
		case 'c': case 'C': mod |= MOD_CONTROL ; break;
		case 's': case 'S': mod |= MOD_SHIFT ; break;
		case 'w': case 'W': mod |= MOD_WIN ; break;
		default:
			fail_if (TRUE, 0, PE ("Invalid character %c detected in modifier string", *mods, 0, 0, 0));
		}

		mods++;
	}

	h = KbFindExistingKey (mod, vk);
	fail_if (!h || !h->inuse, 0, PE ("Hotkey {%i,%i} not found", mod, vk, 0, 0));

	st = UnregisterHotKey (PlGetMainWindow (), h - KbHotkeys);
	fail_if (!st, 0, PE ("UnregisterHotKey failed, %i", GetLastError (), 0, 0, 0));

	PlFree (h->line);
	memset (h, 0, sizeof (hotkey_t));

	return;

failure:
	return;
}


int KbRemoveAllHotkeys (parerror_t *pe)
{
	int x;
	DERR st;
	int failures = 0;

	fail_if (!PlIsMainThread (), 0, PE ("kb.unhotkey must be run in the main thread", 0, 0, 0, 0));

	for (x=0;x<KbNumHotkeys;x++)
	{
		if (KbHotkeys[x].inuse) //definitely want to ignore ones that aren't in use
		{
			st = UnregisterHotKey (PlGetMainWindow (), x);
			if (!st)
			{
				//unregister failed, add to the count and DON'T TOUCH
				//THE hotkey_t.
				failures++;
			}
			else
			{
				//unregister ok, so free the memory.
				PlFree (KbHotkeys[x].line);
				PlMemset (KbHotkeys + x, 0, sizeof (hotkey_t));
			}
		}
	}

	//ok.  So, at this point, if failures is nonzero, that means that at least
	//one hotkey failed to unregister, and thus we shouldn't free the array.
	//Everything that DID unregister has been properly cleaned up, so all should
	//be well to 'fail' at this point.
	fail_if (failures, 0, PE ("%i hotkey(s) failed to unregister.  (See kb.dumphotkeys)", failures, 0, 0, 0));

	//If we got here, that means that every entry in the KbHotkeys array was either
	//not in use or was successfully freed.  So, now we free the hotkeys array.

	PlFree (KbHotkeys);
	KbHotkeys = NULL;
	KbNumHotkeys = 0;

	return 1;

failure:
	return 0;
}

int KbForcedHotkeyCleanup (parerror_t *pe)
{
	int x;
	DERR st;

	fail_if (!PlIsMainThread (), 0, PE ("kb.unhotkey must be run in the main thread", 0, 0, 0, 0));

	for (x=0;x<KbNumHotkeys;x++)
	{
		if (KbHotkeys[x].inuse) //definitely want to ignore ones that aren't in use
		{
			st = UnregisterHotKey (PlGetMainWindow (), x);
			//whether it was ok or not, proceed.
			
			//unregister ok, so free the memory.
			PlFree (KbHotkeys[x].line);
			PlMemset (KbHotkeys + x, 0, sizeof (hotkey_t));
			
		}
	}

	PlFree (KbHotkeys);
	KbHotkeys = NULL;
	KbNumHotkeys = 0;

	return 1;

failure:
	return 0;
}

void KbDumpHotkeys (parerror_t *pe)
{
	int x;
	DERR st;

	fail_if (!PlIsMainThread (), 0, PE ("kb.dumphotkeys must be run in the main thread", 0, 0, 0, 0));

	PlPrintf (" id:mod:vk:command\n");

	for (x=0;x<KbNumHotkeys;x++)
	{
		if (KbHotkeys[x].inuse)
		{
			PlPrintf ("%.3i:%.2i:%.3i:%.60s\n", x, KbHotkeys[x].mod, KbHotkeys[x].vk, KbHotkeys[x].line);
		}
	}

failure:
	return;
}