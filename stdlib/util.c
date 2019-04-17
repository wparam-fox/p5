/* Copyright 2007 wParam, licensed under the GPL */

#include <windows.h>
#include "sl.h"


int UtilReverseColor (int l)
{
	int b1, b2, b3;

	b1 = l & 0xFF;
	b2 = (l >> 8) & 0xFF;
	b3 = (l >> 16) & 0xFF;

	return (b1 << 16) | (b2 << 8) | b3;
}



//Must set in->size before you call this.
unsigned short CkComputeChecksum (ckblock_t *in)
{
	unsigned short int out = 0;
	unsigned short int out2 = 0;
	int x;

	for (x=0;x<in->size;x++)
	{
		//I have no idea how good this will be in practice.
		//rotate left one bit and xor the new byte
		out = (out << 1) | ((out >> 15) & 1);
		out ^= (((unsigned short)in->data[x]) & 0x00FF);

		out2 = (out2 << 1) | ((out2 >> 15) & 1);
		out2 += (((unsigned short)in->data[x]) & 0x00FF);


	}

	return out ^ out2;
}



static ckblock_t *CkMakeChecksum (parerror_t *pe, void *in, int len)
{
	ckblock_t *out;
	DERR st;

	out = PlMalloc (len + sizeof (ckblock_t));
	fail_if (!out, 0, PE_OUT_OF_MEMORY (len + sizeof (ckblock_t)));

	PlMemcpy (out->data, in, len);

	out->size = len;
	out->cksum = CkComputeChecksum (out);

	return out;
failure:
	return NULL;
}

//Returns allocated from ParMalloc
char *CkConvertToOutput (parerror_t *pe, void *in, int inlen, char *type)
{
	DERR st;
	char *out = NULL;
	ckblock_t *ckb = NULL;

	ckb = CkMakeChecksum (pe, in, inlen);
	fail_if (!ckb, 0, 0);

	out = PlBinaryToType (pe, ckb, ckb->size + sizeof (ckblock_t), type);
	fail_if (!out, 0, 0);

	PlFree (ckb);

	return out;
failure:

	if (out)
		ParFree (out);

	if (ckb)
		PlFree (ckb);

	return NULL;
}

//Returns allocated from PlMalloc
ckblock_t *CkConvertFromOutput (parerror_t *pe, char *in, char *typeexpected)
{
	DERR st;
	ckblock_t *ckb = NULL;
	int len, outlen;
	unsigned short sum;

	
	//grab length
	st = PlTypeToBinary (pe, in, typeexpected, &len, NULL, 0);
	fail_if (!st, 0, 0);

	//sanity check to make sure we at least have the checksum block.
	//It's definitely bogus without this.
	fail_if (len < sizeof (ckblock_t), 0, PE ("CkConvertFromOutput: Type size %i definitely too small", len, 0, 0, 0));

	//Go ahead and allocate/read the data
	ckb = PlMalloc (len);
	fail_if (!ckb, 0, PE_OUT_OF_MEMORY (len));

	st = PlTypeToBinary (pe, in, typeexpected, &outlen, ckb, len);
	fail_if (!st, 0, 0);
	fail_if (outlen != len, 0, PE ("CkConvertFromOutput: Sanity failure in PlTypeToBinary", 0, 0, 0, 0));

	//second sanity check--does the length of the data match the size reported in the ckecksum block?
	fail_if (ckb->size != len - sizeof (ckblock_t), 0, PE ("CkConvertFromOutput: Size of TYPE(%i) and size stored in type(%i) do not match", len, ckb->size + sizeof (ckblock_t), 0, 0));

	//Great.  So we know that ckb->data does indeed have as much data as ckb->size says it does.
	//So it's safe to pass to CkComputeChecksum; test to see if it's valid.
	sum = CkComputeChecksum (ckb);
	fail_if (sum != ckb->cksum, 0, PE ("CkConvertFromOutput: Checksum failure in data (0x%.4X should be 0x%.4X)", sum, ckb->cksum, 0, 0));

	//OK.  It's as valid as we can figure it to be.
	//At this point we assume that it is, indeed, valid data returned by some
	//other function.  If the user went to great lengths to construct something
	//invalid to crash IsValidACL() or IsValidSD() or whatever, then we're assigning blame for that to the
	//user and washing our hands of it.

	return ckb;
failure:
	if (ckb)
		PlFree (ckb);

	return NULL;
}

