/*
 * DEC Core-Dump file.
 * Format for storing 36-bits into 5 tape frames.
 *
 *    DEC Core-Dump Mode          ANSI Compatible Mode
 * |00 01 02 03 04 05 06|07     |__ 00 01 02 03 04 05 06|
 *  08 09 10 11 12 13|14 15     |__ 07 08 09 10 11 12 13|
 *  16 17 18 19 20|21 22 23     |__ 14 15 16 17 18 19 20|
 *  24 25 26 27|28 29 30 31     |__ 21 22 23 24 25 26 27|
 *  __ __ __ __ 32 33 34|35|    |35|28 29 30 31 32 33 34|
 *
 * Note: "|" separate the 7-bit bytes,
 * "__" are unused bits (which is zeros).
 *
 */

int36 pdp10_Convert8to36(uchar *data8)
{
	int36 data36;

	data36 = data8[0];
	data36 = (data36 << 8) | data8[1];
	data36 = (data36 << 8) | data8[2];
	data36 = (data36 << 8) | data8[3];
	data36 = (data36 << 4) | data8[4];

	return data36;
}

uchar *pdp10_Convert36to8(int36 data36)
{
	static uchar data8[6];

	data8[0] = (data36 >> 28) & 0xFF;
	data8[1] = (data36 >> 20) & 0xFF;
	data8[2] = (data36 >> 12) & 0xFF;
	data8[3] = (data36 >>  4) & 0xFF;
	data8[4] = data36 & 0xF;
	data8[5] = '\0';

	return data8;
}
