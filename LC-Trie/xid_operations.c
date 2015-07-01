#include "lc_trie.h"

xid shift_left(xid id, int shift)
{
	int byte_s = 0;
	int bit_s = 0;
	int i,j;
	xid tmp;

	if (shift >= 160)
		memset(&tmp, 0, 20);
	else
	{
		byte_s = shift/8;
		bit_s = shift%8;
		if (0 == byte_s)
		{
			for (i=0;i<19;i++)
				id.w[i] = (id.w[i] << bit_s)|(id.w[i+1] >> (8-bit_s));
			id.w[i] = id.w[i] << bit_s;
			tmp = id;
			return tmp;
		}
		for (i=0;i<(20 - byte_s);i++)
			id.w[i] = id.w[i+byte_s];
		for (j=i;j<20;j++)
			id.w[j] = (unsigned char) 0;
		for (j=0;j<i;j++)
			id.w[j] = (id.w[j] << bit_s)|(id.w[j+1] >> (8-bit_s));
		tmp = id;
	}
	return tmp;
}

static xid shift_right(xid id, int shift)
{
	int byte_s = 0;
	int bit_s = 0;
	int i,j;
	xid tmp;

	if (shift >= 160)
		memset(&tmp, 0, 20);
	else
	{
		byte_s = shift/8;
		bit_s = shift%8;
		if (0 == byte_s)
		{
			for (i=19;i>0;i--)
				id.w[i] = (id.w[i] >> bit_s)|(id.w[i-1] << (8-bit_s));
			id.w[i] = id.w[i] >> bit_s;
			tmp = id;
			return tmp;
		}
		for (i=19;i>(byte_s - 1);i--)
			id.w[i] = id.w[i-byte_s];
		for (j=i;j>-1;j--)
			id.w[j] = (unsigned char) 0;
		for (j=19;j>i;j--)
			id.w[j] = (id.w[j] >> bit_s)|(id.w[j-1] << (8-bit_s));
		tmp = id;
	}
	return tmp;
}

xid extract(int pos, int length, xid data)
{
	int i;
	xid tmp;

	// length = 0 is handled in the boolean expression and is short-circuit
	// even though length of zero works in case of xids
	tmp = shift_right(shift_left(data, pos), (160-length));
	return tmp;
}
