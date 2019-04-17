/* Copyright 2008 wParam, licensed under the GPL */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SchExceptions
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "wp_scheme2.h"





struct sex_s
{
	atom_t atom;
	int type;

	struct sex_s *next;
	struct sex_s *down;
	struct sex_s *up;
	struct sex_s **self;
};

typedef struct sex_s sex_t;
#define TYPE_ERROR 0
#define TYPE_NULL 1
#define TYPE_VOID 2
#define TYPE_ATOM 3
#define TYPE_QUOTE 4
#define TYPE_STRING 5
#define TYPE_FUNC 6
#define TYPE_FLOAT 7
#define TYPE_HEX 8
#define TYPE_OCTAL 9
#define TYPE_INT 10
#define TYPE_END_OF_EXPRESSION 11 //essentially a null sex.  These are removed post parsing
#define TYPE_CONVERTED_ATOM 12	//this is assigned to an atom that was changed by the atom bind
								//function.  Bound things are considered strings unless they
								//are ints or floats.  (If you need defines to act like atoms,
								//you'll need a string to atom converted function.)
#define TYPE_TYPE 13
#define TYPE_MAXIMUM 14

//Maps the type codes above to the letter codes functions give in prototypes.
//The types here represent how the paramater is passed, so HEX, INT, and OCT are
//all listed as 'i' because they all represent integers.  This, incidentally, is why
//having 'h' in a prototype will result in a function that ALWAYS gives type mismatches.
//'h' is only somewhat useful as a return type.
//TYPE_NULL is listed as a +, which will match against either s or t
static char SchTypeCodes[] = "X+vaasXfiiiXXt";




#define fail_if(cond, status, other) if (cond) { st = status ; other ; /*printf ("line: %i\n", __LINE__);*/ goto failure ; }



#ifdef SchMalloc
void * SchMalloc (int);
#else
#define SchMalloc malloc
#endif

#ifdef SchFree
void SchFree (void *);
#else
#define SchFree free
#endif

#ifndef SchMemset
#define SchMemset memset
#endif

#ifndef SchMemcpy
#define SchMemcpy memcpy
#endif

#ifndef SchStrlen
#define SchStrlen strlen
#endif

#ifndef SchStrcpy
#define SchStrcpy strcpy
#endif

#ifndef SchStrcmp
#define SchStrcmp strcmp
#endif

#ifndef SchSscanf
#define SchSscanf sscanf
#endif

#ifndef SchSprintf
#define SchSnprintf _snprintf
#endif

#ifndef SCH_DEFAULT_ATOM_LEN
#define SCH_DEFAULT_ATOM_LEN 16
#endif


#define STATE_BASE 1
#define STATE_STRING 2
#define STATE_QUOTE 3
#define STATE_ATOM 4
#define STATE_QUOTE_SEX 5
#define STATE_QUOTE_SEX_STRING 6
#define STATE_TYPE 7
#define STATE_QUOTE_SEX_TYPE 8


struct scheme_s
{
	sex_t *root;
	sex_t *cur;

	int state;
	int nesting;
	int quotelevel;

	int (*findfunc)(char *name, char *proto, unsigned long, func_t **);
	int (*releasefunc)(func_t *, unsigned long); //called when the parser is done working with the func_t

	int (*atombind)(atom_t *, unsigned long, int *changed);
	unsigned long lParam;

	func_t *functions;

};

int SchGetNesting (scheme_t *sch)
{
	return sch->nesting;
}

void SchSetFindFunc (scheme_t *sch, int (*f)(char *, char *, unsigned long, func_t **))
{
	sch->findfunc = f;
}

void SchSetReleaseFunc (scheme_t *sch, int (*f)(func_t *, unsigned long))
{
	sch->releasefunc = f;
}

void SchSetAtomBind (scheme_t *sch, int (*a)(atom_t *, unsigned long, int *changed))
{
	sch->atombind = a;
}

void SchSetParam (scheme_t *sch, unsigned long lParam)
{
	sch->lParam = lParam;
}

unsigned long SchGetParam (scheme_t *sch)
{
	return sch->lParam;
}


//if this function returns success, the result can (and should) be passed to SchFreeSex
static int SchAllocateSex (sex_t **sexout)
{
	int st = STATUS_UNKNOWN_ERROR;
	sex_t *out = NULL;

	out = SchMalloc (sizeof (sex_t));
	fail_if (!out, STATUS_OUT_OF_MEMORY, 0);

	SchMemset (out, 0, sizeof (sex_t));

	*sexout = out;
	return STATUS_SUCCESS;

failure:
	if (out)
		SchFree (out);

	return st;
}

static int SchFreeSex (sex_t *sex)
{
	if (sex->atom.a)
		SchFree (sex->atom.a);

	SchFree (sex);

	return STATUS_SUCCESS;
}

static int SchFreeSexTree (sex_t *root)
{
	if (!root)
		return STATUS_SUCCESS;

	SchFreeSexTree (root->down);
	SchFreeSexTree (root->next);

	SchFreeSex (root);

	return STATUS_SUCCESS;
}





int SchCreate (scheme_t **schout)
{
	scheme_t *out;
	int st = STATUS_UNKNOWN_ERROR;

	out = SchMalloc (sizeof (scheme_t));
	fail_if (!out, STATUS_OUT_OF_MEMORY, 0);

	SchMemset (out, 0, sizeof (scheme_t));

	out->state = STATE_BASE;

	*schout = out;

	return STATUS_SUCCESS;

failure:
	if (out)
		SchFree (out);

	return st;
}


int SchReset (scheme_t *sch)
{
	//note: if you change this function, you must also update SchSaveExpression to match
	SchFreeSexTree (sch->root);

	sch->root = sch->cur = NULL;

	sch->nesting = sch->quotelevel = 0;
	
	sch->state = STATE_BASE;

	return STATUS_SUCCESS;
}

int SchDestroy (scheme_t *sch)
{
	SchReset (sch);

	SchFree (sch);

	return STATUS_SUCCESS;
}

int SchSaveExpression (scheme_t *in, scheme_t **outval)
{
	scheme_t *out;
	int st = STATUS_UNKNOWN_ERROR;

	out = SchMalloc (sizeof (scheme_t));
	fail_if (!out, STATUS_OUT_OF_MEMORY, 0);

	memcpy (out, in, sizeof (scheme_t));

	//we can't use SchReset because that destroys the tree, so we have to just duplicate
	//the code here.  Make sure this and SchReset stay in sync
	in->root = in->cur = NULL;
	in->nesting = in->quotelevel = 0;
	in->state = STATE_BASE;

	*outval = out;

	return STATUS_SUCCESS;
failure:
	*outval = NULL;

	return st;
}

int SchRestoreExpression (scheme_t *targ, scheme_t *saved)
{

	//reset the expression in case there's some half-processed crap in there.
	SchReset (targ);

	//now copy the old stuff back.
	memcpy (targ, saved, sizeof (scheme_t));

	//and free the saved.
	SchFree (saved);

	return STATUS_SUCCESS;
}



static int SchAppendChar (atom_t *atom, char c)
{
	int st = STATUS_UNKNOWN_ERROR;
	char *temp = NULL;

	if (!atom->a)
	{
		atom->a = SchMalloc (SCH_DEFAULT_ATOM_LEN);
		fail_if (!atom->a, STATUS_OUT_OF_MEMORY, 0);

		atom->len = 0;
		atom->size = SCH_DEFAULT_ATOM_LEN;
	}

	if (atom->len == atom->size) //need to realloc
	{
		temp = SchMalloc (atom->size * 2);
		fail_if (!temp, STATUS_OUT_OF_MEMORY, 0);

		SchMemcpy (temp, atom->a, atom->len);
		SchFree (atom->a);
		atom->a = temp;
		atom->size *= 2;
	}

	atom->a[atom->len] = c;
	atom->len++;

	return STATUS_SUCCESS;
failure:

	if (temp)
		SchFree (temp);

	return st;
}

static int SchSetAtomLength (atom_t *atom, int newsize)
{
	char *temp = NULL;
	int st = STATUS_UNKNOWN_ERROR;

	if (!atom->a)
	{
		atom->a = SchMalloc (newsize);
		fail_if (!atom->a, STATUS_OUT_OF_MEMORY, 0);

		SchMemset (atom->a, 0, newsize);

		atom->len = 0;
		atom->size = newsize;

	}

	if (newsize > atom->size)
	{
		temp = SchMalloc (newsize);
		fail_if (!temp, STATUS_OUT_OF_MEMORY, 0);

		SchMemcpy (temp, atom->a, atom->len);
		SchFree (atom->a);
		atom->a = temp;
		atom->size = newsize;
	}

	return STATUS_SUCCESS;
failure:
	if (temp)
		SchFree (temp);

	return st;
}




/****************************************************************

					Execution

****************************************************************/

//meta parameters are things that called functions get that aren't given in the
//s-expression itself.  The total count of parameters, the error_t are examples.
static int SchIsMetaParameter (char test)
{
	if (test == 'c' || test == 'e' || test == 'p' || test == 'n')
		return 1;
	return 0;
}

static int SchCountParameters (char *proto, sex_t *expr)
{
	int out = 0;

	while (*proto)
	{
		//e or c in the prototype will give a param that doesn't come from expr.
		if (SchIsMetaParameter (*proto))
			out++;

		proto++;
	}

	//the first thing in the expression is the function we're calling; not a param
	expr = expr->next;

	while (expr)
	{
		out++;

		expr = expr->next;
	}

	return out;
}

static int SchDigitVal (char d)
{
	if (d >= '0' && d <= '9')
		return d - '0';

	if (d >= 'a' && d <= 'f')
		return (d - 'a') + 10;

	if (d >= 'A' && d <= 'F')
		return (d - 'A') + 10;

	return -1;
}

static int SchConvertInt (char *i, int base)
{
	int out = 0;
	int sign = 1;
	int val;

	if (!i)
		return 0;

	if (*i == '-')
	{
		sign *= -1;
		i++;
	}

	while (*i)
	{
		val = SchDigitVal (*i);

		if (val == -1)
			break;

		out *= base;
		out += val;

		i++;
	}

	return out * sign;
}
		

static int SchIsFloat (char *atom);
static int SchIsInteger (char *atom);
static int SchIsOctal (char *atom);
static int SchIsHex (char *atom);
static int SchMakeFunctionCall (scheme_t *sch, sex_t *expr, func_t *func, atom_t *out, int *type, error_t *err)
{
	void **params = NULL;
	int paramcount;
	int x;
	sex_t *walk = expr;
	void *retval;
	int st;
	int validproto = 1;

	fail_if (!sch || !expr || !func || !out || !type || !err, STATUS_INTERNAL_ERROR, 0);

	paramcount = SchCountParameters (func->prototype, expr);
	params = SchMalloc (sizeof (void *) * paramcount);
	fail_if (!params, STATUS_OUT_OF_MEMORY, 0);

	x = 0;
	walk = walk->next;
	while (1)
	{
		if (x >= paramcount)
			break;//fail_if (x >= paramcount, STATUS_INTERNAL_ERROR, 0);

		if (validproto)
		{
			if (func->prototype[x + 1] == 'c')
			{
				params[x] = (void *)paramcount;
				x++;
				continue;
			}

			if (func->prototype[x + 1] == 'e')
			{
				params[x] = err;
				x++;
				continue;
			}

			if (func->prototype[x + 1] == 'p')
			{
				params[x] = (void *)sch->lParam;
				x++;
				continue;
			}

			if (func->prototype[x + 1] == 'n')
			{
				params[x] = func;
				x++;
				continue;
			}

			if (func->prototype[x + 1] == '.')
				validproto = 0; //prevent accesses past end of string
		}



		//we check here, and not in the while condition, because we need things to work if a function
		//has c or e at the end of its prototype.  This will happen, for example, if a void function
		//needs to return error information.
		if (!walk)
			break;

		switch (walk->type)
		{
		case TYPE_NULL:
		case TYPE_VOID: //I'm not entirely sure functions can accept params like this
			params[x] = NULL;
			break;

		case TYPE_INT:
			params[x] = (void *)SchConvertInt (walk->atom.a, 10);
			break;

		case TYPE_OCTAL:
			params[x] = (void *)SchConvertInt (walk->atom.a + 1, 8);
			break;

		case TYPE_HEX:
			params[x] = (void *)SchConvertInt (walk->atom.a + 2, 16);
			break;

		case TYPE_FLOAT:
			SchSscanf (walk->atom.a, "%f", &params[x]);
			break;

		case TYPE_STRING:
		case TYPE_ATOM:
		case TYPE_QUOTE:
		case TYPE_TYPE:
			params[x] = walk->atom.a;
			break;

		default:
			fail_if (1, STATUS_INTERNAL_ERROR, 0);
		}

		walk = walk->next;
		x++;
	}

	fail_if (x != paramcount || walk, STATUS_INTERNAL_ERROR, 0);

#ifdef SchExceptions
	__try
	{
#endif
		//push the params on the stack in reverse order
		for (x=paramcount - 1;x >= 0 ; x--)
		{
			retval = params[x];
			__asm push retval;
		}

		//do the call and capture the return address
		retval = func->address;
		__asm
		{	
			call retval;
			mov retval, eax
		}

		//if it's a float function, capture the float return (it doesn't come in eax)
		if (func->prototype[0] == 'f')
			__asm fst retval;

		//pop the params off the stack
		for (x=paramcount - 1;x >= 0 ; x--)
		{
			__asm pop eax;
		}
#ifdef SchExceptions
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		//we're catching everything because I don't want everything left in a craptastic
		//state.  The stack will have already been cleared up.
		fail_if (1, STATUS_EXCEPTION_OCCURED, err->info = GetExceptionCode ());
	}
#endif

	//check to see if there was an error, if so, abort like any other error
	fail_if (err->error, err->error, 0);

	//now convert the return value
	switch (func->prototype[0])
	{
	case 'i':
		st = SchSetAtomLength (out, 40);
		fail_if (st, st, 0);
		SchSnprintf (out->a, 39, "%i", retval);
		out->a[39] = '\0';
		out->len = strlen (out->a) + 1;
		*type = TYPE_INT;
		break;

	case 'h':
		st = SchSetAtomLength (out, 40);
		fail_if (st, st, 0);
		SchSnprintf (out->a, 39, "0x%X", retval);
		out->a[39] = '\0';
		out->len = strlen (out->a) + 1;
		*type = TYPE_HEX;
		break;

	case 'f':
		st = SchSetAtomLength (out, 40);
		fail_if (st, st, 0);
		SchSnprintf (out->a, 39, "%f", *((float *)&retval));
		out->a[39] = '\0';
		out->len = strlen (out->a) + 1;
		*type = TYPE_FLOAT;
		break;

	case 's':
		if (out->a)
			SchFree (out->a);
		out->a = retval;
		out->size = out->len = retval ? strlen (retval) + 1 : 0;
		if (retval)
			*type = TYPE_STRING;
		else
			*type = TYPE_NULL;
		break;

	case 't':
		if (out->a)
			SchFree (out->a);
		out->a = retval;
		out->size = out->len = retval ? strlen (retval) + 1 : 0;
		if (retval)
			*type = TYPE_TYPE;
		else
			*type = TYPE_NULL;
		break;

	case 'a':
		fail_if (!retval, STATUS_INVALID_ATOM_RETURNED, 0);
		if (out->a)
			SchFree (out->a);
		out->a = retval;
		out->size = out->len = strlen (retval) + 1;
		*type = TYPE_ATOM;
		break;

	case 'u': //unclassified
		if (out->a)
			SchFree (out->a);
		out->a = retval;
		out->size = out->len = retval ? strlen (retval) + 1 : 0;

		if (!retval)
			*type = TYPE_NULL;
		else if (SchIsHex (retval))
			*type = TYPE_HEX;
		else if (SchIsOctal (retval))
			*type = TYPE_OCTAL;
		else if (SchIsInteger (retval))
			*type = TYPE_INT;
		else if (SchIsFloat (retval))
			*type = TYPE_FLOAT;
		else
			*type = TYPE_STRING;

		break;

	case 'v':
		*type = TYPE_VOID;
		break;

	default:
		fail_if (1, STATUS_INVALID_RETURN_TYPE, 0);
	}

	SchFree (params);
	return STATUS_SUCCESS;

	
failure:
	if (params)
		SchFree (params);

	return st;
}



int SchAddFunction (scheme_t *sch, char *name, char *prototype, void *address)
{
	func_t *f = NULL;
	func_t **walk;
	int st = STATUS_UNKNOWN_ERROR;

	f = SchMalloc (sizeof (func_t));
	fail_if (!f, STATUS_OUT_OF_MEMORY, 0);

	f->address = address;
	f->next = NULL;
	f->name = f->prototype = NULL; //needed for error recovery

	f->name = SchMalloc (SchStrlen (name) + 1);
	fail_if (!f->name, STATUS_OUT_OF_MEMORY, 0);
	SchStrcpy (f->name, name);

	f->prototype = SchMalloc (SchStrlen (name) + 1);
	fail_if (!f->prototype, STATUS_OUT_OF_MEMORY, 0);
	SchStrcpy (f->prototype, prototype);

	//go to the trouble of putting it at the end of the list.
	//this is so common things like + or whatever can be found quickly

	walk = &sch->functions;
	while (*walk)
		walk = &((*walk)->next);
	*walk = f;

	return STATUS_SUCCESS;

failure:
	if (f && f->name)
		SchFree (f->name);

	if (f && f->prototype)
		SchFree (f->prototype);

	if (f)
		SchFree (f);

	return st;

}

int SchEqualPrototype (char *p1, char *p2)
{
	while (1)
	{
		//c and e aren't parameters until the function is called, they're ignored
		//for the purposes of type matching.  Do this check before the \0 check so
		//that it will work correctly if c or e appear at the end of the string
		while (SchIsMetaParameter (*p1))
			p1++;
		while (SchIsMetaParameter (*p2))
			p2++;

		if (*p1 == '.' || *p2 == '.')
			return 1; //if they match up to a ., then we call it a match; vararg functions are assumed to match.

		if (!*p1 && !*p2)
			return 1; //match

		if (!*p1 || !*p2)
			return 0; //no match.  (this is really testing for if one is null and the other isn't, because
						//we already know they aren't both null.

		//if either one is a *, skip this character.  * is used for the return type
		//since it isn't known at the point where FindFunction is called
		if (*p1 == '*' || *p2 == '*')
		{
			p1++;
			p2++;
			continue;
		}

		//The + character is NULL, it needs to match either s or t
		if ((*p1 == '+' && (*p2 == 's' || *p2 == 't')) ||
			(*p2 == '+' && (*p1 == 's' || *p1 == 't')) ) //+ matches s or t
		{
			p1++;
			p2++;
			continue;
		}


		if (*p1 != *p2)
			return 0;

		p1++;
		p2++;
	}

	//can't be reached
	return 0;
}
		


static int SchFindFunction (scheme_t *sch, char *funcname, char *prototype, int *external, func_t **out)
{
	int res;
	func_t *temp;

	if (sch->findfunc)
	{
		res = sch->findfunc (funcname, prototype, sch->lParam, &temp);
		if (res == FF_FOUND)
		{
			*out = temp;
			*external = 1;
			return STATUS_SUCCESS;
		}
		else if (res == FF_TYPEMISMATCH)
		{
			//leave external 0, it is a flag used for unlocking only
			return STATUS_TYPE_MISMATCH;
		}

		//else res == FF_NOTFOUND, and we look to the internal ones.
	}

	res = STATUS_NOT_APPLICABLE;
	temp = sch->functions;
	while (temp)
	{
		if (SchStrcmp (temp->name, funcname) == 0)
		{
			//found a function with the given name, so if we end up with no match
			//it's a type error instead of a not found error
			res = STATUS_TYPE_MISMATCH;
			if (SchEqualPrototype (temp->prototype, prototype))
			{
				*out = temp;
				*external = 0;
				return STATUS_SUCCESS;
			}
		}

		temp = temp->next;
	}

	return res;
}



static int SchBuildPrototype (sex_t *expr, atom_t *atom)
{
	int st = STATUS_UNKNOWN_ERROR;

	fail_if (!atom || !expr, STATUS_INTERNAL_ERROR, 0);

	st = SchAppendChar (atom, '*');
	fail_if (st, st, 0);
	expr = expr->next; //skip the first one; it;s always the function name

	while (expr)
	{
		fail_if (expr->type >= TYPE_MAXIMUM || SchTypeCodes[expr->type] == 'X', STATUS_INTERNAL_ERROR, 0);

		st = SchAppendChar (atom, SchTypeCodes[expr->type]);
		fail_if (st, st, 0);

		expr = expr->next;
	}

	st = SchAppendChar (atom, '\0');
	fail_if (st, st, 0);

	return STATUS_SUCCESS;

failure:
	//don't need to free because it will be freed when the sex tree is deleted.
	//Plus, who knows if we were actually the one who allocated it.
	return st;
}

static int SchExecuteFunction (scheme_t *sch, sex_t *expr, atom_t *out, int *type, error_t *err)
{
	int st = STATUS_UNKNOWN_ERROR;
	func_t *func = NULL;
	int external = 0;
	atom_t proto = {0};

	fail_if (!sch || !expr || !out || !type, STATUS_INTERNAL_ERROR, 0);

	if (expr->type != TYPE_ATOM)
		return STATUS_NOT_APPLICABLE;

	st = SchBuildPrototype (expr, &proto);
	fail_if (st, st, 0);

	st = SchFindFunction (sch, expr->atom.a, proto.a, &external, &func);
	fail_if (st, st, 0);

	//at this point, if external is 1 and the external logic requires locking,
	//the func_t is locked in place and needs to be released with the
	//releasefunc defined in the scheme_t, if any.  (If that func is null,
	//no locking is required and we don't have to worry about it.

	//we do our own check on the prototype.  External findfunc routines are not
	//required to differentiate functions based on their type, so the prototype
	//compare function may not have been called.

	st = SchEqualPrototype (func->prototype, proto.a);
	fail_if (!st, STATUS_TYPE_MISMATCH, 0);

	st = SchMakeFunctionCall (sch, expr, func, out, type, err);
	fail_if (st, st, 0);

	if (external && sch->releasefunc)
		sch->releasefunc (func, sch->lParam);

	if (proto.a)
		SchFree (proto.a);

	return STATUS_SUCCESS;


failure:

	if (external && func && sch->releasefunc)
		sch->releasefunc (func, sch->lParam);

	if (proto.a)
		SchFree (proto.a);

	return st;
}


static int SchApplyExpression (scheme_t *sch, sex_t *expr, atom_t *out, int *type, error_t *err)
{
	sex_t *walk;
	int st = STATUS_UNKNOWN_ERROR;

	walk = expr;
	while (walk)
	{
		//evaluate all sub functions.
		if (walk->type == TYPE_FUNC)
		{
			fail_if (!walk->down, STATUS_INTERNAL_ERROR, 0);
			st = SchApplyExpression (sch, walk->down, &walk->atom, &walk->type, err);
			fail_if (st, st, 0);
			fail_if (walk->type == TYPE_FUNC, STATUS_INTERNAL_ERROR, 0);
		}

		walk = walk->next;
	}

	//if (expr != sch->root)
		st = SchExecuteFunction (sch, expr, out, type, err);
	//else
	//	st = STATUS_SUCCESS;

	return st;
failure:

	return st;
}







//Don't call this if sch->atombind is null
static int SchBindAtoms (scheme_t *sch, sex_t *root)
{
	int st;
	int changed = 0;

	if (!root)
		return STATUS_SUCCESS;

	//things typed as 'quote' or 'string' are not passed here
	if (root->type == TYPE_ATOM)
	{
		st = sch->atombind (&root->atom, sch->lParam, &changed);
		fail_if (st, st, 0);
	}

	if (changed)
		root->type = TYPE_CONVERTED_ATOM;

	st = SchBindAtoms (sch, root->down);
	fail_if (st, st, 0);

	st = SchBindAtoms (sch, root->next);
	fail_if (st, st, 0);

	return STATUS_SUCCESS;

failure:
	return st;
}

static int SchIsHex (char *atom)
{
	if (atom[0] != '0')
		return 0;

	if (atom[1] != 'x' && atom[1] != 'X')
		return 0;

	atom += 2;

	while (*atom)
	{
		if (!(*atom >= '0' && *atom <= '9') &&
			!(*atom >= 'A' && *atom <= 'F') &&
			!(*atom >= 'a' && *atom <= 'f'))
			return 0;

		atom++;
	}

	return 1;
}

static int SchIsOctal (char *atom)
{
	if (atom[0] != '0')
		return 0;

	atom += 1;

	while (*atom)
	{
		if (!(*atom >= '0' && *atom <= '7'))
			return 0;

		atom++;
	}

	return 1;
}

static int SchIsInteger (char *atom)
{
	int foundnum = 0;
	while (*atom)
	{
		if ((*atom > '9' || *atom < '0') && *atom != '-')
			return 0;
		if (*atom != '-')
			foundnum = 1;
		atom++;
	}
	if (foundnum)
		return 1;

	//- by itself should be an atom
	return 0;
}

static int SchIsFloat (char *atom)
{
	int foundnum = 0;
	int numdec = 0;
	while (*atom)
	{
		if ((*atom > '9' || *atom < '0') && *atom != '.' && *atom != '-')
			return 0;
		if (*atom >= '0' && *atom <= '9')
			foundnum = 1;
		if (*atom == '.')
			numdec++;
		atom++;
	}

	if (numdec > 1)
		return 0; /* Too many decimal points */

	if (foundnum)
		return 1;

	return 0;
}

	

static int SchClassifySex (sex_t *root)
{
	int st;

	if (!root)
		return STATUS_SUCCESS;

	st = SchClassifySex (root->down);
	fail_if (st, st, 0);

	st = SchClassifySex (root->next);
	fail_if (st, st, 0);

	if (!root->atom.a) //double check this
		return STATUS_SUCCESS;

	if (root->type == TYPE_ATOM || root->type == TYPE_CONVERTED_ATOM)
	{
		//strings and quoted things already have their type set accordingly.
		//We just have to check and see what's actually an int

		if (SchIsHex (root->atom.a))
			root->type = TYPE_HEX;
		else if (SchIsOctal (root->atom.a))
			root->type = TYPE_OCTAL;
		else if (SchIsInteger (root->atom.a))
			root->type = TYPE_INT;
		else if (SchIsFloat (root->atom.a))
			root->type = TYPE_FLOAT;

		//if the type is still 'converted atom', then mark it as a string.
		//defines will not become atoms on their own.
		if (root->type == TYPE_CONVERTED_ATOM)
			root->type = TYPE_STRING;
	}

	return STATUS_SUCCESS;
failure:
	return st;
}





/****************************************************************

					Parsing

****************************************************************/

static int SchIsWhitespace (char a)
{
	return (a == ' ') || (a == '\t') || (a == '\r') || (a == '\n');
}

//specifically, any char that will end an atom
static int SchIsSpecialChar (char a)
{
	return (a == '\"') || (a == '\'') || (a == '(') || (a == ')') || (a == '`');
}

static int SchEndCurrent (sex_t **current)
{
	sex_t *n = NULL;
	int res;
	int st = STATUS_UNKNOWN_ERROR;

	res = SchAllocateSex (&n);
	fail_if (res, res, 0);

	n->type = TYPE_END_OF_EXPRESSION;
	n->self = &(*current)->next;
	n->up = (*current)->up;
	(*current)->next = n;
	*current = n;

	return STATUS_SUCCESS;

failure:
	if (n)
		SchFreeSex (n);

	return st;
}





static int SchPostParseProcessing (sex_t *root)
{
	int st = STATUS_UNKNOWN_ERROR;
	int res;

	if (!root)
		return STATUS_SUCCESS;

	if (root->type == TYPE_END_OF_EXPRESSION)
	{
		//sanity check: don't leak anything, there shouldn't be an atom, and don't crash on root->Self set
		fail_if (root->atom.a || root->next || root->down || !root->self, STATUS_INTERNAL_ERROR, 0);

		//remove it from teh list
		*root->self = NULL;

		res = SchFreeSex (root);
		fail_if (res, res, 0);

		return STATUS_SUCCESS;
	}

	res = SchPostParseProcessing (root->down);
	fail_if (res, res, 0);

	res = SchPostParseProcessing (root->next);
	fail_if (res, res, 0);

	//if this was a function, but it now has no ->down (because it was TYPE_END_OF_EXPRESSION
	//and was removed) set the type to TYPE_NULL.  The end result of this is that () evaluates
	//to a NULL string pointer.  ("()" should be the only way to get null passed to a function)

	if (root->type == TYPE_FUNC && !root->down)
		root->type = TYPE_NULL;

	//if an atom is defined, add a null terminator
	//fix: If the line parsed contains an empty string (""), atom.a will be null, but
	//this is NOT what we want to happen.  In this case we will force a null char on the
	//end.
	if (root->atom.a || root->type == TYPE_STRING)
	{
		res = SchAppendChar (&root->atom, '\0');
		fail_if (res, res, 0);
	}

	//A quote character with nothing after it is always an error.  Likewise, a type string
	//that contains nothing (``) is considered to always be an error, because types need
	//to encode what their type is at minimum.  (A zero-length type still has characters in
	//it.)
	fail_if (root->type == TYPE_TYPE && !root->atom.a, STATUS_DEGENERATE_TYPE, 0);
	fail_if (root->type == TYPE_QUOTE && !root->atom.a, STATUS_INVALID_QUOTE, 0);

	return STATUS_SUCCESS;

failure:
	return st;
}




static int SchParseLineBase (scheme_t *sch, char *line, int *info, int *advance, int pos)
{
	sex_t *temp = NULL;
	int st = STATUS_UNKNOWN_ERROR;
	int res;

	if (SchIsWhitespace (*line))
	{
		*advance = 1;
		return STATUS_SUCCESS;
	}

	if (*line == '(')
	{
		//begin a sub-expression.

		res = SchAllocateSex (&temp);
		fail_if (res, res, 0);

		temp->up = sch->cur;
		temp->self = &sch->cur->down;
		temp->type = TYPE_END_OF_EXPRESSION;

		sch->cur->type = TYPE_FUNC;

		sch->cur->down = temp;
		sch->cur = temp;

		sch->nesting++;

		*advance = 1; //pass the (
	}
	else if (*line == ')')
	{
		//end a sub expression
		fail_if (!sch->nesting, STATUS_SYNTAX_ERROR, *info = pos);

		//ok.  the STATE_ATOM or whatever will have already allocated a new
		//sex_t at the end of the current level of type END_OF_EXPRESSION.
		//Move that from the end of this expression to the next of the parent
		//expression and set cur to it.

		*sch->cur->self = NULL; //remove it from the list it was in
		sch->cur->up->next = sch->cur;
		sch->cur->self = &sch->cur->up->next;
		sch->cur->up = sch->cur->up->up;

		sch->nesting--;

		*advance = 1; //skip the )
	}
	else if (*line == '\"')
	{
		// a string.
		sch->state = STATE_STRING;

		sch->cur->type = TYPE_STRING;
		
		//everything else is the same.  sch->cur will receive the string.
		*advance = 1; //pass the "
	}
	else if (*line == '`')
	{
		//a "type".  Processed like a string
		sch->state = STATE_TYPE;

		sch->cur->type = TYPE_TYPE;

		*advance = 1;
	}
	else if (*line == '\'')
	{
		sch->cur->type = TYPE_QUOTE;

		//quoted something.  STATE_QUOTE deals with both '() and 'atom
		if (*(line + 1) == '(')
			sch->state = STATE_QUOTE_SEX;
		else
			sch->state = STATE_QUOTE;

		*advance = 1; //the ' is not part of the expression
	}
	else
	{
		//must be an atom
		sch->state = STATE_ATOM;

		sch->cur->type = TYPE_ATOM;

		*advance = 0; //DO NOT skip any characters; the char we're on is part of the atom.
	}

	return STATUS_SUCCESS;

failure:
	if (temp)
		SchFreeSex (temp);

	return st;

}


static int SchParseLineString (scheme_t *sch, char *line, int *info, int *advance, int pos) 
{
	int st = STATUS_UNKNOWN_ERROR;
	int res;
	char next;

	if (*line == '\"')
	{
		
		res = SchEndCurrent (&sch->cur);
		fail_if (res, res, 0);
		sch->state = STATE_BASE;
		*advance = 1; //pass the end of the string
		return STATUS_SUCCESS;
	}

	next = *line;
	if (*line == '\\')
	{
		line++;
		if (*line == '\"') // \" found in a string.  If the \ is at the end of the line, this won't trigger,
		{
			(*advance)++;								  // a backslash will appear in the string.
			next = '\"';
		}
		else if (*line == 'r')
		{
			(*advance)++;
			next = '\r';
		}
		else if (*line == 't')
		{
			(*advance)++;
			next = '\t';
		}
		else if (*line == 'n')
		{
			(*advance)++;
			next = '\n';
		}
		else if (*line == '\\')
		{
			(*advance)++;
			next = '\\';
		}
	}

	res = SchAppendChar (&sch->cur->atom, next);
	fail_if (res, res, 0);

	(*advance)++;

	return STATUS_SUCCESS;

failure:
	return st;

}

static int SchParseLineType (scheme_t *sch, char *line, int *info, int *advance, int pos)
{
	int st = STATUS_UNKNOWN_ERROR;
	int res;

	if (*line == '`')
	{
		res = SchEndCurrent (&sch->cur);
		fail_if (res, res, 0);
		sch->state = STATE_BASE;
		*advance = 1; //pass the `
		return STATUS_SUCCESS;
	}

	res = SchAppendChar (&sch->cur->atom, *line);
	fail_if (res, res, 0);

	*advance = 1;

	return STATUS_SUCCESS;

failure:
	return st;
}
	


static int SchParseLineQuote (scheme_t *sch, char *line, int *info, int *advance, int pos) 
{
	int st = STATUS_UNKNOWN_ERROR;
	int res;

	if (SchIsWhitespace (*line) || SchIsSpecialChar (*line))
	{	
		res = SchEndCurrent (&sch->cur);
		fail_if (res, res, 0);

		sch->state = STATE_BASE;

		//don't advance
		return STATUS_SUCCESS;
	}

	res = SchAppendChar (&sch->cur->atom, *line);
	fail_if (res, res, 0);

	*advance = 1;

	return STATUS_SUCCESS;

failure:
	return st;

}

static int SchParseLineAtom (scheme_t *sch, char *line, int *info, int *advance, int pos) 
{
	int st = STATUS_UNKNOWN_ERROR;
	int res;

	if (SchIsWhitespace (*line) || SchIsSpecialChar (*line))
	{
		
		res = SchEndCurrent (&sch->cur);
		fail_if (res, res, 0);

		sch->state = STATE_BASE;

		//don't advance
		return STATUS_SUCCESS;
	}

	res = SchAppendChar (&sch->cur->atom, *line);
	fail_if (res, res, 0);

	*advance = 1;

	return STATUS_SUCCESS;

failure:
	return st;

}


static int SchParseLineQuoteSex (scheme_t *sch, char *line, int *info, int *advance, int pos) 
{
	int st = STATUS_UNKNOWN_ERROR;
	int res;

	if (*line == '(')
	{
		sch->quotelevel++;
	}

	if (*line == '\"')
	{
		//need to handle this specially
		sch->state = STATE_QUOTE_SEX_STRING;

		//fall through is ok
	}

	if (*line == '`')
	{
		//same deal, we need to not process (, ), or " if we're in a TYPE
		sch->state = STATE_QUOTE_SEX_TYPE;
		//fall through
	}

	//if they're closing when they haven't opened, fail.
	//on second thought, I don't think this can happen, because the string "')fnjrg" will land you
	//in STATE_QUOTE, not STATE_QUOTE_SEX
	//leaving the check in just for sanity.
	fail_if (*line == ')' && sch->quotelevel <= 0, STATUS_SYNTAX_ERROR, *info = pos);

	res = SchAppendChar (&sch->cur->atom, *line);
	fail_if (res, res, 0);

	if (*line == ')')
	{
		if (sch->quotelevel == 1) //the end of the '()
		{
			res = SchEndCurrent (&sch->cur);
			fail_if (res, res, 0);

			sch->state = STATE_BASE;
		}

		sch->quotelevel--;
	}

	*advance = 1;

	return STATUS_SUCCESS;
	
failure:
	return st;

}

static int SchParseLineQuoteSexType (scheme_t *sch, char *line, int *info, int *advance, int pos)
{
	int st = STATUS_UNKNOWN_ERROR;
	int res;

	if (*line == '`')
	{
		//end of doodad; there is no quoting in types
		sch->state = STATE_QUOTE_SEX;
		//fall through
	}

	res = SchAppendChar (&sch->cur->atom, *line);
	fail_if (res, res, 0);

	*advance = 1;

	return STATUS_SUCCESS;

failure:
	return st;
}

static int SchParseLineQuoteSexString (scheme_t *sch, char *line, int *info, int *advance, int pos) 
{
	int st = STATUS_UNKNOWN_ERROR;
	int res;

	if (*line == '\\' && *(line + 1) == '\\')
	{
		//when we find \\, we need the result to have \\ in it.
		//Also, we need to NOT let the logic in the next branch of this if
		//run/

		res = SchAppendChar (&sch->cur->atom, *line);
		fail_if (res, res, 0);

		(*advance)++;
		line++;
	}
	else if (*line == '\\' && *(line + 1) == '\"')
	{
		//for a quoted " inside a string, add the \ here, then advance line.
		//Both the \ and the " need to be in the result.

		res = SchAppendChar (&sch->cur->atom, *line);
		fail_if (res, res, 0);

		(*advance)++;
		line++;
	}
	else if (*line == '\"')
	{
		//this needs to be in an else if clause because we don't want it to happen
		//if the first branch of this if hits.
		sch->state = STATE_QUOTE_SEX;

		//fall through
	}

	res = SchAppendChar (&sch->cur->atom, *line);
	fail_if (res, res, 0);

	(*advance)++;

	return STATUS_SUCCESS;

failure:
	return st;

}


static int (*SchStateFunctions[])(scheme_t *sch, char *line, int *info, int *advance, int pos) =
{
	NULL,
	SchParseLineBase,
	SchParseLineString,
	SchParseLineQuote,
	SchParseLineAtom,
	SchParseLineQuoteSex,
	SchParseLineQuoteSexString,
	SchParseLineType,
	SchParseLineQuoteSexType,
};
#define NUM_STATES (sizeof (SchStateFunctions) / sizeof (void *))


int SchParseLine (scheme_t *sch, char *linein, error_t *err, char **out)
{
	int res;
	int st = STATUS_UNKNOWN_ERROR, info = 0;
	char *line = linein;
	error_t dummyerror;
	int advance;

	if (!err)
		err = &dummyerror;

	fail_if (!sch, STATUS_INVALID_INTERPRETER, 0);

	if (!sch->root)
	{
		res = SchAllocateSex (&sch->root);
		fail_if (res, res, 0);
		sch->cur = sch->root;
	}

	while (*line)
	{
		fail_if (sch->state >= NUM_STATES || !SchStateFunctions[sch->state], STATUS_INTERNAL_ERROR, 0);

		advance = 0;
		res = SchStateFunctions[sch->state] (sch, line, &info, &advance, line - linein);
		fail_if (res, res, 0);

		//note:  if you return > 1 in the advance variable, it is up to the state function to make sure
		//that it isn't advanced passed the null.  (i.e. don't return 2 in advance if *line == '\0'

		line += advance;
	}


	if (sch->nesting) //can't use fail_if because we need to NOT call SchReset
	{
		err->error = STATUS_NEED_MORE_DATA;
		err->info = sch->nesting;

		*out = NULL;

		return STATUS_NEED_MORE_DATA;
	}

	//null termiate all the atoms, remove any END_OF_EXPRESSIONs, make () actually mean TYPE_NULL
	res = SchPostParseProcessing (sch->root);
	fail_if (res, res, 0);

	if (sch->atombind)	
	{
		res = SchBindAtoms (sch, sch->root);
		fail_if (res, res, 0);
	}

	res = SchClassifySex (sch->root);
	fail_if (res, res, 0);

	//ok.  It is go time.
	if (sch->root->type == TYPE_FUNC) //if root needs to be resolved, do it.
	{
		fail_if (!sch->root->down, STATUS_INTERNAL_ERROR, 0);
		err->info = 0;
		res = SchApplyExpression (sch, sch->root->down, &sch->root->atom, &sch->root->type, err);
		fail_if (res, res, info = err->info);
	}

	//now just copy the output data out

	*out = sch->root->atom.a;
	sch->root->atom.a = NULL; //NULL it out so it won't be freed by the call to SchReset

	err->error = STATUS_SUCCESS;

	SchReset (sch);

	return STATUS_SUCCESS;

failure:

	SchReset (sch);

	err->error = st;
	err->info = info;

	*out = NULL;

	return st;
}


/****************************************************************

					Other

****************************************************************/

static char *SchErrorStrings[] = 
{
	"STATUS_SUCCESS",
	"STATUS_OUT_OF_MEMORY",		
	"STATUS_USER_DEFINED",		
	"STATUS_SYNTAX_ERROR",		
	"STATUS_UNKNOWN_ERROR",
	"STATUS_INVALID_INTERPRETER	",
	"STATUS_NEED_MORE_DATA",	
	"STATUS_INTERNAL_ERROR",	
	"STATUS_NOT_APPLICABLE",		
	"STATUS_TYPE_MISMATCH",	
	"STATUS_EXCEPTION_OCCURED",
	"STATUS_INVALID_ATOM_RETURNED",
	"STATUS_INVALID_RETURN_TYPE",
	"STATUS_USER_DEFINED_ERROR",
	"STATUS_INVALID_QUOTE",
	"STATUS_DEGENERATE_TYPE",
};

char *SchGetErrorString (int error)
{
	int numerrors = sizeof (SchErrorStrings) / sizeof (char *);

	if (error < 0 || error >= numerrors)
		return "STATUS_UNKNOWN_INVALID_ERROR_CODE";

	return SchErrorStrings[error];
}



#if 0

#include <windows.h>

static void SchDumpSex (sex_t *sex, int depth)
{
	if (!sex)
		return;

	printf ("%.*s%i:%s\n", depth, "                       ", sex->type, sex->atom.a ? sex->atom.a : "(nullatom)");

	SchDumpSex (sex->down, depth + 1);

	SchDumpSex (sex->next, depth);
}

int plus (int a, int b)
{
	return a + b;
}

int plus2 (int a, int b, int count, error_t *err)
{
	//printf ("count = %i\n", count);
	err->info = 34;
	return a + b;
}

float plusf (float a, float b)
{
	return a + b;
}

char *va(char *format, ...)
{
	char *out, buffer[1024];
	va_list marker;

	va_start (marker, format);
	wvsprintf (buffer, format, marker);
	va_end (marker);

	out = SchMalloc (strlen (buffer) + 1);
	//if (!out)
	//	return NULL;
	strcpy (out, buffer);

	return out;
}

void textprint (char *line)
{
	printf ("%s\n", line);
}

HANDLE heap;
int degrade = 0;

void *allocator (int size)
{
	if (degrade && rand () % 100 < degrade)
		return NULL;

	return HeapAlloc (heap, 0, size);
}

void freer (void *block)
{
	HeapFree (heap, 0, block);
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

int main (void)
{

	scheme_t *foo;
	error_t err;
	char *text;
	int bob;

	heap = HeapCreate (0, 1000000, 0);

	count ();

	//foo = _alloca (3);

	SchCreate (&foo);

	count ();

	SchAddFunction (foo, "+", "iii", plus);
	SchAddFunction (foo, "+2", "iiice", plus2);
	SchAddFunction (foo, "+", "fff", plusf);
	SchAddFunction (foo, "text", "vs", textprint);
	SchAddFunction (foo, "atom", "va", textprint);
	SchAddFunction (foo, "va", "ss.", va);

	degrade = 10;

	bob = count ();

	while (1)
	{

		//SchParseLine (foo, "(blah (poo '(+ 8 7 \"eijf))(()()())))\\\"))(()(\")) 5 '(ass poo \"b)))))\\\"ar\") \"sh\\\"it\")", &err);
		//SchParseLine (foo, "(12345678901234567890 poo) (+ 4 3) ()", &err, &text);
		//SchParseLine (foo, "(+ 1 (+ 0xFF 010))", &err, &text);
		//SchParseLine (foo, "(text \"\\\\\n\\\\\")", &err, &text);
		SchParseLine (foo, "(text (va \"%i blah\" 34))", &err, &text);
		printf ("%i %i, %s\n", err.error, err.info, text);
		SchFree (text);

		if (count () != bob)
			break;
		0;
	}

	printf ("dead, bob is %i\n", count ());

	SchDumpSex (foo->root, 0);



}





#endif