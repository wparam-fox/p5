/* Copyright 2006 wParam, licensed under the GPL */
#include <string.h>
#include <stdlib.h>
#include "wp_parse.h"

#define fail_if(cond, status, other) if (cond) { st = status ; other ; /*printf ("line: %i\n", __LINE__);*/ goto failure ; }

/*
#define StrMalloc testallocator
#define StrFree testfreer
#define StrRealloc testrealloc
*/

#ifdef StrMalloc
void * StrMalloc (int);
#else
#define StrMalloc malloc
#endif

#ifdef StrFree
void StrFree (void *);
#else
#define StrFree free
#endif

#ifdef StrRealloc
void * StrRealloc (void *, int);
#else
#define StrRealloc realloc
#endif

int StrTokenize (char *in, int *argcout, char ***argvout)
{
	//PARSE_DYN_T pdt={NULL, 0, sizeof (char *)};
	int st;
	char *start;
	char temp;
	int ret=0;
	int inmul=0;
	char **argv;
	void *tempval;
	

	argv = StrMalloc (0);
	fail_if (!argv, 0, 0);

	while (1)
	{
		if (*in && (*in == ' ' || *in == '\n'))
			while (*in && *in != ' ' && *in != '\n')
				in++;

		if (*in && (*in == ' ' || *in == '\n'))
			in++;
		if (*in == '\"')
		{
			in++;
			inmul = 1;
		}
		else
			inmul = 0;
		start = in;
		if (!inmul)
		{
			while (*in && *in != ' ' && *in != '\n')
				in++;
		}
		else
		{
			while (*in && *in != '\"')
				in++;
			//in--; //don't include the quote
		}

		if (start != in) //length is more than 1
		{
			tempval = StrRealloc (argv, sizeof (char **) * (ret+1));
			fail_if (!tempval, 0, 0);
			argv = tempval;
			argv[ret] = StrMalloc (in - start + 1);
			fail_if (!argv[ret], 0, 0);
			temp = *in;
			*in = '\0';
			strcpy (argv[ret], start);
			*in = temp;
			ret++;
		}
		else //end of string
		{
			break;
		}
		if (inmul)
		{
			in++;
		}
	}

	*argcout = ret;
	*argvout = argv;

	return 1;

failure:
	if (argv)
	{
		for (ret--;ret >= 0;ret--)
		{
			if (argv[ret])
				StrFree (argv[ret]);
		}

		StrFree (argv);
	}

	*argcout = 0;
	*argvout = NULL;

	return 0;

}

void StrFreeTokenize (int argc, char **argv)
{
	int x;
	for (x=0;x<argc;x++)
	{
		StrFree (argv[x]);
	}
	StrFree (argv);
}

#if 0

#include <windows.h>

HANDLE heap;
int degrade = 0;

void *testallocator (int size)
{
	if (degrade && rand () % 100 < degrade)
		return NULL;

	return HeapAlloc (heap, 0, size);
}

void testfreer (void *block)
{
	HeapFree (heap, 0, block);
}

void *testrealloc (void *blah, int size)
{
	if (degrade && rand () % 100 < degrade)
		return NULL;

	return HeapReAlloc (heap, 0, blah, size);
}

int count (void)
{
	PROCESS_HEAP_ENTRY phe = {0};
	int count = 0;
	int busycount = 0;

	while (HeapWalk (heap, &phe))
	{
		if (phe.wFlags & PROCESS_HEAP_ENTRY_BUSY)
			busycount++;

		count++;
	}

	//printf ("heap count = %i,%i\n", count, busycount);

	return busycount;
}

int StrTestRoutine (void)
{
	int bob;
	int argc;
	char **argv;
	char buffer[100];

	heap = HeapCreate (0, 1000000, 0);

	bob = count ();

	degrade = 10;

	while (1)
	{
		strcpy (buffer, "la la la la la");
		StrTokenize (buffer, &argc, &argv);
		StrFreeTokenize (argc, argv);
		if (count () != bob) break;

		strcpy (buffer, "la la \"la la\" la");
		StrTokenize (buffer, &argc, &argv);
		StrFreeTokenize (argc, argv);
		if (count () != bob) break;

		strcpy (buffer, "la la la la la oaerjoigjerogjreg \" \" rgnrug");
		StrTokenize (buffer, &argc, &argv);
		StrFreeTokenize (argc, argv);
		if (count () != bob) break;
	}

	__asm int 3;

	return 0;
}


#endif