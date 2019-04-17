/* Copyright 2008 wParam, licensed under the GPL */

#include <windows.h>
#include "sl.h"




char *SidGetByName (parerror_t *pe, char *machine, char *name)
{
	SID_NAME_USE snu;
	char *domain = NULL;
	SID *sid = NULL;
	char *out = NULL;
	DWORD domainlen = 0;
	DWORD sidlen = 0;
	int dummy, res;
	DERR st;

	fail_if (!name, 0, PE_BAD_PARAM (2));

	res = LookupAccountName (machine, name, (SID *)&dummy, &sidlen, (char *)&dummy, &domainlen, &snu);
	fail_if (sidlen == 0 || domainlen == 0, 0, PE("SidGetByName: LookupAccountName failed, %i\n", GetLastError (), 0, 0, 0));

	domain = PlMalloc (domainlen + 3);
	fail_if (!domain, 0, PE_OUT_OF_MEMORY (domainlen + 3));

	sid = PlMalloc (sidlen);
	fail_if (!sid, 0, PE_OUT_OF_MEMORY (sidlen));

	res = LookupAccountName (machine, name, sid, &sidlen, domain, &domainlen, &snu);
	fail_if (!res, 0, PE ("SidGetByName: LookupAccountName failed, second query, %i\n", GetLastError (), 0, 0, 0));

	out = PlBinaryToType (pe, sid, sidlen, "SID");
	fail_if (!out, 0, 0);

	//success
	PlFree (sid);
	PlFree (domain);

	return out;

failure:
	if (sid)
		PlFree (sid);

	if (domain)
		PlFree (domain);

	if (out)
		ParFree (out);

	return NULL;
}


SID *SidTypeToBinary (parerror_t *pe, char *type)
{
	SID *out;
	int len;
	int temp;
	DERR st;

	st = PlTypeToBinary (pe, type, "SID", &len, NULL, 0);
	fail_if (!st, 0, 0);

	out = PlMalloc (len);
	fail_if (!out, 0, PE_OUT_OF_MEMORY (len));

	st = PlTypeToBinary (pe, type, "SID", &temp, out, len);
	fail_if (!st, 0, 0);

	return out;
failure:
	return NULL;
}






/********************************** ACL Stuff ***********************************/




char *AclCreateEmpty (parerror_t *pe)
{
	ACL acl;
	DERR st;
	char *out = NULL;

	st = InitializeAcl (&acl, sizeof (ACL), ACL_REVISION);
	fail_if (!st, 0, PE ("AclCreateEmpty: InitializeACL failed, %i", GetLastError (), 0, 0, 0));

	out = CkConvertToOutput (pe, &acl, sizeof (ACL), "ACL");
	fail_if (!out, 0, 0);

	return out;
failure:

	if (out)
		ParFree (out);

	return NULL;
}


int AclIsValid (parerror_t *pe, char *aclin)
{
	ckblock_t *ckb = NULL;
	DERR st;

	fail_if (!aclin, 0, PE_BAD_PARAM (1));

	ckb = CkConvertFromOutput (pe, aclin, "ACL");
	fail_if (!ckb, 0, 0);

	//definitely not valid, and probably not a good idea to pass it to IsValidAcl
	if (ckb->size < sizeof (ACL))
		return 0;

	/* XXX: memory leak */

	return IsValidAcl ((ACL *)ckb->data);
failure:

	if (ckb)
		ParFree (ckb);

	return 0;
}



