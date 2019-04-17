/* Copyright 2006 wParam, licensed under the GPL */

#include <windows.h>
#include "instrument.h"

static instlist_t *InList = NULL;


static instimpl_t *InTypes[] = 
{
	NULL,
	&WhWindowImplementation,
	&FhFunctionImplementation,
	&FhAddressImplementation,
};
#define MAX_IMPL (sizeof (InTypes) / sizeof (instimpl_t *))

instlist_t *InCreateListEntry (instrument_t *inst, action_t *actions)
{
	int size;
	instlist_t *out = NULL;
	BYTE *ptr;
	int st;

	size = sizeof (instlist_t) + (sizeof (void *) * InTypes[inst->type]->numrumtime) + inst->size + actions->size;

	out = IsmMalloc (size);
	fail_if (!out, 0, DP (ERROR, "Memory alloc of %i bytes failed\n", size));

	memset (out, 0, size);

	ptr = out->data;
	memcpy (ptr, inst, inst->size);
	out->inst = (instrument_t *)ptr;

	ptr += inst->size;

	memcpy (ptr, actions, actions->size);
	out->actions = (action_t *)ptr;

	ptr += actions->size;

	out->runtime = (void **)ptr;

	return out;

failure:

	if (out)
		IsmFree (out);

	return NULL;

}

int InSetInstrument (instrument_t *inst, action_t *actions)
{
	instlist_t *item = NULL;
	int res;
	int st;

	//sanity checks
	fail_if (!IsmCheckMode (MODE_AGENT), 0, DP (ERROR, "Instruments can only be set by an agent\n"));
	fail_if (inst->type >= MAX_IMPL || !InTypes[inst->type], 0, DP (ERROR, "Unknown instrument type %i or type not implemented\n", inst->type));

	item = InCreateListEntry (inst, actions);
	fail_if (!item, 0, DP (ERROR, "List item create failed\n"));

	res = InTypes[inst->type]->set (item);
	fail_if (!res, 0, DP (ERROR, "Instrumentation failure\n"));
	
	//don't fail below here.

	UtilListInsert (&InList, item);



	return 1;

failure:
	if (item)
		IsmFree (item);

	return 0;


}


instlist_t *InSearchInstruments (int (*compare)(instlist_t *, void *), void *data)
{
	instlist_t *walk;

	walk = InList;
	while (walk)
	{
		if (compare (walk, data))
			return walk;

		walk = walk->next;
	}

	return NULL;
}

instlist_t *InFindInstrument (instrument_t *inst)
{
	instlist_t *walk = InList;

	while (walk)
	{
		if (inst->size == walk->inst->size && memcmp (walk->inst, inst, inst->size) == 0)
			return walk;

		walk = walk->next;
	}

	return NULL;
}

int InUnsetInstrument (instrument_t *inst)
{
	instlist_t *item;
	int st;

	fail_if (!IsmCheckMode (MODE_AGENT), 0, DP (ERROR, "Instruments can only be unset by an agent\n"));

	item = InFindInstrument (inst);
	fail_if (!item, 0, DP (WARN, "Could not find specified instrument to unset\n"));

	UtilListRemove (&InList, item);

	st = InTypes[item->inst->type]->unset (item);
	fail_if (!st, 0, DP (ERROR, "Could not un instrument\n"));

	IsmFree (item);

	return 1;


failure:

	//nothing here.  We can't free the item if it wasn't uninstrumented

	return 0;

}

instrument_t *InCreateWindowInstrument (HWND hwnd, UINT message, WPARAM wParam, WPARAM wMask, LPARAM lParam, LPARAM lMask, parerror_t *pe)
{
	instwindow_t *inst;
	instrument_t *out;
	int st;

	out = IsmMalloc (sizeof (instrument_t) + sizeof (instwindow_t));
	fail_if (!out, 0, PE_OUT_OF_MEMORY (sizeof (instrument_t) + sizeof (instwindow_t)));

	out->size = sizeof (instrument_t) + sizeof (instwindow_t);
	out->type = INSTRUMENT_WINDOW;

	inst = (instwindow_t *)out->data;

	inst->hwnd = hwnd;
	inst->message = message;
	inst->wParam = wParam;
	inst->wMask = wMask;
	inst->lParam = lParam;
	inst->lMask = lMask;

	return out;

failure:
	return NULL;
}


instrument_t *InCreateFunctionInstrument (char *lib, char *func, int numparams, int stdcall, int dofinalcode, parerror_t *pe)
{
	instfunction_t *inst;
	instrument_t *out;
	int size;
	int st;

	size = sizeof (instrument_t) + sizeof (instfunction_t) + strlen (lib) + 1 + strlen (func) + 1;
	size = (((size - 1) >> 2) + 1) << 2;

	fail_if (size > MAX_INSTRUMENT_SIZE, 0, PE ("Instrument too big (%i > %i)\n", size, MAX_INSTRUMENT_SIZE, 0, 0));

	out = IsmMalloc (size);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (size));

	out->type = INSTRUMENT_FUNCTION;
	out->size = size;

	inst = (instfunction_t *)out->data;

	inst->dofinalcode = dofinalcode;
	inst->numparams = numparams;
	inst->stdcall = stdcall;

	strcpy (inst->data, lib);

	inst->nameofs = strlen (lib) + 1;

	strcpy (inst->data + inst->nameofs, func);

	return out;
failure:
	return NULL;
}

instrument_t *InCreateAddressInstrument (void *addy, int numparams, int stdcall, int dofinalcode, parerror_t *pe)
{
	instaddress_t *insta;
	instrument_t *out;
	int size;
	int st;

	size = sizeof (instrument_t) + sizeof (instaddress_t);
	size = (((size - 1) >> 2) + 1) << 2;

	fail_if (size > MAX_INSTRUMENT_SIZE, 0, PE ("Instrument too big (%i > %i)\n", size, MAX_INSTRUMENT_SIZE, 0, 0));

	out = IsmMalloc (size);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (size));

	out->type = INSTRUMENT_ADDRESS;
	out->size = size;

	insta = (instaddress_t *)out->data;

	insta->dofinalcode = dofinalcode;
	insta->numparams = numparams;
	insta->stdcall = stdcall;
	
	insta->address = addy;

	return out;
failure:
	return NULL;
}








