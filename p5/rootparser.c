/* Copyright 2008 wParam, licensed under the GPL */

#include <windows.h>
#include "p5.h"
#include <stdio.h>


//this is only a container for global functions and variables, it should NOT be taking parse 
//requests directly, hence we declare it static.
static parser_t *RopRootParser = NULL;

//if this is set to 1, all loops will stop looping.
static int RopRunawayLoopStop = 0;

//if this is set to 1, begin() will print extra information
static int RopBeginDebugging = 0;






static char *RopVa (parerror_t *pe, char *format, ...)
{
	char *out, buffer[1024];
	va_list marker;
	int st;

	fail_if (!format, 0, PE_BAD_PARAM (1));

	va_start (marker, format);
	__try
	{
		_vsnprintf (buffer, 1023, format, marker);
		buffer[1023] = '\0';
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		va_end (marker);
		fail_if (TRUE, 0, PE ("_vsnprintf exception 0x%.8X", GetExceptionCode (), 0, 0, 0));
	}
		
	va_end (marker);

	out = ParMalloc (strlen (buffer) + 1);
	fail_if (!out, 0, PE ("Out of memory on %i bytes", strlen (buffer) + 1, 0, 0, 0));

	strcpy (out, buffer);

	return out;
failure:
	return NULL;

}




static char *RopIf (parser_t *par, parerror_t *pe, int arg, char *true, char *false)
{
	int nesting, st;
	char *out = NULL;

	if (arg)
		st = ParRunLineCurrent (par, true,  &out, &nesting, pe);
	else
		st = ParRunLineCurrent (par, false, &out, &nesting, pe);

	if (!DERR_OK (st))
	{
		if (out) //shouldn't be possible
			ParFree (out);

		//basically, we're just punting the error on back up the chain.  In theory,
		//pe should already be filled with the appropriate error values.
		return NULL;
	}

	if (nesting)
	{
		//we're calling this a syntax error, although really I'm not convinced it can happen
		//(I'm pretty sure (nesting) implies (!DERR_OK (st)) when ParRunLineCurrent is being called.
		if (out)
			ParFree (out);

		PE ("Syntax error in %s expression", arg ? "true" : "false", 0, 0, 0);
		return NULL;
	}
	
	//!out is caught by the schemer

	return out;
}


static void RopDefineFull (parser_t *par, parerror_t *pe, char *name, char *proto, char *action, char *help1, char *help2, char *help3)
{
	DERR st, res;

	
	fail_if (!proto, 0, PE_BAD_PARAM (2));
	fail_if (!help1, 0, PE_BAD_PARAM (4));
	fail_if (!help2, 0, PE_BAD_PARAM (5));
	fail_if (!help3, 0, PE_BAD_PARAM (6));

	res = ParAddDefine (par, name, proto, action, help1, help2, help3);
	fail_if (!DERR_OK (res), 0, PE ("ParAddDefine failed, 0x%.8X", res, 0, 0, 0));

failure:
	return;
}

static void RopDefineRootFull (parser_t *par, parerror_t *pe, char *name, char *proto, char *action, char *help1, char *help2, char *help3)
{
	//no need to check params, RopDefineFull will do just fine.
	RopDefineFull (RopRootParser, pe, name, proto, action, help1, help2, help3);
}

static void RopDefine (parser_t *par, parerror_t *pe, char *name, char *proto, char *action)
{
	//again, no need to check because the indexes are all the same.
	RopDefineFull (par, pe, name, proto, action, "", "", "");
}

static void RopUndefine (parser_t *par, parerror_t *pe, char *name, char *proto)
{
	DERR st, res;

	fail_if (!proto, 0, PE_BAD_PARAM (2));

	res = ParRemoveDefine (par, name, proto);
	fail_if (!DERR_OK (res), 0, PE ("ParRemoveDefine failed, 0x%.8X", res, 0, 0, 0));
failure:
	return;
}

static void RopUndefineRoot (parser_t *par, parerror_t *pe, char *name, char *proto)
{
	RopUndefine (RopRootParser, pe, name, proto);
}


static void RopBind (parser_t *par, parerror_t *pe, char *atom, char *value)
{
	int st, res;

	fail_if (!value, 0, PE_BAD_PARAM (2));

	res = ParAddAtom (par, atom, value, 0);
	fail_if (!DERR_OK (res), 0, PE ("ParAddAtom returned failure 0x%.8X", res, 0, 0, 0));
failure:
	return;
}

static void RopBindRoot (parerror_t *pe, char *atom, char *value)
{
	RopBind (RopRootParser, pe, atom, value);
}

static void RopUnbind (parser_t *par, parerror_t *pe, char *atom)
{
	int st, res;

	res = ParRemoveAtom (par, atom);
	fail_if (!DERR_OK (res), 0, PE ("ParRemoveAtom returned failure 0x%.8X", res, 0, 0, 0));
failure:
	return;
}

static void RopUnbindRoot (parerror_t *pe, char *atom)
{
	RopUnbind (RopRootParser, pe, atom);
}


static void RopAlias (parser_t *par, parerror_t *pe, char *func, char *proto, char *target)
{
	DERR st;

	fail_if (!proto, 0, PE_BAD_PARAM (2));

	st = ParAddAlias (par, func, proto, target);
	fail_if (!DERR_OK (st), st, PE ("ParAddAlias returned failure, {%s,%i}", PfDerrString (st), GETDERRINFO (st), 0, 0));

failure:
	return;
}

static void RopAliasRoot (parerror_t *pe, char *func, char *proto, char *target)
{
	RopAlias (RopRootParser, pe, func, proto, target);
}

static void RopUnalias (parser_t *par, parerror_t *pe, char *func, char *proto)
{
	DERR st;

	fail_if (!proto, 0, PE_BAD_PARAM (2));

	st = ParRemoveAlias (par, func, proto);
	fail_if (!DERR_OK (st), st, PE ("ParRemoveAlias returned failure, {%s,%i}", PfDerrString (st), GETDERRINFO (st), 0, 0));

failure:
	return;
}

static void RopUnaliasRoot (parerror_t *pe, char *func, char *proto)
{
	RopUnalias (RopRootParser, pe, func, proto);
}

static void RopSetBeginDebug (int arrest)
{
	RopBeginDebugging = arrest;
}


static void RopBegin (parser_t *par, parerror_t *pe, int count, ...)
{
	DERR st;
	int numparams = count - 3;
	int x = 0;
	va_list va;
	int inva = 0;
	char *param;
	int incomment, abort, errormode, restart, gotocount;
	char labelbuf[MAX_LABEL];
	char *targetlabel;
	char error[1024];
	char *output;

	//first step is to make sure all the params form valid strings.
	__try
	{
		va_start (va, count);
		inva = 1;
		for (x=0;x<numparams;x++)
		{
			param = va_arg (va, char *);

			fail_if ((((int)param) & 0xFFFF0000) == 0, 0, PE ("Parameter %i to (begin) invalid. (not a string)", x + 1, 0, 0, 0));
			fail_if (param[0] != '(', 0, PE ("Parameter %i to (begin) invalid. (not an s-expression)", x + 1, 0, 0, 0));
			strlen (param); //should throw an exception if it's invalid
		}
		va_end (va);
		inva = 0;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		fail_if (TRUE, 0, PE ("Parameter %i to (begin) invalid. (exception on access)", x + 1, 0, 0, 0));
	}

	incomment = 0;
	abort = 0;
	errormode = ERROR_ABORT;
	targetlabel = NULL;
	gotocount = 256;

	va_start (va, count);
	inva = 1;
	for (x=0;x<numparams;x++)
	{
		param = va_arg (va, char *);

		if (ScrIsHashLine (param))
		{
			if (RopBeginDebugging)
				ConWriteStringf ("BEGIN: %i=HASH %s\n", x, param);

			st = ScrHashProcessing (param, &incomment, labelbuf, &targetlabel, &restart, &errormode, 0, &abort, 0, error, 1024, &gotocount);
			fail_if (!DERR_OK (st), st, PE ("ScrHashProcessing returned {%s,%i} for param %i", PfDerrString (st), GETDERRINFO (st), x, 0));
			fail_if (abort, PF_SUCCESS, 0); //just return if abort is set

			if (restart)
			{
				va_end (va);
				va_start (va, count);
				x = -1;
			}

			continue;
		}

		if (incomment)
			continue;

		if (targetlabel)
			continue;

		if (RopBeginDebugging)
			ConWriteStringf ("BEGIN: %i=RUN %s\n", x, param);

		st = ParRunLineCurrent (par, param, &output, NULL, pe);
		if (output)
			ParFree (output);
		//if the expression isn't complete, STATUS_MORE_DATA_REQUIRED is returned as an error

		if (!DERR_OK (st))
		{
			fail_if (errormode & ERROR_ABORT, 0, 0);

			//ok, so some sort of continue is set.
			//we need to capture the error string out of the parerror_t, because it
			//may have memory allocated in it.  (That's why we always call ProcessError,
			//even if we have no intention of actually printing it.)

			ParProcessErrorType (st, error, 1024, pe);

			if (errormode & ERROR_VERBOSE)
				ConWriteStringf ("Begin expresison %i error: %s\n", x, error);
			
			//reset the error to the default (ok) state
			memset (pe, 0, sizeof (parerror_t));

			if (errormode & ERROR_SKIP)
				targetlabel = labelbuf;

			*error = '\0';
		}

	}
	va_end (va);

	if (RopBeginDebugging)
		ConWriteStringf ("BEGIN: done.");

	return;

failure:

	if (RopBeginDebugging)
		ConWriteStringf ("BEGIN: err out (%i), line %i\n", st, x);

	if (inva)
		va_end (va);

	return;
}


DERR RopAddRootFunction (char *name, char *proto, void *addy, char *h1, char *h2, char *h3)
{
	return ParAddFunction (RopRootParser, name, proto, addy, h1, h2, h3);
}

DERR RopRemoveRootFunction (char *name, char *proto)
{
	return ParRemoveFunction (RopRootParser, name, proto);
}


DERR RopHelpwinMainThreadHelper (void *param)
{
	helpblock_t **helpout = param, *help;

	//help is always freed by HwCreateHelpWindow no matter what.
	//However, it is possible that the call to PfRunInMainThread may have
	//failed, so we need to signal RopHelpwin that the memory was indeed
	//passed to HwCreateHelpWindow.  If, after PfRunInMainThread returns,
	//help is not null, it is freed, so we set it to NULL from here to
	//prevent a double free.
	help = *helpout;
	*helpout = NULL;


	return HwCreateHelpWindow (help);

}


void RopHelpwin (parser_t *par, parerror_t *pe)
{
	helpblock_t *help = NULL;
	DERR st;

	st = ParGetHelpData (par, &help);
	fail_if (!DERR_OK (st), st, PE ("ParGetHelpData failed, {%s,%i}\n", PfDerrString (st), GETDERRINFO (st), 0, 0));

	
	st = PfRunInMainThread (RopHelpwinMainThreadHelper, &help);
	//If help is massed to HwCreateHelpWindow, it will come out NULL.  If it's
	//not NULL, then the failure must have been in PfRunInMainThread, so we
	//free it, either in the failure clause or before the normal return.
	fail_if (!DERR_OK (st), st, PE ("HeCreateHelpWindow failed, {%s,%i}\n", PfDerrString (st), GETDERRINFO (st), 0, 0));

	if (help)
		ParFree (help);

	return;
failure:

	if (help)
		ParFree (help);


}

void RopHelpwinRoot (parerror_t *pe)
{
	RopHelpwin (RopRootParser, pe);
}

static void RopWhile (parser_t *par, parerror_t *pe, int max, char *condition, char *body)
{
	int nesting, st;
	char *out = NULL;

	fail_if (PfIsMainThread () && max < 0, 0, PE ("Unlimited loops are not allowed in the main thread", 0, 0, 0, 0));


	while (max)
	{
		fail_if (RopRunawayLoopStop, 0, PE ("Loop terminated; The runaway loop safeguard is active.", 0, 0, 0, 0));

		st = ParRunLineCurrent (par, condition,  &out, &nesting, pe);
		fail_if (!DERR_OK (st), 0, 0); //pass pe on up the chain
		fail_if (nesting, 0, PE ("Syntax error in condition", 0, 0, 0, 0));
		fail_if (!out, 0, PE ("Condition returned NULL", 0, 0, 0, 0));

		st = atoi (out);
		fail_if (!st, 0, 0); //this isn't an error; don't set PE to anything

		ParFree (out);
		out = NULL;

		//run the body.

		st = ParRunLineCurrent (par, body,  &out, &nesting, pe);
		fail_if (!DERR_OK (st), 0, 0); //pass pe on up the chain
		fail_if (nesting, 0, PE ("Syntax error in body", 0, 0, 0, 0));
		
		if (out)
			ParFree (out);
		out = NULL;

		if (max > 0)
			max--;

	}

	return;

failure:

	if (out)
		ParFree (out);
}

void RopLoopArrest (int arrest)
{
	RopRunawayLoopStop = arrest;
}

DERR RopInit (void)
{
	DERR st;

	st = ParCreate (&RopRootParser, NULL);
	fail_if (!DERR_OK (st), st, 0);


	st = ParAddFunction (RopRootParser, "par.if", "upeiaa", RopIf, "Conditional branch", "(par.if [i:boolean expr] [a:true expr] [a:false expr]) = [u:result]", "Both true and false should be parsable expressions.  The if function returns what it returned by whichever branch is executed.  It will be classified into int, float, or string (may be NULL).");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (RopRootParser, "par.while", "vpeiaa", RopWhile, "Basic loop construct", "(par.while [i:max iterations] [a:condition expression] [a:body]) = [v]", "This works just like a C while loop.  First the condition is evaluated, and if it evaluates to nonzero, the body is executed.  The process repeats until the condition evaluates to zero.  The maximum iterations parameter is included as a means to prevent accidental infinite loops from locking things up.  You can use this as an easy way to make a loop run a certain fixed number of times.  If you really, really want the loop to run forever, you can specify a negative number, however, this is only valid if the loop is running outside of the main thread.  The condition should be an expression that evaluates to an integer.  It can just be a variable name, but it must be quoted to make it an atom (i.e. you have to say 'x instead of just x).  If the condition returns NULL, the loop fails.  If the condition returns something other than an integer, the loop will stop (but not fail).  I _STRONGLY_ recommend that you avoid loops in the main thread whenever possible, and when they must be used, use them with a small number of max iterations.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (RopRootParser, "par.looparrest", "vi", RopLoopArrest, "Runaway loop control", "(par.looparrest [i:enable/disable]) = [v]", "This command can be used to stop runaway loops.  If you pass a nonzero integer to it as the parameter, all running loops will terminate on their next iteration, and no new loops will run.  (The loops will end with an error.)  This stays in effect until you remove it by passing 0 to thie function.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (RopRootParser, "par.begin", "vpec.", RopBegin, "Sequential execution of s-expressions", "(par.begin [a:expr1] [a:expr2] ...) = [v]", "(begin) will execute each s-expression in order.  By default, any error will abort the (begin) command.  You can override this by putting a \'(# error.cont) as one of the s-expressions.  (begin) supports the same (#) syntax as the (script) command.  See (script.help) for more information.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (RopRootParser, "par.begindebug", "vi", RopSetBeginDebug, "Controls the begin debugging value", "(par.begindebug [i:newvalue]) = [v]", "If this is nonzero, (begin) commands will dump verbose status output to the console.  This is effectively reuqired for complicated expressions that go wrong.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (RopRootParser, "par.define", "vpeasasss", RopDefineFull, "Define a new function with help information", "(par.define [a:name] [s:prototype] [a:value] [s:help-description] [s:help-usage] [s:help-remarks]) = [v]", "Prototype should be a string specifying the types of the arguments, followed by a semicolon seperated list of parameter names.  For example, a define that takes an integer named foo1, a string named bar1 and returns a float would have a prototype string of \"fis;foo1;bar1\".  The return type character is always first.  If the define takes no parameters, the prototype string will be a single letter.  Action should be an s-expression; making it an integer sort of defeats the purpose of having a define.  Valid types are: v:void, i:int, f:float, a:atom, s:string.  Void is only valid as a return type.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (RopRootParser, "par.rt.define", "vpeasasss", RopDefineRootFull, "Define a new function in the root parser with help information", "(par.root.define [a:name] [s:prototype] [a:value] [s:help-description] [s:help-usage] [s:help-remarks]) = [v]", "Prototype should be a string specifying the types of the arguments, followed by a semicolon seperated list of parameter names.  For example, a define that takes an integer named foo1, a string named bar1 and returns a float would have a prototype string of \"fis;foo1;bar1\".  The return type character is always first.  If the define takes no parameters, the prototype string will be a single letter.  Action should be an s-expression; making it an integer sort of defeats the purpose of having a define.  Valid types are: v:void, i:int, f:float, a:atom, s:string.  Void is only valid as a return type.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (RopRootParser, "par.define", "vpeasa", RopDefine, "Define a new function", "(par.define [a:name] [s:prototype] [a:value]) = [v]", "Prototype should be a string specifying the types of the arguments, followed by a semicolon seperated list of parameter names.  For example, a define that takes an integer named foo1, a string named bar1 and returns a float would have a prototype string of \"fis;foo1;bar1\".  The return type character is always first.  If the define takes no parameters, the prototype string will be a single letter.  Action should be an s-expression; making it an integer sort of defeats the purpose of having a define.  Valid types are: v:void, i:int, f:float, a:atom, s:string.  Void is only valid as a return type.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (RopRootParser, "par.undefine", "vpeas", RopUndefine, "Remove a user defined function", "(par.undefine [a:name] [s:prototype]) = [v]", "Both the function name and the prototype must match exactly what was passed to the (define) functions.  Well, not exactly, the prototype should only contain the first term, i.e. the return an parameter type codes.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (RopRootParser, "par.rt.undefine", "vpeas", RopUndefineRoot, "Remove a user defined function from the root parser", "(par.root.undefine [a:name] [s:prototype]) = [v]", "Both the function name and the prototype must match exactly what was passed to the (define) functions.  Well, not exactly, the prototype should only contain the first term, i.e. the return an parameter type codes.");
	fail_if (!DERR_OK (st), st, 0);


	st = ParAddFunction (RopRootParser, "par.bind", "vpeas", RopBind, "Bind an atom to a value", "(par.bind [a:name] [s:value]) = [v]", "To bind an int, you'll need to convert it to a string first.  The atom will be classified before it is used.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (RopRootParser, "par.rt.bind", "veas", RopBindRoot, "Bind an atom to a value in the root parser", "(par.rt.bind [a:name] [s:value]) = [v]", "To bind an int, you'll need to convert it to a string first.  The atom will be classified before it is used.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (RopRootParser, "par.unbind", "vpea", RopUnbind, "Unbinds an atom (i.e. deletes a variable)", "(par.unbind [a:name]) = [v]", "You will most likely have to quote the atom's name when you pass it, otherwise it will be bound and you will get a type mismatch.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (RopRootParser, "par.rt.unbind", "vea", RopUnbindRoot, "Unbinds an atom (i.e. deletes a variable) from the root parser", "(par.rt.unbind [a:name]) = [v]", "You will most likely have to quote the atom's name when you pass it, otherwise it will be bound and you will get a type mismatch.");
	fail_if (!DERR_OK (st), st, 0);


	st = ParAddFunction (RopRootParser, "par.alias", "vpeasa", RopAlias, "Give a new name to an existing function", "(par.alias [a:new name] [s:prototype] [a:existing name]) = [v]", "Adding an alias is functionally the same as defining a function that does nothing but call the existing function, except that the execution is many many times faster.  The target function does not necessarily have to exist at the time when (alias) is called.  When the parser is searching for a function, if it finds an alias, the search is repeated with the new name.  The prototype needs to match for the alias to trigger, however, if you're too lazy to figure out what the right prototype is, you can use \"*.\" to have the alias match anything.  (The arguments will still need to match the target function or a type mismatch will occur.)  Warning: Use of star-dot can have unintended consequences, particularly related to overloaded functions with the same name as a star-dot function.  Use of star-dot is discouraged and should be used sparingly.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (RopRootParser, "par.rt.alias", "veasa", RopAliasRoot, "Give a new name to an existing function in the root parser", "(par.rt.alias [a:new name] [s:prototype] [a:existing name]) = [v]", "Adding an alias is functionally the same as defining a function that does nothing but call the existing function, except that the execution is many many times faster.  The target function does not necessarily have to exist at the time when (alias) is called.  When the parser is searching for a function, if it finds an alias, the search is repeated with the new name.  The prototype needs to match for the alias to trigger, however, if you're too lazy to figure out what the right prototype is, you can use \"*.\" to have the alias match anything.  (The arguments will still need to match the target function or a type mismatch will occur.)  Warning: Use of star-dot can have unintended consequences, particularly related to overloaded functions with the same name as a star-dot function.  Use of star-dot is discouraged and should be used sparingly.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (RopRootParser, "par.unalias", "vpeas", RopUnalias, "Removes an alias", "(par.unalias [a:name] [s:prototype]) = [v]", "The name and prototype must match exactly with what was passed to the (alias) function.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (RopRootParser, "par.rt.unalias", "veas", RopUnaliasRoot, "Removes an alias from the root parser", "(par.rt.unalias [a:name] [s:prototype]) = [v]", "The name and prototype must match exactly with what was passed to the (alias) function.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (RopRootParser, "par.helpwin", "vpe", RopHelpwin, "Brings up the help window for the current parser.", "(par.helpwin) = [v]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (RopRootParser, "par.rt.helpwin", "ve", RopHelpwinRoot, "Brings up the help window for the root parser.", "(par.rt.helpwin) = [v]", "");
	fail_if (!DERR_OK (st), st, 0);


	st = ParAddFunction (RopRootParser, "par.va", "ses.", RopVa, "Generic printf", "(par.va [s:format string] ...) = [s:formatted string]", "Format string and arguments should be standard printf form.  Note: Due to me being lazy, you will not be able to use %f in the format string.  If you must print floats, use %s and the (ftos) function.");
	fail_if (!DERR_OK (st), st, 0);
	

	st = ParAddAlias (RopRootParser, "begin", "vpec.", "par.begin"); fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (RopRootParser, "while", "vpeiaa", "par.while"); fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (RopRootParser, "if", "upeiaa", "par.if"); fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (RopRootParser, "va", "ses.", "par.va"); fail_if (!DERR_OK (st), st, 0);
	

	st = MthAddGlobalCommands (RopRootParser);
	fail_if (!DERR_OK (st), st, 0);

	st = ScrAddGlobalCommands (RopRootParser);
	fail_if (!DERR_OK (st), st, 0);

	st = ConAddGlobalCommands (RopRootParser);
	fail_if (!DERR_OK (st), st, 0);
	
	st = PfAddGlobalCommands (RopRootParser);
	fail_if (!DERR_OK (st), st, 0);

	st = PlugAddGlobalCommands (RopRootParser);
	fail_if (!DERR_OK (st), st, 0);

	st = InfAddGlobalCommands (RopRootParser);
	fail_if (!DERR_OK (st), st, 0);

	st = UtilAddGlobalCommands (RopRootParser);
	fail_if (!DERR_OK (st), st, 0);

	st = WatAddGlobalCommands (RopRootParser);
	fail_if (!DERR_OK (st), st, 0);

	return PF_SUCCESS;

failure:

	if (RopRootParser)
	{
		ParDestroy (RopRootParser);
		RopRootParser = NULL;
	}

	return st;

}

void RopDenit (void)
{
	ParDestroy (RopRootParser);
	RopRootParser = NULL;
}

DERR RopInherit (parser_t **out)
{
	int st;

	st = ParCreate (out, RopRootParser);

	return st;
}



void RopStressTestHelper (parerror_t *pe, int in)
{
	char *temp;

	switch (in)
	{
	case 1:
		temp = ParMalloc (25);
		if (!temp)
		{
			pe->error = STATUS_USER_DEFINED_ERROR;
			pe->format = "Out of memory, 25";
			
			return;
		}
		else
		{
			pe->error = STATUS_USER_DEFINED_ERROR;
			pe->format = "Holy shit %s\n";
			pe->param1 = (int)temp;
			pe->flags = PEF_PARAM1;

			strcpy (temp, "jexux!");

			return;
		}
	}
}


void RopParserStressTest (void)
{
	int origcount;
	parser_t *par = NULL;
	char *output = NULL;
	char error[1024];
	DERR st;
	int nesting;
	int runs = 0;

	
	Sleep (2000); //wait for startup to finish
	ConWriteStringf ("Commencing stress test\n");

	ParSetDegrade (5);

	while (1)
	{
		par = NULL;
		output = NULL;

		origcount = ParHeapCount ();

		st = RopInherit (&par);
		fail_if (!DERR_OK (st), st, 0);

		st = ParAddFunction (par, "stress", "vei", RopStressTestHelper, "", "", "");
		fail_if (!DERR_OK (st), st, 0);

		st = ParRunLine (par, "(+ 5 4)", &output, &nesting, error, 1024);
		if (output) ParFree (output);

		st = ParRunLine (par, "(stress 0)", &output, NULL, error, 1024);
		if (output) ParFree (output);

		st = ParRunLine (par, "(stress 1)", &output, NULL, error, 1024);
		if (output) ParFree (output);

		st = ParRunLine (par, "(va \"lala %i\" 34)", &output, NULL, error, 1024);
		if (output) ParFree (output);

		//(define r1 "ii;r1.param" '(if (== r1.param 0) '1 '(* (r1 (- r1.param 1)) r1.param)))
		st = ParRunLine (par, "(define r1 \"ii;r1.param\" '(if (== r1.param 0) '1 '(* (r1 (- r1.param 1)) r1.param)))", &output, NULL, error, 1024);
		if (output) ParFree (output);

		st = ParRunLine (par, "(define r1 \"ii;r1.param\" '(if (== r1.param 0) '1 '(* (r1 (- r1.param 1)) r1.param)))", &output, NULL, error, 1024);
		if (output) ParFree (output);

		st = ParRunLine (par, "(define r1 \"ii;r1.param\" '(if (== r1.param 0) '1 '(* (r1 (- r1.param 1)) r1.param)))", &output, NULL, error, 1024);
		if (output) ParFree (output);

		st = ParRunLine (par, "(define test2 \"ii;r1.param\" '(if (== r1.param 0) '(va \"%s\" 4) '(* (test2 (- r1.param 1)) r1.param)))", &output, NULL, error, 1024);
		if (output) ParFree (output);

		st = ParRunLine (par, "(test2 5)", &output, NULL, error, 1024);
		if (output) ParFree (output);

		st = ParRunLine (par, "(r1 5)", &output, NULL, error, 1024);
		if (output) ParFree (output);

		st = ParRunLine (par, "(undefine r1 \"ii\")", &output, NULL, error, 1024);
		if (output) ParFree (output);

		st = ParRunLine (par, "(begin '(stress 1) '(stress 1))", &output, NULL, error, 1024);
		if (output) ParFree (output);

		st = ParRunLine (par, "(begin '(# error.cont) '(stress 1) '(stress 1))", &output, NULL, error, 1024);
		if (output) ParFree (output);

		st = ParRunLine (par, "(va \"lala %s\" 34)", &output, NULL, error, 1024);
		if (output) ParFree (output);

		st = ParRemoveFunction (par, "stress", "vei");
		fail_if (!DERR_OK (st), st, 0);

failure:
		if (par)
			ParDestroy (par);

		runs++;

		if (origcount != ParHeapCount ())
		{
			ConWriteStringf ("Count failure after %i runs (%i != %i)\n", runs, origcount, ParHeapCount ());
			break;
		}

		if (runs % 10000 == 0)
		{
			ConWriteStringf ("%i runs: %i,%.8X\n", runs, origcount, st);
		}

	}

}


		




