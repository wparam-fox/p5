/* Copyright 2006 wParam, licensed under the GPL */

#include "instrument.h"


#include "help.h"
#include "export.h"
#include "pvar.h"


EXPORT int Plugin_Init (void)
{
	fail_if (!SetMode (MODE_MANAGER), (ERROR, "Mode is already set"));
	//SetDbLevel (NOISE);

	CancelAgent ();
	
	return 1;

failure:
	return 0;
}

EXPORT void Plugin_Denit (void)
{

}


//!instrument pid [pid] [etc]
int CMD_Instrument (int argc, char **argv)
{
	message_t *m;
	DWORD pid;
	int out;

	if (argc < 3)
		return INSTRUMENT_TFP;

	if (strcmp (argv[1], "pid") == 0)
		pid = SmartIntConvert (argv[2]);
	else if (strcmp (argv[1], "hookpid") == 0)
	{
		pid = SmartIntConvert (argv[2]);
		ManagerHookProcess (pid);
		Sleep (500); //need to wait for instrument thread in target process to get running
	}
	else
		return INSTRUMENT_BADPIDTYPE;

	argc -= 3;
	argv += 3;

	m = ParseMessage (argc, argv);
	if (!m)
		return INSTRUMENT_NOMESSAGE;

	out = ManagerSend (pid, m);

	free (m);

	if (out == 2)
		return INSTRUMENT_NOACK;
	else if (out == 0)
		return INSTRUMENT_NOSEND;

	return INSTRUMENT_NOERROR;
}

#define PAIR_SIZE (sizeof (char *) + sizeof (DWORD (*)()))
static const struct 
{
	char *name;
	int (*func)(int, char **);
} cmds[] = 
{
	{"!instrument", CMD_Instrument},
};




EXPORT int Plugin_Parse (int argc, char **argv)
{
	int x;
	int fin=sizeof (cmds)/PAIR_SIZE;
	for (x=0;x<fin;x++)
	{
		if (strcmp (argv[0], cmds[x].name) == 0)
		{
			return cmds[x].func (argc, argv);
			
		}
	}
	return 0;
}


const help_t PluginHelp[] =
{
	{
		"!instrument",
		"The only command in the DLL",
		"",
		"",
	},
};

EXPORT int Plugin_GetNumCmds (void)
{
	return sizeof (PluginHelp) / sizeof (help_t);
}


EXPORT lpHelp Plugin_GetHelp (void)
{
	return (help_t *)PluginHelp;
}

EXPORT char * Plugin_GetInfoString (void)
{
	return "Generic, arbitrary function and window instrumentation";
}

const char * ErrorStrings[] =
{
	NULL, //returning zero means its some other plugins command
	"No Error",   //return of 1, normally no error
	"To few Parameters",
	"Bad PID type",
	"ParseMessage failed",
};

EXPORT char *Plugin_TranslateErrorCode (int code)
{
	if (code >= (sizeof (ErrorStrings) / sizeof (char *)))
		return "Invalid error code";
	return (char *)(ErrorStrings[code]);
}


///////optional




