/* Copyright 2008 wParam, licensed under the GPL */

#include <windows.h>
#include "p5.h"

#define WATCHER_OUT_OF_MEMORY			DERRCODE (WATCHER_DERR_BASE, 0x01)
#define WATCHER_NOT_FOUND				DERRCODE (WATCHER_DERR_BASE, 0x02)
#define WATCHER_NULL_NOT_WATCHED		DERRCODE (WATCHER_DERR_BASE, 0x03)
#define WATCHER_ARGUMENT_ERROR			DERRCODE (WATCHER_DERR_BASE, 0x04)
#define WATCHER_ALREADY_EXISTS			DERRCODE (WATCHER_DERR_BASE, 0x05)


char *WatErrors[] = 
{
	NULL,
	"WATCHER_OUT_OF_MEMORY",
	"WATCHER_NOT_FOUND",
	"WATCHER_NULL_NOT_WATCHED",
	"WATCHER_ARGUMENT_ERROR",
	"WATCHER_ALREADY_EXISTS",
};

char *WatGetErrorString (int errindex)
{
	int numerrors = sizeof (WatErrors) / sizeof (char *);

	if (errindex >= numerrors || !WatErrors[errindex])
		return "WATCHER_UNKNOWN_ERROR_CODE";

	return WatErrors[errindex];
}


typedef struct watcher_s
{
	char *plugin;
	char *type;

	void *value; /* for now this is all we'll hold. */

	struct watcher_s *next;
} watcher_t;

watcher_t *WatList = NULL;
CRITICAL_SECTION WatGuard; /* All access to WatList go through here */




DERR WatInit (void)
{
	DERR st;

	st = PfSaneInitCriticalSection (&WatGuard);
	fail_if (!DERR_OK (st), st, 0);

	return PF_SUCCESS;
failure:
	return st;
}

void WatDenit (void)
{
	watcher_t *walk, *temp;

	DeleteCriticalSection (&WatGuard);

	walk = WatList;
	while (walk)
	{
		temp = walk->next;
		PfFree (walk);
		walk = temp;
	}
}

/* Requires the watcher lock */
static watcher_t *WatFindWatcher (char *type, void *value)
{
	watcher_t *w;

	w = WatList;
	while (w)
	{
		if (strcmp (w->type, type) == 0 && w->value == value)
			return w;
		w = w->next;
	}

	return NULL;
}

DERR WatAdd (char *plugin, char *type, void *value)
{
	int len;
	DERR st;
	watcher_t *w = NULL;
	int inguard = 0;

	fail_if (!value, WATCHER_NULL_NOT_WATCHED, 0);
	fail_if (!plugin, DERRINFO (WATCHER_ARGUMENT_ERROR, 1), 0);
	fail_if (!type, DERRINFO (WATCHER_ARGUMENT_ERROR, 2), 0);

	len = sizeof (watcher_t) + strlen (plugin) + 1 + strlen (type) + 1;
	w = PfMalloc (len);
	fail_if (!w, WATCHER_OUT_OF_MEMORY, 0);

	w->plugin = ((char *)w) + sizeof (watcher_t);
	strcpy (w->plugin, plugin);
	w->type = ((char *)w) + sizeof (watcher_t) + strlen (plugin) + 1;
	strcpy (w->type, type);

	w->value = value;

	P (&WatGuard);
	inguard = 1;

	fail_if (WatFindWatcher (type, value), WATCHER_ALREADY_EXISTS, 0);

	w->next = WatList;
	WatList = w;
	V (&WatGuard);

	return PF_SUCCESS;
failure:
	if (w)
		PfFree (w);
	if (inguard)
		V (&WatGuard);

	return st;
}

/* NOTE: it's important that this function not fail unless the watcher is
 * not found.  A decent amount of code assumes that if WatCheck() passes,
 * WatDelete() will also pass. */
DERR WatDelete (char *type, void *value)
{
	DERR st;
	watcher_t **w, *temp;
	int inguard = 0;

	fail_if (!value, WATCHER_NULL_NOT_WATCHED, 0);
	fail_if (!type, DERRINFO (WATCHER_ARGUMENT_ERROR, 1), 0);

	P (&WatGuard);
	inguard = 1;

	w = &WatList;
	while (*w)
	{
		if (strcmp (type, (*w)->type) == 0 && (*w)->value == value)
			break;

		w = &(*w)->next;
	}

	/* !w shouldn't actually ever happen */
	fail_if (!w || !*w, WATCHER_NOT_FOUND, 0);

	temp = *w;
	*w = (*w)->next;

	PfFree (temp);

	V (&WatGuard);
	return PF_SUCCESS;
failure:
	if (inguard)
		V (&WatGuard);
	return st;
}

void WatDump (void)
{
	watcher_t *walk;

	ConWriteStringf ("Watchers currently known:\n");

	P (&WatGuard);

	walk = WatList;
	while (walk)
	{
		ConWriteStringf ("%s: (%s)0x%.8X\n", walk->plugin, walk->type, walk->value);
		walk = walk->next;
	}

	V (&WatGuard);
}

int WatCheck (char *type, void *value)
{
	int retval;

	if (!type)
		return 0;/* BAD programmer! */

	P (&WatGuard);

	retval = WatFindWatcher (type, value) ? 1 : 0;

	V (&WatGuard);

	return retval;
}


/******************      Parser functions      ************************/


static void WatScriptAddWatcher (parerror_t *pe, char *type, int value)
{
	DERR st;

	st = WatAdd ("script", type, (void *)value);
	fail_if (!DERR_OK (st), st, PE ("WatAdd returned {%s,%i}", PfDerrString (st), GETDERRINFO (st), 0, 0));
failure:
	return;
}

static void WatScriptDeleteWatcher (parerror_t *pe, char *type, int value)
{
	DERR st;

	st = WatDelete (type, (void *)value);
	fail_if (!DERR_OK (st), st, PE ("WatDelete returned {%s,%i}", PfDerrString (st), GETDERRINFO (st), 0, 0));
failure:
	return;
}

static int WatScriptCheckWatcher (parerror_t *pe, char *type, int value)
{
	DERR st;

	fail_if (!type, 0, PE_BAD_PARAM (1));
	return WatCheck (type, (void *)value);
failure:
	return 0;
}




DERR WatAddGlobalCommands (parser_t *root)
{
	DERR st;

	st = ParAddFunction (root, "p5.wat.add", "vesi", WatScriptAddWatcher, "Adds a new watcher value", "(p5.wat.add [s:type] [i:value]) = [v]", "Normally you won't need this.  See (info \"watcher\") for details.  The plugin name will be set to \"script\".");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "p5.wat.del", "vesi", WatScriptDeleteWatcher, "Deletes a watcher value", "(p5.wat.del [s:type] [i:value]) = [v]", "Normally you won't need this.  See (info \"watcher\") for details.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "p5.wat.dump", "v", WatDump, "Dumps watcher values to the console", "(p5.wat.dump) = [v]", "");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "p5.wat.check", "iesi", WatScriptCheckWatcher, "Checks whether a watcher exists or not", "(p5.wat.check [s:type] [i:value]) = [i:exists/not]", "Returns 1 if a watcher with the given name exists, 0 if not.");
	fail_if (!DERR_OK (st), st, 0);

	return PF_SUCCESS;
failure:
	return st;
}