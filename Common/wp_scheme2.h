/* Copyright 2007 wParam, licensed under the GPL */

#ifndef _WP_SCHEME2_H_
#define _WP_SCHEME2_H_



//Note:  The memory stored in this buffer must come from the same allocator
//as SchMalloc, so either you need to use malloc() or you need to use whatever
//you defined SchMalloc as.
typedef struct atom_s
{
	char *a;
	int len;
	int size; //size of memory buffer pointer to by this.a
} atom_t;


typedef struct func_t
{
	char *name;
	char *prototype; //return type letter followed by more
	void *address;

	struct func_t *next;
} func_t;

// Valid prototype letters:
//  i - int (can be given as 123, 0x123, or 0123
//  f - float (MUST have a deciamal point to be considered float)
//  s - string (comes in as char *, may be NULL)
//  v - void (Will always be null, don't use this for a parameter; it doesn't make sense.  Return type only)
//  a - atom (comes as char *, will never be NULL)
//  e - error (comes as error_t *, identical to the pointer passed to the line function)
//  c - count (comes as an integer, is the total number of paramaters passed to the function)
//  . - vararg (put this last, it makes the function take variable args.)
//  h - hex (return type only--if you put this in a prototype you will always get TYPE_MISMATCH)
//  u - unclassified (return type only--Return a char*, the schemer will classify it to null, int, float, or string)
//  p - param (comes as int; the parameter for the current parser as set by SchSetParam.)
//  n - function (comes as func_t *, it's the same pointer value returned by the findfunc function)
//	t - "type" (comes as char *, converts to and from string).  A type is specified with ``.  Do not put ()"' inside ``.
//
// Notes about functions:
//  if you return something besides STATUS_SUCCESS in the error_t, don't allocate
//  anything for your return value.  (If return type is string.)

typedef struct scheme_s scheme_t;


typedef struct error_s
{
	int error;	//a code from below
	int info;	//some extra info
} error_t;

// Error codes:
#define STATUS_SUCCESS					0
#define STATUS_OUT_OF_MEMORY			1
#define STATUS_USER_DEFINED				2
#define STATUS_SYNTAX_ERROR				3
#define STATUS_UNKNOWN_ERROR			4
#define STATUS_INVALID_INTERPRETER		5
#define STATUS_NEED_MORE_DATA			6
#define STATUS_INTERNAL_ERROR			7
#define STATUS_NOT_APPLICABLE			8
#define STATUS_TYPE_MISMATCH			9
#define STATUS_EXCEPTION_OCCURED		10
#define STATUS_INVALID_ATOM_RETURNED	11
#define STATUS_INVALID_RETURN_TYPE		12
#define STATUS_USER_DEFINED_ERROR		13
#define STATUS_INVALID_QUOTE			14
#define STATUS_DEGENERATE_TYPE			15

//numerical error code to string.  Returns a pointer to
//"STATUS_UNKNOWN_INVALID_ERROR_CODE" if error is not one
//of the status codes defined above
char *SchGetErrorString (int error);


//Create a new scheme interpreter
int SchCreate (scheme_t **schout);

//Reset an interpreter.  This destroys the current state and puts
//everything back to square one.  (Use it to abort parsing of a line
//partway through.)
int SchReset (scheme_t *sch);

//Destroy an interpreter
int SchDestroy (scheme_t *sch);

//Add a function to the internal list.  The internal list is just a
//convenience thing; if you intend on having a lot of functions you
//should handle them yourself and use binary search or something
int SchAddFunction (scheme_t *sch, char *name, char *prototype, void *address);

//Test to see if two prototypes are equal in terms of function calling.
int SchEqualPrototype (char *p1, char *p2);

//Parse function.  If line forms a complete s-expression, it is evaluated.
//If the function returns STATUS_NEED_MORE_DATA, the expression is half-
//parsed, meaning you will need to call SchReset if you want to start again.
//Any other code besides STATUS_SUCCESS means that the expression was
//automatically reset.
int SchParseLine (scheme_t *sch, char *linein, error_t *err, char **out);

//Save the state of the current expression to a temp variable and clear out
//the parser so that it can accept a new expression.  Use this, for example,
//if one of your functions takes another function as a parameter and needs
//to execute it.
int SchSaveExpression (scheme_t *in, scheme_t **outval);

//Restore a previously saved state.  Note that you must do this or the memory
//used will be leaked.  (Don't just pass it to SchFree; that will still
//leak a bunch of stuff.
int SchRestoreExpression (scheme_t *targ, scheme_t *saved);



//accessors
int SchGetNesting (scheme_t *sch);
void SchSetFindFunc (scheme_t *sch, int (*f)(char *, char *, unsigned long, func_t **));
void SchSetAtomBind (scheme_t *sch, int (*a)(atom_t *, unsigned long, int *changed));
void SchSetReleaseFunc (scheme_t *sch, int (*f)(func_t *, unsigned long));
void SchSetParam (scheme_t *sch, unsigned long lParam);
unsigned long SchGetParam (scheme_t *sch);


/*********************************************
         Atom bind function

  int AtomBind (atom_t *atom, unsigned long param, int *changed);

  Called with the atom already allocated and set to whatever the user
  set it to.  The function's job is to look at the string in atom, decide
  if it's a variable name, and replace it with the variable's value.

  It is possible that the buffer in atom will already have enough space, in
  this case no reallocation is necessary, but be sure to set the length
  properly.  Note that the length DOES include the null character, so
  set it to strlen(a) + 1.

  At present, this function will be called for numbers as well as atoms.

  If you set changed to 1, the atom will be marked as having been changed.
  I believe the ONLY side effect of this change is that when things get
  classified by type, an atom marked changed will become a string if it
  isn't a number, one marked not changed will stay an atom.  So, if you
  set changed to 1, you won't be able to (directly) bind atoms to other
  atoms.  If you don't set it to 1, you won't be able to bind atoms to
  strings.

  The function should return one of the STATUS_ codes.  If it returns
  an error, processing is aborted.

*******************************************/


/*******************************************
		Find function function

  int FindFunc (char *name, char *proto, unsigned long param, func_t **func);
  int ReleaseFunc (func_t *func, unsigned long param);

  Called for each s-expression to look for a function with the given
  name and prototype.  Use the SchEqualPrototype function to test
  whether two functions "match" by prototype.  Note that you don't
  have to check the prorotype if you don't want to.  Having it there
  allows for function overloading, which is good because currently
  there is no conversion between (for example) int and float.

  Once the pointer is returned, it should be considered 'locked',
  i.e. it should not be moved or freed.  If needed, a release
  function can be specified.  Once a func_t is released, it will not
  be accessed by the scheme parser, and the function is points to
  will not be called.  Note:  nothing in wp_scheme2.c is thread safe.
  You will need to synchronize things yourself if you will have
  multiple threads doing things at the same time.

  Note that even though the release function specifies a return,
  value, currently the value returned is ignored.

  Also note:  The ->next field of the func_t is available for use;
  wp_scheme2.c only uses if for internally defined functions.

  Also also note:  You can have both internal and external functions
  defined, in this case the external are searched first, then the
  internal.

  The find function should return one of the FF_ constants below.
  Return FF_TYPEMISMATCH if there's a function by that name but no
  prototype match.  (It's a cosmetic thing; you can just always return
  FF_NOTFOUND if you want, but in that case the user will get
  STATUS_NOT_APPLICABLE when the actual problem should be reported as
  STATUS_TYPE_MISMATCH.

  For the love of god, don't return FF_FOUND if you don't set *func to 
  something meaningful.  Likewise, if you don't return FF_FOUND, don't
  set *func to anything, the parser will not consider what you return locked,
  and will not call the unlock function for it.

*******************************************/
#define FF_FOUND 0
#define FF_NOTFOUND 1
#define FF_TYPEMISMATCH 2


#endif