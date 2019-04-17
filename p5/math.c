/* Copyright 2008 wParam, licensed under the GPL */

#include <windows.h>
#include <math.h>
#include <stdio.h>

#include "p5.h"


static int MthAddInt (int a, int b)				{ return a + b; }
static int MthSubInt (int a, int b)				{ return a - b; }
static int MthMulInt (int a, int b)				{ return a * b; }
static int MthAndInt (int a, int b)				{ return a & b; }
static int MthOrInt  (int a, int b)				{ return a | b; }
static int MthXorInt (int a, int b)				{ return a ^ b; }
static int MthNotInt (int a)					{ return ~a; }
static int MthIncInt (int a)					{ return ++a; }
static int MthDecInt (int a)					{ return --a; }	

static float MthAddFloat (float a, float b)		{ return a + b; }
static float MthSubFloat (float a, float b)		{ return a - b; }
static float MthMulFloat (float a, float b)		{ return a * b; }
static float MthDivFloat (float a, float b)		{ return a / b; }

static float MthSqrtFloat (float a)				{ return (float)sqrt ((double)a); }
static float MthSinFloat (float a)				{ return (float)sin ((double)a); }
static float MthConFloat (float a)				{ return (float)cos ((double)a); }
static float MthTanFloat (float a)				{ return (float)tan ((double)a); }
static float MthLnFloat (float a)				{ return (float)log ((double)a); }

static float MthPowFloat (float a, float b)		{ return (float)pow ((double)a, (double)b); }
static int MthPowInt (int a, int b)			{ return (int)pow ((double)a, (double)b); }

static int MthEqualInt (int a, int b)			{ return a == b; }
static int MthNotEqualInt (int a, int b)		{ return a != b; }
static int MthLessThanInt (int a, int b)		{ return a < b; }
static int MthGreaterThanInt (int a, int b)		{ return a > b; }
static int MthLessEqualInt (int a, int b)		{ return a <= b; }
static int MthGreaterEqualInt (int a, int b)	{ return a >= b; }

static int MthEqualFloat (float a, float b)			{ return a == b; }
static int MthNotEqualFloat (float a, float b)		{ return a != b; }
static int MthLessThanFloat (float a, float b)		{ return a < b; }
static int MthGreaterThanFloat (float a, float b)	{ return a > b; }
static int MthLessEqualFloat (float a, float b)		{ return a <= b; }
static int MthGreaterEqualFloat (float a, float b)	{ return a >= b; }


static int MthDivInt (int a, int b, parerror_t *pe) 
{
	if (!b)
	{
		pe->error = STATUS_USER_DEFINED_ERROR;
		pe->format = "Integer division by zero";
		return 0;
	}
	return a / b; 
}
static int MthModInt (int a, int b, parerror_t *pe) 
{
	if (!b)
	{
		pe->error = STATUS_USER_DEFINED_ERROR;
		pe->format = "Integer mod by zero";
		return 0;
	}
	return a % b; 
}

static int MthLogicalAnd (int a, int b) { return a && b; }
static int MthLogicalOr  (int a, int b) { return a || b; }
static int MthLogicalNot (int a) { return !a; }

static int	MthFtoi (float f) { return (int)f; }
static float MthItof (int i) { return (float)i; }
static int MthItoh (int i) {return i;}


static char *MthFtos (parerror_t *pe, float in)
{
	char *out, buffer[1024];
	int st;

	_snprintf (buffer, 1023, "%f", in);
	buffer[1023] = '\0';

	out = ParMalloc (strlen (buffer) + 1);
	fail_if (!out, 0, PE ("Out of memory on %i bytes", strlen (buffer) + 1, 0, 0, 0));

	strcpy (out, buffer);

	return out;
failure:
	return NULL;
}

static char *MthItos (parerror_t *pe, int in)
{
	char *out, buffer[1024];
	int st;

	_snprintf (buffer, 1023, "%i", in);
	buffer[1023] = '\0';

	out = ParMalloc (strlen (buffer) + 1);
	fail_if (!out, 0, PE ("Out of memory on %i bytes", strlen (buffer) + 1, 0, 0, 0));

	strcpy (out, buffer);

	return out;
failure:
	return NULL;
}

static char *MthHtos (parerror_t *pe, int in)
{
	char *out, buffer[1024];
	int st;

	_snprintf (buffer, 1023, "%X", in);
	buffer[1023] = '\0';

	out = ParMalloc (strlen (buffer) + 1);
	fail_if (!out, 0, PE ("Out of memory on %i bytes", strlen (buffer) + 1, 0, 0, 0));

	strcpy (out, buffer);

	return out;
failure:
	return NULL;
}

static char *MthStos (parerror_t *pe, char *in)
{
	int st;
	char *out;

	fail_if (!in, 0, PE_BAD_PARAM (1));

	out = ParMalloc (strlen (in) + 1);
	fail_if (!out, 0, PE ("Out of memory on %i bytes", strlen (in) + 1, 0, 0, 0));

	strcpy (out, in);

	return out;
failure:
	return NULL;
}


/**************************************************************************
							64 bit integers
 **************************************************************************/


static char *MthAddInt64 (parerror_t *pe, char *o1, char *o2)
{
	DERR st;
	__int64 a, b;
	char *out = NULL;

	fail_if (!o1, 0, PE_BAD_PARAM (1));
	fail_if (!o2, 0, PE_BAD_PARAM (2));

	st = UtilTypeToLongInt (pe, o1, &a);
	fail_if (!st, 0, 0); //PE already set.
	st = UtilTypeToLongInt (pe, o2, &b);
	fail_if (!st, 0, 0); //PE already set.

	out = UtilLongIntToType (pe, a + b);
	fail_if (!out, 0, 0);

	return out;
failure:
	if (out)
		ParFree (out);

	return NULL;
}

static char *MthSubInt64 (parerror_t *pe, char *o1, char *o2)
{
	DERR st;
	__int64 a, b;
	char *out = NULL;

	fail_if (!o1, 0, PE_BAD_PARAM (1));
	fail_if (!o2, 0, PE_BAD_PARAM (2));

	st = UtilTypeToLongInt (pe, o1, &a);
	fail_if (!st, 0, 0); //PE already set.
	st = UtilTypeToLongInt (pe, o2, &b);
	fail_if (!st, 0, 0); //PE already set.

	out = UtilLongIntToType (pe, a - b);
	fail_if (!out, 0, 0);

	return out;
failure:
	if (out)
		ParFree (out);

	return NULL;
}

static char *MthMulInt64 (parerror_t *pe, char *o1, char *o2)
{
	DERR st;
	__int64 a, b;
	char *out = NULL;

	fail_if (!o1, 0, PE_BAD_PARAM (1));
	fail_if (!o2, 0, PE_BAD_PARAM (2));

	st = UtilTypeToLongInt (pe, o1, &a);
	fail_if (!st, 0, 0); //PE already set.
	st = UtilTypeToLongInt (pe, o2, &b);
	fail_if (!st, 0, 0); //PE already set.

	out = UtilLongIntToType (pe, a * b);
	fail_if (!out, 0, 0);

	return out;
failure:
	if (out)
		ParFree (out);

	return NULL;
}

static char *MthDivInt64 (parerror_t *pe, char *o1, char *o2)
{
	DERR st;
	__int64 a, b;
	char *out = NULL;

	fail_if (!o1, 0, PE_BAD_PARAM (1));
	fail_if (!o2, 0, PE_BAD_PARAM (2));

	st = UtilTypeToLongInt (pe, o1, &a);
	fail_if (!st, 0, 0); //PE already set.
	st = UtilTypeToLongInt (pe, o2, &b);
	fail_if (!st, 0, 0); //PE already set.

	fail_if (b == 0, 0, PE ("64 bit integer division by zero", 0, 0, 0, 0));

	out = UtilLongIntToType (pe, a + b);
	fail_if (!out, 0, 0);

	return out;
failure:
	if (out)
		ParFree (out);

	return NULL;
}

static char *MthModInt64 (parerror_t *pe, char *o1, char *o2)
{
	DERR st;
	__int64 a, b;
	char *out = NULL;

	fail_if (!o1, 0, PE_BAD_PARAM (1));
	fail_if (!o2, 0, PE_BAD_PARAM (2));

	st = UtilTypeToLongInt (pe, o1, &a);
	fail_if (!st, 0, 0); //PE already set.
	st = UtilTypeToLongInt (pe, o2, &b);
	fail_if (!st, 0, 0); //PE already set.

	fail_if (b == 0, 0, PE ("64 bit integer mod by zero", 0, 0, 0, 0));

	out = UtilLongIntToType (pe, a % b);
	fail_if (!out, 0, 0);

	return out;
failure:
	if (out)
		ParFree (out);

	return NULL;
}

int MthEqualInt64 (parerror_t *pe, char *o1, char *o2)
{
	DERR st;
	__int64 a, b;

	fail_if (!o1, 0, PE_BAD_PARAM (1));
	fail_if (!o2, 0, PE_BAD_PARAM (2));

	st = UtilTypeToLongInt (pe, o1, &a);
	fail_if (!st, 0, 0); //PE already set.
	st = UtilTypeToLongInt (pe, o2, &b);
	fail_if (!st, 0, 0); //PE already set.

	return a == b;
failure:
	return 0;
}

int MthNotEqualInt64 (parerror_t *pe, char *o1, char *o2)
{
	DERR st;
	__int64 a, b;

	fail_if (!o1, 0, PE_BAD_PARAM (1));
	fail_if (!o2, 0, PE_BAD_PARAM (2));

	st = UtilTypeToLongInt (pe, o1, &a);
	fail_if (!st, 0, 0); //PE already set.
	st = UtilTypeToLongInt (pe, o2, &b);
	fail_if (!st, 0, 0); //PE already set.

	return a != b;
failure:
	return 0;
}

int MthLessThanInt64 (parerror_t *pe, char *o1, char *o2)
{
	DERR st;
	__int64 a, b;

	fail_if (!o1, 0, PE_BAD_PARAM (1));
	fail_if (!o2, 0, PE_BAD_PARAM (2));

	st = UtilTypeToLongInt (pe, o1, &a);
	fail_if (!st, 0, 0); //PE already set.
	st = UtilTypeToLongInt (pe, o2, &b);
	fail_if (!st, 0, 0); //PE already set.

	return a < b;
failure:
	return 0;
}

int MthGreaterThanInt64 (parerror_t *pe, char *o1, char *o2)
{
	DERR st;
	__int64 a, b;

	fail_if (!o1, 0, PE_BAD_PARAM (1));
	fail_if (!o2, 0, PE_BAD_PARAM (2));

	st = UtilTypeToLongInt (pe, o1, &a);
	fail_if (!st, 0, 0); //PE already set.
	st = UtilTypeToLongInt (pe, o2, &b);
	fail_if (!st, 0, 0); //PE already set.

	return a > b;
failure:
	return 0;
}

int MthLessEqualInt64 (parerror_t *pe, char *o1, char *o2)
{
	DERR st;
	__int64 a, b;

	fail_if (!o1, 0, PE_BAD_PARAM (1));
	fail_if (!o2, 0, PE_BAD_PARAM (2));

	st = UtilTypeToLongInt (pe, o1, &a);
	fail_if (!st, 0, 0); //PE already set.
	st = UtilTypeToLongInt (pe, o2, &b);
	fail_if (!st, 0, 0); //PE already set.

	return a <= b;
failure:
	return 0;
}

int MthGreaterEqualInt64 (parerror_t *pe, char *o1, char *o2)
{
	DERR st;
	__int64 a, b;

	fail_if (!o1, 0, PE_BAD_PARAM (1));
	fail_if (!o2, 0, PE_BAD_PARAM (2));

	st = UtilTypeToLongInt (pe, o1, &a);
	fail_if (!st, 0, 0); //PE already set.
	st = UtilTypeToLongInt (pe, o2, &b);
	fail_if (!st, 0, 0); //PE already set.

	return a >= b;
failure:
	return 0;
}


int MthInt64ToInt (parerror_t *pe, char *o1)
{
	DERR st;
	__int64 a;

	fail_if (!o1, 0, PE_BAD_PARAM (1));

	st = UtilTypeToLongInt (pe, o1, &a);
	fail_if (!st, 0, 0); //PE already set.

	return (int)(a);
failure:
	return 0;
}

int MthInt64LoWord (parerror_t *pe, char *o1)
{
	DERR st;
	__int64 a;

	fail_if (!o1, 0, PE_BAD_PARAM (1));

	st = UtilTypeToLongInt (pe, o1, &a);
	fail_if (!st, 0, 0); //PE already set.

	return (int)(a & 0xFFFFFFFF);
failure:
	return 0;
}

int MthInt64HiWord (parerror_t *pe, char *o1)
{
	DERR st;
	__int64 a;

	fail_if (!o1, 0, PE_BAD_PARAM (1));

	st = UtilTypeToLongInt (pe, o1, &a);
	fail_if (!st, 0, 0); //PE already set.

	return (int)((a >> 32) & 0xFFFFFFFF);
failure:
	return 0;
}

char *MthFloatToInt64 (parerror_t *pe, float f)
{
	char *out = NULL;
	DERR st;

	out = UtilLongIntToType (pe, (__int64)f);
	fail_if (!out, 0, 0); //PE already set

	return out;
failure:
	if (out)
		ParFree (out);
	return NULL;
}

char *MthIntToInt64 (parerror_t *pe, int i)
{
	char *out = NULL;
	DERR st;

	out = UtilLongIntToType (pe, (__int64)i);
	fail_if (!out, 0, 0); //PE already set

	return out;
failure:
	if (out)
		ParFree (out);
	return NULL;
}

char *MthIntHighLowToInt64 (parerror_t *pe, int high, int low)
{
	char *out = NULL;
	DERR st;

	//The and with 0xFFFFFFFF is there because a cast to int64 extends the sign bit.
	//I should probably change the prototype of this function to accept unsigned ints,
	//but it's late and this is working as it is.
	out = UtilLongIntToType (pe, (((__int64)high) << 32) | (((__int64)low) & 0xFFFFFFFF));
	fail_if (!out, 0, 0); //PE already set

	return out;
failure:
	if (out)
		ParFree (out);
	return NULL;
}

float MthInt64ToFloat (parerror_t *pe, char *o1)
{
	DERR st;
	__int64 a;

	fail_if (!o1, 0, PE_BAD_PARAM (1));

	st = UtilTypeToLongInt (pe, o1, &a);
	fail_if (!st, 0, 0); //PE already set.

	return (float)a;
failure:
	return 0;
}

char *MthInt64ToString (parerror_t *pe, char *o1)
{
	DERR st;
	__int64 a;
	char *out = NULL;

	fail_if (!o1, 0, PE_BAD_PARAM (1));

	st = UtilTypeToLongInt (pe, o1, &a);
	fail_if (!st, 0, 0); //PE already set.

	out = ParMalloc (50); //It's late, too lazy to figure out the right answer
	fail_if (!out, 0, PE_OUT_OF_MEMORY (50));

	_snprintf (out, 49, "%I64i", a);
	out[49] = '\0'; //failsafe NULL

	return out;
failure:
	if (out)
		ParFree (out);
	return 0;
}

void MthDebugTest (parerror_t *pe, char *type)
{
	RECT r;
	
	int len;
	DERR st;

	fail_if (!type, 0, PE_BAD_PARAM (1));

	//bob = UtilBinaryToType (pe, &r, sizeof (RECT), "rect");

	//memset (&r, 0, sizeof (RECT));

	st = UtilTypeToBinary (pe, type, "rect", &len, NULL, 0);
	fail_if (!st, 0, 0);
	fail_if (len != sizeof (RECT), 0, PE ("Given type does not match length of a RECT (%i should be %i)", len, sizeof (RECT), 0, 0));

	st = UtilTypeToBinary (pe, type, "rect", &len, &r, sizeof (RECT));
	fail_if (!st, 0, 0);

	ConWriteStringf ("RECT: %i %i %i %i\n", r.top, r.left, r.bottom, r.right);

failure:
	return;

}

char *MthDebugTest2 (parerror_t *pe, int top, int left, int bottom, int right)
{
	RECT r;
	DERR st;
	char *out = NULL;

	r.top = top;
	r.left = left;
	r.right = right;
	r.bottom = bottom;

	out = UtilBinaryToType (pe, &r, sizeof (RECT), "rect");
	fail_if (!out, 0, 0);

	return out;
failure:
	return NULL;
}

DERR MthAddGlobalCommands (parser_t *root)
{
	DERR st;

	st = ParAddFunction (root, "dbg", "vet", MthDebugTest, "Integer addition", "(+ [i:a] [i:b]) = [i:a + b]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "dbg", "teiiii", MthDebugTest2, "Integer addition", "(+ [i:a] [i:b]) = [i:a + b]", "");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "math.+", "iii", MthAddInt, "Integer addition", "(+ [i:a] [i:b]) = [i:a + b]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.-", "iii", MthSubInt, "Integer subtraction", "(- [i:a] [i:b]) = [i:a - b]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.*", "iii", MthMulInt, "Integer multiplication", "(* [i:a] [i:b]) = [i:a * b]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math./", "iiie", MthDivInt, "Integer division", "(/ [i:a] [i:b]) = [i:a / b]", "An error is raised if b is zero.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.%", "iiie", MthModInt, "Integer mod", "(% [i:a] [i:b]) = [i:a % b]", "An error is raised if b is zero.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.&", "iii", MthAndInt, "Integer boolean AND", "(& [i:a] [i:b]) = [i:a & b]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.|", "iii", MthOrInt, "Integer boolean OR", "(| [i:a] [i:b]) = [i:a | b]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.^", "iii", MthXorInt, "Integer boolean XOR", "(^ [i:a] [i:b]) = [i:a ^ b]", "If you are looking for exponentiation, i.e. 5^2 = 25, you want math.pow, also known as **");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.~", "ii", MthNotInt, "Integer boolean NOT", "(^ [i:a]) = [i: ~a]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.++", "ii", MthIncInt, "Integer increment", "(^ [i:a]) = [i:a + 1]", "Note: this does not have the side effects that the C ++ operator has; (++ a) is identical to (+ a 1) in every way, except that (++ a) probably parses slightly faster.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.--", "ii", MthDecInt, "Integer decrement", "(^ [i:a]) = [i:a - 1]", "Note: this does not have the side effects that the C -- operator has; (-- a) is identical to (- a 1) in every way, except that (-- a) probably parses slightly faster.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "math.+", "fff", MthAddFloat, "Floating point addition", "(+ [f:a] [f:b]) = [f:a + b]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.-", "fff", MthSubFloat, "Floating point subtraction", "(- [f:a] [f:b]) = [f:a - b]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.*", "fff", MthMulFloat, "Floating point multiplication", "(* [f:a] [f:b]) = [f:a * b]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math./", "fff", MthMulFloat, "Floating point division", "(/ [f:a] [f:b]) = [f:a / b]", "For some reason, the default action of the floating point unit in my machine is to return 0 when b is 0.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "math.+", "tett", MthAddInt64, "64 bit integer addition", "(+ [t:a] [t:b]) = [t:a + b]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.-", "tett", MthSubInt64, "64 bit integer subtraction", "(+ [t:a] [t:b]) = [t:a - b]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.*", "tett", MthMulInt64, "64 bit integer multiplication", "(+ [t:a] [t:b]) = [t:a * b]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math./", "tett", MthDivInt64, "64 bit integer division", "(+ [t:a] [t:b]) = [t:a / b]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.%", "tett", MthDivInt64, "64 bit integer modulo", "(% [t:a] [t:b]) = [t:a % b]", "");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "math.pow", "fff", MthPowFloat, "Floating point exponentiation", "(** [f:a] [f:b]) = [f:a ^ b]", "This is what ^ is usually used to mean (instead of exclusive or).  This function returns a raised to the power of b.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.pow", "iii", MthPowInt, "Integer point exponentiation", "(** [i:a] [i:b]) = [i:a ^ b]", "This is what ^ is usually used to mean (instead of exclusive or).  This function returns a raised to the power of b.  Note that internally this uses the same function as the floating point **, so it's posible that rounding errors may occur.  I haven't looked too deeply into it, so I'm not sure.");
	fail_if (!DERR_OK (st), st, 0);


	st = ParAddFunction (root, "math.==", "iii", MthEqualInt, "Tests for integer equality", "(== [i] [i]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.!=", "iii", MthNotEqualInt, "Tests for integer inequality", "(!= [i] [i]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.<", "iii", MthLessThanInt, "Tests for integer less than relationship", "(< [i] [i]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.>", "iii", MthGreaterThanInt, "Tests for integer greater than relationship", "(> [i] [i]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.<=", "iii", MthLessEqualInt, "Tests for integer less than or equal to relationship", "(<= [i] [i]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.>=", "iii", MthGreaterEqualInt, "Tests for integer greater than or equal to relationship", "(>= [i] [i]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "math.==", "iff", MthEqualFloat, "Tests for floating point equality", "(== [f] [f]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.!=", "iff", MthNotEqualFloat, "Tests for floating point inequality", "(!= [f] [f]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.<", "iff", MthLessThanFloat, "Tests for floating point less than relationship", "(< [f] [f]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.>", "iff", MthGreaterThanFloat, "Tests for floating point greater than relationship", "(> [f] [f]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.<=", "iff", MthLessEqualFloat, "Tests for floating point less than or equal to relationship", "(<= [f] [f]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.>=", "iff", MthGreaterEqualFloat, "Tests for floating point greater than or equal to relationship", "(>= [f] [f]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "math.==", "iett", MthEqualInt64, "Tests for 64 bit integer equality", "(== [t] [t]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.!=", "iett", MthNotEqualInt64, "Tests for 64 bit integer inequality", "(!= [t] [t]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.<", "iett", MthLessThanInt64, "Tests for 64 bit integer less than relationship", "(< [t] [t]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.>", "iett", MthGreaterThanInt64, "Tests for 64 bit integer greater than relationship", "(> [t] [t]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.<=", "iett", MthLessEqualInt64, "Tests for 64 bit integer less than or equal to relationship", "(<= [t] [t]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.>=", "iett", MthGreaterEqualInt64, "Tests for 64 bit integer greater than or equal to relationship", "(>= [t] [t]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);


	st = ParAddFunction (root, "math.!", "ii", MthLogicalNot, "Logical NOT", "(! [i]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.&&", "iii", MthLogicalAnd, "Logical AND", "(&& [i] [i]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.||", "iii", MthLogicalOr, "Logical OR", "(|| [i] [i]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "math.conv.ftoi", "if", MthFtoi, "Convert float to integer", "(ftoi [f]) = [i]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.conv.itof", "fi", MthItof, "Convert integer to float", "(itof [i]) = [f]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.conv.itoh", "hi", MthItoh, "Convert integer to hex format", "(itoh [i]) = [h]", "This is only really useful when you want the final output to be formatted as hex.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "math.conv.ftol", "tef", MthFloatToInt64, "Convert float to 64 bit integer", "(ftol [f]) = [t]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.conv.ltof", "fet", MthInt64ToFloat, "Convert 64 bit integer to float", "(ltof [t]) = [f]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.conv.itol", "tei", MthIntToInt64, "Convert integer to 64 bit integer", "(itol [i]) = [t]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.conv.iitol", "teii", MthIntHighLowToInt64, "Joins two integers together to form one 64 bit integer", "(iitol [i:high bits] [i:low bits]) = [t]", "High bits come first so that the number will look roughly correct if you put them both in in hex.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "math.conv.loword", "iet", MthInt64LoWord, "Gets the low 32 bits of a 64 bit integer.", "(math.conv.loword [t:int64]) = [i:low bits]", "The input is simply truncated to 32 bits and returned as an integer.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.conv.hiword", "iet", MthInt64HiWord, "Gets the high 32 bits of a 64 bit integer.", "(math.conv.hiword [t:int64]) = [i:high bits]", "The high 32 bits of the input are returned as an integer.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddFunction (root, "math.conv.ftos", "sef", MthFtos, "Float to string", "(ftos [f]) = [s]", "Use this to print floating point numbers.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.conv.itos", "sei", MthItos, "Int to string", "(itos [i]) = [s]", "");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.conv.htos", "sei", MthHtos, "Int to (hex) string", "(htos [i]) = [s]", "Due to the way things work, you can't use the normal (s) or (itos) functions to get output in hex format.  (s (itoh)) won't work either.  Call this explicitly.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.conv.ltos", "set", MthInt64ToString, "64 bit int to string", "(ltos [t]) = [s]", "This function converts the int64 to a string representation using base 10.  There is no easy way I am aware of to convert this back into an int64, it is intended mainly for output purposes, and is tied to the \"s\" alias because it seems most appropriate.  Use ttos and stot to convert between any generic type and string.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.conv.stos", "ses", MthStos, "String to string", "(stos [s]) = [s]", "This function returns the argument you pass in.  Yes, this function really does exist.  It exists because the \"s\" function, which converts any type to a string, needs it.  Use (s) when you have a variable that MUST be interpreted as a string.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.conv.atos", "sea", MthStos, "Atom to string", "(atos [a]) = [s]", "This function exists so that (s) can accept atoms.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.conv.stoa", "aes", MthStos, "String to Atom", "(stoa [s]) = [a]", "Variables never evaluate to atoms, so if you actually need to store an atom (such as a function name) in a variable, you will have to pass it through this function before it can be properly used.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.conv.ttos", "set", MthStos, "TYPE to string", "(ttos [t]) = [s]", "This simply changes the type of the input from a TYPE to a string.  (From a string surrounded with `` to one surrounded with \"\".)  Included for completeness with stot.");
	fail_if (!DERR_OK (st), st, 0);
	st = ParAddFunction (root, "math.conv.stot", "tes", MthStos, "String to TYPE", "(stot [s]) = [t]", "Variables can't store TYPEs natively, so you need to run them through this so that functions will recognize the variable properly.  The string passed in will have to be a valid, of the form \"footype:[hex data]\", where footype is something useful.");
	fail_if (!DERR_OK (st), st, 0);

	st = ParAddAlias (root, "+",  "*.", "math.+");		fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "-",  "*.", "math.-");		fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "*",  "*.", "math.*");		fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "/",  "*.", "math./");		fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "^",  "*.", "math.^");		fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "&",  "*.", "math.&");		fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "|",  "*.", "math.|");		fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "%",  "*.", "math.%");		fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "++", "*.", "math.++");		fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "--", "*.", "math.--");		fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "~",  "*.", "math.~");		fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "**", "*.", "math.pow");	fail_if (!DERR_OK (st), st, 0);

	st = ParAddAlias (root, "==", "*.", "math.==");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "!=", "*.", "math.!=");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "<",  "*.", "math.<");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, ">",  "*.", "math.>");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "<=", "*.", "math.<=");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, ">=", "*.", "math.>=");	fail_if (!DERR_OK (st), st, 0);

	st = ParAddAlias (root, "!",  "*.", "math.!");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "&&", "*.", "math.&&");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "||", "*.", "math.||");	fail_if (!DERR_OK (st), st, 0);

	st = ParAddAlias (root, "ftoi", "if",  "math.conv.ftoi");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "itof", "fi",  "math.conv.itof");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "itoh", "hi",  "math.conv.itoh");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "ftos", "sef", "math.conv.ftos");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "itos", "sei", "math.conv.itos");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "htos", "sei", "math.conv.htos");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "ltos", "set", "math.conv.ltos");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "stos", "ses", "math.conv.stos");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "atos", "sea", "math.conv.atos");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "stoa", "aes", "math.conv.stoa");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "ttos", "set", "math.conv.ttos");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "stot", "tes", "math.conv.stot");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "ltoi", "iet", "math.conv.loword");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "ftol", "tf",  "math.conv.ftol");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "ltof", "ft",  "math.conv.ltof");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "itol", "ti",  "math.conv.itol");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "iitol","tii", "math.conv.iitol");	fail_if (!DERR_OK (st), st, 0);

	st = ParAddAlias (root, "s", "si", "math.conv.itos");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "s", "st", "math.conv.ltos");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "s", "sf", "math.conv.ftos");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "s", "ss", "math.conv.stos");	fail_if (!DERR_OK (st), st, 0);
	st = ParAddAlias (root, "s", "sa", "math.conv.atos");	fail_if (!DERR_OK (st), st, 0);


	st = ParAddAtom (root, "PI", "3.14159265", 0);
	fail_if (!DERR_OK (st), st, 0);
	


	return PF_SUCCESS;

failure:
	return st;
}