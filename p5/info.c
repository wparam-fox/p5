/* Copyright 2006 wParam, licensed under the GPL */

#include <windows.h>
#include "p5.h"

#define INF_NAME_EXISTS					DERRCODE (INF_DERR_BASE, 0x01)
#define INF_INVALID_RESOURCE_ID			DERRCODE (INF_DERR_BASE, 0x02)
#define INF_OUT_OF_MEMORY				DERRCODE (INF_DERR_BASE, 0x03)
#define INF_NAME_NOT_FOUND				DERRCODE (INF_DERR_BASE, 0x04)
#define INF_RESOURCE_NOT_FOUND			DERRCODE (INF_DERR_BASE, 0x05)
#define INF_ZERO_LENGTH_RESOURCE		DERRCODE (INF_DERR_BASE, 0x06)
#define INF_LOAD_RESOURCE_FAILED		DERRCODE (INF_DERR_BASE, 0x07)
#define INF_LOCK_RESOURCE_FAILED		DERRCODE (INF_DERR_BASE, 0x08)
#define INF_INTERNAL_ERROR				DERRCODE (INF_DERR_BASE, 0x09)



char *InfErrors[] = 
{
	NULL,
	"INF_NAME_EXISTS",
	"INF_INVALID_RESOURCE_ID",
	"INF_OUT_OF_MEMORY",
	"INF_NAME_NOT_FOUND",
	"INF_RESOURCE_NOT_FOUND",
	"INF_ZERO_LENGTH_RESOURCE",
	"INF_LOAD_RESOURCE_FAILED",
	"INF_LOCK_RESOURCE_FAILED",
	"INF_INTERNAL_ERROR",

};

char *InfGetErrorString (int errindex)
{
	int numerrors = sizeof (InfErrors) / sizeof (char *);

	if (errindex >= numerrors || !InfErrors[errindex])
		return "INF_UNKNOWN_ERROR_CODE";

	return InfErrors[errindex];
}

typedef struct inf_t
{
	HMODULE instance;
	int resid;

	struct inf_t *next;

	char name[0];
} inf_t;

CRITICAL_SECTION InfGuard;
inf_t *InfListHead = NULL;


DERR InfInit (void)
{
	DERR st;

	st = PfSaneInitCriticalSection (&InfGuard);
	fail_if (!DERR_OK (st), st, 0);

	return PF_SUCCESS;
failure:
	return st;
}

void InfDenit (void)
{
	inf_t *i, *temp;

	DeleteCriticalSection (&InfGuard);

	i = InfListHead;
	while (i)
	{
		temp = i->next;
		PfFree (i);
		i = temp;
	}
	InfListHead = NULL;

}


//call with the lock already held
int InfNameExists (char *name)
{
	inf_t *i;

	i = InfListHead;
	while (i)
	{
		if (strcmp (name, i->name) == 0)
			return 1;

		i = i->next;
	}

	return 0;
}

DERR InfAddInfoBlock (char *name, HINSTANCE module, int resid)
{
	DERR st;
	inf_t *out = NULL;
	int len;


	P (&InfGuard);

	fail_if (InfNameExists (name), INF_NAME_EXISTS, 0);
	fail_if (resid & 0xFFFF0000, INF_INVALID_RESOURCE_ID, 0);

	len = sizeof (inf_t) + strlen (name) + 1;
	out = PfMalloc (len);
	fail_if (!out, INF_OUT_OF_MEMORY, DI (len));
	memset (out, 0, len);

	out->instance = module;
	out->resid = resid;
	strcpy (out->name, name);

	//link it; we're done
	out->next = InfListHead;
	InfListHead = out;

	V (&InfGuard);

	return PF_SUCCESS;

failure:

	if (out)
		PfFree (out);

	V (&InfGuard);

	return st;
}

void InfRemoveInfoBlock (char *name)
{
	inf_t **walk, *temp;

	P (&InfGuard);

	walk = &InfListHead;
	while (*walk)
	{
		//look for a name match, kill if so
		if (strcmp (name, (*walk)->name) == 0)
		{
			temp = *walk;
			*walk = (*walk)->next;
			PfFree (temp);

			break; //don't return, stick with single point of exit from lock block
		}

		walk = &(*walk)->next;
	}

	//whether it was ok or not, we're done.
	//it's no big deal if a plugin leaves a stray entry; the FindResource
	//functions will (I think) handle the invalid HINSTANCE case, and I
	//don't want to complicate plugin denit routines with potential failures
	//of this function.

	V (&InfGuard);
}

DERR InfGetInfoText (char *name, char **text)
{
	DERR st;
	char *out = NULL;
	HRSRC res;
	HGLOBAL loadedres;
	int len;
	void *data;
	inf_t *i;

	P (&InfGuard);

	i = InfListHead;
	while (i)
	{
		if (strcmp (name, i->name) == 0)
			break;

		i = i->next;
	}
	fail_if (!i, INF_NAME_NOT_FOUND, 0);

	res = FindResource (i->instance, MAKEINTRESOURCE (i->resid), "INFO");
	fail_if (!res, INF_RESOURCE_NOT_FOUND, DIGLE);

	len = SizeofResource (i->instance, res);
	fail_if (!len, INF_ZERO_LENGTH_RESOURCE, DIGLE);

	loadedres = LoadResource (i->instance, res);
	fail_if (!loadedres, INF_LOAD_RESOURCE_FAILED, DIGLE);

	data = LockResource (loadedres);
	fail_if (!data, INF_LOCK_RESOURCE_FAILED, 0);

	out = PfMalloc (len + 1);
	fail_if (!out, INF_OUT_OF_MEMORY, DI (len + 1));

	memcpy (out, data, len);
	out[len] = '\0';

	*text = out;

	V (&InfGuard);

	return PF_SUCCESS;

failure:

	//res, loadedres, and data are all just pointers to various points within
	//the process' image, so no freeing is necessary.


	if (out)
		PfFree (out);

	V (&InfGuard);

	return st;
}


DERR InfGetInfoNameList (char ***output)
{
	char **out = NULL;
	char **walk;
	char *c;
	int count;
	int len, totallen;
	int temp;
	inf_t *i;
	DERR st;

	P (&InfGuard);

	len = 0;
	count = 0;
	i = InfListHead;
	while (i)
	{
		count++;
		len += strlen (i->name) + 1;

		i = i->next;
	}

	totallen = sizeof (char *) * (count + 1) + len;
	out = PfMalloc (totallen);
	fail_if (!out, INF_OUT_OF_MEMORY, DI (totallen));

	c = (char *)out + (sizeof (char *) * (count + 1));
	walk = out;

	i = InfListHead;
	while (i)
	{
		*walk = c;
		walk++;

		temp = strlen (i->name) + 1;
		fail_if (temp > len, INF_INTERNAL_ERROR, DI (1));
		strcpy (c, i->name);
		len -= temp;
		c += temp;
		
		
		i = i->next;
	}

	*walk = NULL; //the last in the list, NULL signals the end.

	fail_if (walk - out != count, INF_INTERNAL_ERROR, DI (2));
	fail_if (len != 0, INF_INTERNAL_ERROR, DI (3));
	fail_if (c - ((char *)out) != totallen, INF_INTERNAL_ERROR, DI (4));

	//ok, done.

	*output = out;

	V (&InfGuard);

	return PF_SUCCESS;
failure:

	if (out)
		PfFree (out);

	V (&InfGuard);

	return st;
}







//////////////// Script functions///////////////////////////


#include "resource.h"



char *InfScriptStr (parerror_t *pe, char *name)
{
	DERR st;
	char *text = NULL;
	char *out = NULL;

	fail_if (!name, 0, PE_BAD_PARAM (1));

	st = InfGetInfoText (name, &text);
	fail_if (!DERR_OK (st), st, PE ("InfGetInfoText failed; returned {%s,%i}", PfDerrString (st), GETDERRINFO (st), 0, 0));

	out = ParMalloc (strlen (text) + 1);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (strlen (text) + 1));

	strcpy (out, text);

	PfFree (text);

	return out;

failure:

	if (text)
		PfFree (text);

	if (out)
		ParFree (out);

	return NULL;
}

void InfScriptBox (parerror_t *pe, char *name)
{
	DERR st;
	char *text = NULL;

	fail_if (!name, 0, PE_BAD_PARAM (1));

	st = InfGetInfoText (name, &text);
	fail_if (!DERR_OK (st), st, PE ("InfGetInfoText failed; returned {%s,%i}", PfDerrString (st), GETDERRINFO (st), 0, 0));

	MessageBox (NULL, text, name, 0);

	PfFree (text);

	return;

failure:

	if (text)
		PfFree (text);

}

void InfScriptList (parerror_t *pe)
{
	char **list = NULL;
	DERR st;
	char *buffer = NULL;
	int len;
	int x;

	st = InfGetInfoNameList (&list);
	fail_if (!DERR_OK (st), st, PE ("Could not get Info topic list, {%s,%i}", PfDerrString (st), GETDERRINFO (st), 0, 0));

	//count the length of all the topics.  Add 5 bytes for each one, to account for 
	//various things that might come between (spaces, tabs, \r\n, whatever)
	len = 0;
	x = 0;
	while (list[x])
	{
		len += strlen (list[x]) + 5;

		x++;
	}

	//100 bytes for "Info topic list:\r\n\r\n"
	buffer = PfMalloc (len + 100);
	fail_if (!buffer, 0, PE_OUT_OF_MEMORY (len + 100));
	*buffer = '\0';

	strcat (buffer, "Info topic list:\r\n\r\n");
	if (!list[0])
		strcat (buffer, "[none]");

	x = 0;
	while (list[x])
	{
		strcat (buffer, list[x]);

		if (list[x + 1])
			strcat (buffer, ",");

		if ((x + 1) % 7 == 0)
			strcat (buffer, "\r\n");
		else
			strcat (buffer, " ");

		x++;
	}

	MessageBox (NULL, buffer, "Info topic list", 0);

	PfFree (buffer);
	PfFree (list);

	return;
failure:

	if (buffer)
		PfFree (buffer);

	if (list)
		PfFree (list);

	return;
}




DERR InfAddGlobalCommands (parser_t *root)
{
	DERR st;

	st = ParAddFunction (root, "p5.info.str", "ses", InfScriptStr, "Get an info block as a string", "(p5.info.str [s:info name]) = [s:info block]", "The string returned may be long; it is probably better to avoid using this function unless there is some significant need.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.info.box", "ves", InfScriptBox, "Displays an info block in a message box", "(p5.info.box [s:info name]) = [v]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "p5.info.list", "ve", InfScriptList, "Lists the available info topics", "(p5.info.list) = [v]", "The list is displayed in a message box.  Use p5.info.box to see a topic.  The help window is probably the best way to browse info topics.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddAlias (root, "info", "ves", "p5.info.box");
	fail_if (!DERR_OK (st), st, 0);

	
	return PF_SUCCESS;
failure:
	return st;


}


