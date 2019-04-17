/* Copyright 2008 wParam, licensed under the GPL */

#include <windows.h>
#include "govern.h"


HANDLE GbSection = NULL;
HANDLE GbMutex = NULL;

DWORD GbDefaultSize = 8192;
DWORD GbCurrentSize = -1;

typedef struct
{
	int buffersize;
	int nextfreeofs;
	int firsttypeofs;
} sectionroot_t;

typedef struct
{
	int nexttypeofs;
	int firstobjofs;
	char name[0];
} sectiontype_t;

void GbClearBuffer (void *buffer)
{
	((sectionroot_t *)buffer)->buffersize = GbDefaultSize;
	((sectionroot_t *)buffer)->firsttypeofs = 0;
	((sectionroot_t *)buffer)->nextfreeofs = sizeof (sectionroot_t);
}

int GbCreateHandles (parerror_t *pe, int exclusive)
{
	DERR st;
	char name[MAX_PATH];
	SECURITY_ATTRIBUTES sa;
	void *out = NULL;
	int doinit = 1;
	SECURITY_DESCRIPTOR sd;

	fail_if (!GovIsPlugin (), 0, PE ("GbCreateHandles will not operate outside of a plugin.  How did you get to this code anyhow?", 0, 0, 0, 0));
	fail_if (GbSection, 0, PE ("GbOpenHandles: Section already open\n", 0, 0, 0, 0));
	fail_if (GbMutex, 0, PE ("GbOpenHandles: Semaphore already open\n", 0, 0, 0, 0));

	st = InitializeSecurityDescriptor (&sd, SECURITY_DESCRIPTOR_REVISION);
	fail_if (!st, 0, PE ("Couldn't init security descriptor", GetLastError (), 0, 0, 0));
	st = SetSecurityDescriptorDacl (&sd, TRUE, NULL, FALSE);
	fail_if (!st, 0, PE ("Couldn't set wide-open ACL", GetLastError (), 0, 0, 0));
	st = IsValidSecurityDescriptor (&sd);
	fail_if (!st, 0, PE ("SD seems to not be valid", 0, 0, 0, 0));

	st = GovGetName (name, MAX_PATH, "section");
	fail_if (!st, 0, PE ("Could not construct the name for the section object (%i)\n", GetLastError (), 0, 0, 0));

	sa.nLength = sizeof (SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = &sd;

	GbSection = CreateFileMapping (NULL, &sa, PAGE_READWRITE, 0, GbDefaultSize, name);
	fail_if (!GbSection, 0, PE_STR ("Could not create section \"%s\", %i\n", COPYSTRING (name), GetLastError (), 0, 0));
	fail_if (exclusive && GetLastError () == ERROR_ALREADY_EXISTS, 0, PE_STR ("Section %s already exists", COPYSTRING (name), 0, 0, 0));
	if (GetLastError () == ERROR_ALREADY_EXISTS)
		doinit = 0;

	st = GovGetName (name, MAX_PATH, "mutex");
	fail_if (!st, 0, PE ("Could not construct name for the mutex object (%i)\n", GetLastError (), 0, 0, 0));

	GbMutex = CreateMutex (&sa, FALSE, name);
	fail_if (!GbMutex, 0, PE_STR ("Could not open mutex \"%s\", %i\n", COPYSTRING (name), GetLastError (), 0, 0));
	fail_if (exclusive && GetLastError () == ERROR_ALREADY_EXISTS, 0, PE_STR ("Semaphore %s already exists", COPYSTRING (name), 0, 0, 0));
	fail_if (!doinit && GetLastError () != ERROR_ALREADY_EXISTS, 0, PE ("Section already exists, but semaphore does not?? (%i)", GetLastError (), 0, 0, 0));

	out = MapViewOfFile (GbSection, FILE_MAP_WRITE, 0, 0, sizeof (sectionroot_t));

	__try
	{
		if (doinit)
		{
			/* We're the first to open it, need to initialize */
			GbClearBuffer (out);
		}

		GbCurrentSize = ((sectionroot_t *)out)->buffersize;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		fail_if (1, 0, PE ("Exception 0x%.8X accessing section", GetExceptionCode (), 0, 0, 0));
	}

	st = UnmapViewOfFile (out);
	if (!st)
		GovDprintf (CRIT, "Could not unmap view of file!  %i!\n", GetLastError ());
	out = NULL;

	return 1;
failure:

	if (out)
		UnmapViewOfFile (out);

	if (GbSection)
	{
		CloseHandle (GbSection);
		GbSection = NULL;
	}

	if (GbMutex)
	{
		CloseHandle (GbMutex);
		GbMutex = NULL;
	}

	return 0;
}


static int GbOpenHandles (void)
{
	DERR st;
	char name[MAX_PATH];

	fail_if (GovIsPlugin (), 0, DP (ERROR, "GbOpenHandles will not operate inside of the plugin\n"));
	/* Check for this; it should never happen--precondition for this function is
	 * that they are not open */
	fail_if (GbSection, 0, DP (ERROR, "GbOpenHandles: Section already open\n"));
	fail_if (GbMutex, 0, DP (ERROR, "GbOpenHandles: Semaphore already open\n"));

	st = GovGetName (name, MAX_PATH, "section");
	fail_if (!st, 0, DP (ERROR, "Could not construct the name for the section object (%i)\n", GetLastError ()));

	GbSection = OpenFileMapping (FILE_MAP_WRITE, FALSE, name);
	fail_if (!GbSection, 0, DP (ERROR, "Could not open section \"%s\", %i\n", name, GetLastError ()));

	st = GovGetName (name, MAX_PATH, "mutex");
	fail_if (!st, 0, DP (ERROR, "Could not construct name for the mutex object (%i)\n", GetLastError ()));

	GbMutex = OpenMutex (MUTEX_ALL_ACCESS, FALSE, name);
	fail_if (!GbMutex, 0, DP (ERROR, "Could not open mutex \"%s\", %i\n", name, GetLastError ()));

	return 1;
failure:
	if (GbSection)
	{
		CloseHandle (GbSection);
		GbSection = NULL;
	}

	if (GbMutex)
	{
		CloseHandle (GbMutex);
		GbMutex = NULL;
	}

	return 0;
}

void GbCloseHandles (void)
{
	if (GbSection)
	{
		GovDprintf (NOISE, "Pid %i closing section handle\n", GetCurrentProcessId ());
		CloseHandle (GbSection);
		GbSection = NULL;
	}

	if (GbMutex)
	{
		CloseHandle (GbMutex);
		GbMutex = NULL;
	}

	GbCurrentSize = -1;
}



int GbGetBuffer (parerror_t *pe, void **buffer)
{
	DERR st;
	void *out = NULL;
	int insem = 0;

	//PlPrintf ("Get buffer: %X %X\n", GbSection, GbMutex);

	if (!GbSection)
	{
		fail_if (GovIsPlugin (), 0, PEIF ("Governor not yet enabled", 0, 0, 0, 0));

		/* Sanity check */
		fail_if (GbMutex, 0, DP (ERROR, "Consistency failure: Semaphore open, section not.\n"));

		st = GbOpenHandles ();
		fail_if (!st, 0, PEIF ("Couldn't open the buffer", 0, 0, 0, 0));
	}

	st = WaitForSingleObject (GbMutex, 1000);
	fail_if (st == WAIT_FAILED, 0, PEIF ("WaitForSingleObject Failed (%i)", GetLastError (), 0, 0, 0));
	fail_if (st == WAIT_TIMEOUT, 0, PEIF ("WaitForSingleObject timed out", 0, 0, 0, 0));
	insem = 1;

	if (GbCurrentSize == -1)
	{
		out = MapViewOfFile (GbSection, FILE_MAP_WRITE, 0, 0, sizeof (sectionroot_t));
		fail_if (!out, 0, PEIF ("Could not map view of section to get size, %i", GetLastError (), 0, 0, 0));

		__try
		{
			GbCurrentSize = ((sectionroot_t *)out)->buffersize;
			fail_if (GbCurrentSize < sizeof (sectionroot_t), 0, PEIF ("Current size error; %i too small--Section initilization failure?", GbCurrentSize, 0, 0, 0));
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			fail_if (1, 0, PEIF ("Exception 0x%.8X accessing section", GetExceptionCode (), 0, 0, 0));
		}

		st = UnmapViewOfFile (out);
		if (!st)
			GovDprintf (CRIT, "Could not UNmap view of section, %i!\n", GetLastError ());
		out = NULL;
	}

	out = MapViewOfFile (GbSection, FILE_MAP_WRITE, 0, 0, GbCurrentSize);
	fail_if (!out, 0, PEIF ("Could not map view of section, %i", GetLastError (), 0, 0, 0));

	*buffer = out;
	return 1;
failure:
	if (out)
		UnmapViewOfFile (out);

	if (insem)
		ReleaseMutex (GbMutex);

	return 0;
}

void GbReleaseBuffer (void *buffer)
{
	int st;

	st = UnmapViewOfFile (buffer);
	if (!st)
		GovDprintf (CRIT, "Could not unmap view of data, %i\n", GetLastError ());

	st = ReleaseMutex (GbMutex);
	if (!st)
		GovDprintf (CRIT, "Could not release mutex, %i\n", GetLastError ());

}


void *GbAppend (void *buffer, int len)
{
	void *out;

	sectionroot_t *r = buffer;

	/* Round len up to the next multiple of 4. */
	len = (len + 3) & ~0x3;
	if (r->nextfreeofs + len > r->buffersize)
		return NULL; /* no space */

	out = (char *)buffer + r->nextfreeofs;
	r->nextfreeofs += len;

	return out;
}

#define S_TYPE(buffer, ofs) ((sectiontype_t *)(((char *)buffer) + (ofs)))
#define S_OBJ(buffer, ofs) ((sectionobj_t *)(((char *)buffer) + (ofs)))

int GbSetEntry (void *buffer, char *type, char *key, char *parse, int flags)
{
	DERR st;
	sectiontype_t *t;
	sectionroot_t *r;
	sectionobj_t *o;
	int *newval;
	int len;

	fail_if (!buffer || !type || !key || !parse, 0, 0);

	r = buffer;

	/* Find the type.  If we hit the end we'll add one */
	newval = &r->firsttypeofs;
	while (*newval)
	{
		t = S_TYPE (buffer, *newval);
		if (strcmp (t->name, type) == 0)
			break;
		newval = &t->nexttypeofs;
	}

	if (!*newval)
	{
		/* Add a new type */
		len = sizeof (sectiontype_t) + strlen (type) + 1;
		t = GbAppend (buffer, len);
		fail_if (!t, 0, 0);

		strcpy (t->name, type);
		t->firstobjofs = 0; /* No objects yet */
		t->nexttypeofs = 0; /* Last type in the list */

		*newval = (char *)t - (char *)buffer;
	}

	/* Now we have our type; add a new object. */

	len = sizeof (sectionobj_t) + strlen (key) + 1 + strlen (parse) + 1;
	o = GbAppend (buffer, len);
	fail_if (!o, 0, 0);

	o->flags = flags;
	o->nextobjofs = t->firstobjofs;
	strcpy (o->strings, key);
	strcpy (o->strings + (strlen (key) + 1), parse);
	t->firstobjofs = (char *)o - (char *)buffer;

	return 1;
failure:
	/* Note:  If the type append succeeds, but the object append fails, this
	 * will result in the type being left behind.  I condsider this OK; there
	 * should only be things added to the buffer, never removed.*/
	return 0;
}

int GbGetEntry (void *buffer, char *type, char *key, sectionobj_t **obj)
{
	sectionroot_t *r;
	sectiontype_t *t;
	sectionobj_t *o;
	int walk;

	if (!buffer || !type || !key || !obj)
		return 0;

	r = buffer;

	walk = r->firsttypeofs;
	while (walk)
	{
		t = S_TYPE (buffer, walk);
		if (strcmp (t->name, type) == 0)
			break;

		walk = t->nexttypeofs;
	}

	if (!walk) /* Nothing of this type, so can't be found */
		return 0;

	walk = t->firstobjofs;
	while (walk)
	{
		o = S_OBJ (buffer, walk);
		if (strcmp (o->strings, key) == 0)
			break;

		walk = o->nextobjofs;
	}

	if (!walk)
		return 0;

	/* Found. */
	*obj = o;
	return 1;
}

void GbDumpBuffer (void *buffer)
{
	sectionroot_t *r;
	sectiontype_t *t;
	sectionobj_t *o;
	int twalk, owalk;

	if (!buffer)
		return;

	r = buffer;

	PlPrintf ("ROOT: size:%X nextfree:%X firsttype: %X\n", r->buffersize, r->nextfreeofs, r->firsttypeofs);

	twalk = r->firsttypeofs;
	while (twalk)
	{
		t = S_TYPE (buffer, twalk);

		PlPrintf ("%.4X:TYPE: %s (%X,%X)\n", twalk, t->name, t->firstobjofs, t->nexttypeofs);
		owalk = t->firstobjofs;
		while (owalk)
		{
			o = S_OBJ (buffer, owalk);
			PlPrintf ("%.4X: OBJ: %s %s (%i,%X)\n", owalk, o->strings, o->strings + strlen (o->strings) + 1, o->flags, o->nextobjofs);

			owalk = o->nextobjofs;
		}

		twalk = t->nexttypeofs;
	}
	
	
}





/******************  script functions *********************/

int GbGetSize (parerror_t *pe)
{
	void *buffer = NULL;
	DERR st;
	int out;

	st = GbGetBuffer (pe, &buffer);
	fail_if (!st, 0, 0);

	__try
	{
		out = ((sectionroot_t *)buffer)->buffersize;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		fail_if (1, 0, PE ("Exception 0x%.8X while trying to read buffer size", GetExceptionCode (), 0, 0, 0));
	}

	GbReleaseBuffer (buffer);
	buffer = NULL;

	fail_if ((int)GbCurrentSize != out, 0, PE ("Internal consistency failure: Section size == %i, Known size == %i", out, GbCurrentSize, 0, 0));

	return out;
failure:
	if (buffer)
		GbReleaseBuffer (buffer);

	return 0;
}


int GbGetFreeSpace (parerror_t *pe)
{
	void *buffer = NULL;
	DERR st;
	int out, size;

	st = GbGetBuffer (pe, &buffer);
	fail_if (!st, 0, 0);

	__try
	{
		out = ((sectionroot_t *)buffer)->buffersize - ((sectionroot_t *)buffer)->nextfreeofs;
		size = ((sectionroot_t *)buffer)->buffersize;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		fail_if (1, 0, PE ("Exception 0x%.8X while trying to read buffer size", GetExceptionCode (), 0, 0, 0));
	}

	GbReleaseBuffer (buffer);
	buffer = NULL;

	fail_if ((int)GbCurrentSize != size, 0, PE ("Internal consistency failure: Section size == %i, Known size == %i", out, GbCurrentSize, 0, 0));

	return out;
failure:
	if (buffer)
		GbReleaseBuffer (buffer);

	return 0;
}

int GbGetDefaultSize (void)
{
	return GbDefaultSize;
}

void GbSetDefaultSize (parerror_t *pe, int newsize)
{
	DERR st;

	fail_if (GbSection, 0, PE ("Cannot set size when governor is enabled", 0, 0, 0, 0));
	fail_if (newsize < sizeof (sectionroot_t), 0, PE ("Size %i is less than absolute minimum (%i)", newsize, sizeof (sectionroot_t), 0, 0));

	GbDefaultSize = newsize;
failure:
	return;
}


#define F_TRUE fail_if (st, 0, PE ("Fail %i %i", failnum, __LINE__, 0, 0)); failnum++
#define F_FALSE fail_if (!st, 0, PE ("Fail %i %i", failnum, __LINE__, 0, 0)); failnum++
#define F_SENTINEL fail_if (*((unsigned int *)(buffer + sizeofs)) != 0xFEFEFEFE, 0, PE ("Sentinel fail %i %i", failnum, __LINE__, 0, 0)); failnum++
void GbTestBuffer (parerror_t *pe)
{
	char buffer[1024];
	sectionroot_t *r = (void *)buffer;
	sectionobj_t *o;
	int st;
	int sizeofs;
	int failnum = 0;

	r->buffersize = 1020;
	r->firsttypeofs = 0;
	r->nextfreeofs = sizeof (sectionroot_t);
	sizeofs = 1020;
	*((int *)(buffer + sizeofs)) = 0xFEFEFEFE;
	
	GbDumpBuffer (buffer);

	st = GbSetEntry (buffer, "CREATE_CLASS", "Notepad", "(do something)", 0);
	F_FALSE;
	F_SENTINEL;

	st = GbSetEntry (buffer, "CREATE_CLASS", "Aim_window", "(do something)", 0);
	F_FALSE;
	F_SENTINEL;

	st = GbGetEntry (buffer, "CREATE_CLASS", "Notepad", &o);
	F_FALSE;

	st = GbGetEntry (buffer, "CREATE_CLASS", "Notepad2", &o);
	F_TRUE;

	st = GbGetEntry (buffer, "CREATE_TITLE", "Notepad", &o);
	F_TRUE;

	GbDumpBuffer (buffer);

	st = GbSetEntry (buffer, "TYPE_TWO", "Something", "", 0);
	F_FALSE;
	F_SENTINEL;

	st = GbSetEntry (buffer, "TYPE_TWO", "Something", "more", 0);
	F_FALSE;
	F_SENTINEL;

	st = GbGetEntry (buffer, "TYPE_TWO", "Something", &o);
	F_FALSE;
	fail_if (strcmp (o->strings + strlen (o->strings) + 1, "more") != 0, 0, PE ("Result mismatch", 0, 0, 0, 0));


	GbDumpBuffer (buffer);

	r->buffersize = 0x44;
	r->firsttypeofs = 0;
	r->nextfreeofs = sizeof (sectionroot_t);
	sizeofs = 0x44;
	*((int *)(buffer + sizeofs)) = 0xFEFEFEFE;

	st = GbSetEntry (buffer, "CREATE_CLASS", "Notepad", "(do something)", 0);
	F_FALSE;
	F_SENTINEL;

	st = GbSetEntry (buffer, "CREATE_CLASS", "Aim_window", "(do something)", 0);
	F_TRUE;
	F_SENTINEL;

	st = (int)GbAppend (buffer, 0);
	F_FALSE;
	F_SENTINEL;

	st = (int)GbAppend (buffer, 1);
	F_TRUE;
	F_SENTINEL;

	GbDumpBuffer (buffer);


	r->buffersize = 0x45;
	r->firsttypeofs = 0;
	r->nextfreeofs = sizeof (sectionroot_t);
	sizeofs = 0x44;
	*((int *)(buffer + sizeofs)) = 0xFEFEFEFE;

	st = GbSetEntry (buffer, "CREATE_CLASS", "Notepad", "(do something)", 0);
	F_FALSE;
	F_SENTINEL;

	st = GbSetEntry (buffer, "CREATE_CLASS", "Aim_window", "(do something)", 0);
	F_TRUE;
	F_SENTINEL;

	st = (int)GbAppend (buffer, 0);
	F_FALSE;
	F_SENTINEL;

	st = (int)GbAppend (buffer, 1);
	F_TRUE;
	F_SENTINEL;

	GbDumpBuffer (buffer);


	r->buffersize = 0x48;
	r->firsttypeofs = 0;
	r->nextfreeofs = sizeof (sectionroot_t);
	sizeofs = 0x44;
	*((int *)(buffer + sizeofs)) = 0xFEFEFEFE;

	st = GbSetEntry (buffer, "CREATE_CLASS", "Notepad", "(do something)", 0);
	F_FALSE;
	F_SENTINEL;

	st = GbSetEntry (buffer, "CREATE_CLASS", "Aim_window", "(do something)", 0);
	F_TRUE;
	F_SENTINEL;

	st = (int)GbAppend (buffer, 0);
	F_FALSE;
	F_SENTINEL;

	st = (int)GbAppend (buffer, 1);
	F_FALSE;
	F_SENTINEL;

	st = (int)GbAppend (buffer, 1);
	F_TRUE;
	F_SENTINEL;

	GbDumpBuffer (buffer);


	r->buffersize = 0x60;
	r->firsttypeofs = 0;
	r->nextfreeofs = sizeof (sectionroot_t);
	sizeofs = 0x60;
	*((int *)(buffer + sizeofs)) = 0xFEFEFEFE;

	st = GbSetEntry (buffer, "CREATE_CLASS", "Notepad", "(do something)", 0);
	F_FALSE;
	F_SENTINEL;

	st = GbSetEntry (buffer, "CREATE_TITLE", "Aim_window", "(do something)", 0);
	F_TRUE;
	F_SENTINEL;

	GbDumpBuffer (buffer);


	r->buffersize = 0x60;
	r->firsttypeofs = 0;
	r->nextfreeofs = sizeof (sectionroot_t);
	sizeofs = 0x60;
	*((int *)(buffer + sizeofs)) = 0xFEFEFEFE;

	st = GbSetEntry (buffer, "CREATE_CLASS", "Notepad", "(do something)", 0);
	F_FALSE;
	F_SENTINEL;

	st = GbSetEntry (buffer, "CREATE_TITLE", "Aim_window", "(do something)", 0);
	F_TRUE;
	F_SENTINEL;

	r->buffersize = 1000;
	st = GbSetEntry (buffer, "CREATE_TITLE", "Test 2", "(do something2)", 0);
	F_FALSE;
	
	GbDumpBuffer (buffer);


	PlPrintf ("** Start run **\n");
	r->buffersize = 1020;
	r->firsttypeofs = 0;
	r->nextfreeofs = sizeof (sectionroot_t);
	sizeofs = 1020;
	*((int *)(buffer + sizeofs)) = 0xFEFEFEFE;

	st = GbSetEntry (buffer, "ACTIVATE_CLASS", "whatever", "(parse this bitch)", 0);
	F_FALSE;
	F_SENTINEL;

	GbDumpBuffer (buffer);

	st = GbGetEntry (buffer, "ACTIVATE_CLASS", "whatever", &o);
	F_FALSE;

	o->flags = 34;

	GbDumpBuffer (buffer);

	st = GbGetEntry (buffer, "ACTIVATE_CLASS", "whatever", &o);
	F_FALSE;

	o->strings[0] = '\0'; /* "delete" equivalent. */

	GbDumpBuffer (buffer);
failure:
	return;
}