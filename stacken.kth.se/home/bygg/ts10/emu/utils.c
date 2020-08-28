// utils.c - utility routines are provided for the PDP emulator
//
// Written by
//  Timothy Stark <sword7@speakeasy.org>
//
// This file is part of the TS10 Emulator.
// See ReadMe for copyright notice.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include <time.h>
#include "emu/defs.h"

void util_RemoveSpaces(register char *String)
{
	register char *curString, *endString;

	if (String && *String) {
		// Remove trailing spaces
		endString = String + strlen(String) - 1;
		curString = endString;
		while(isspace(*curString) && (curString >= String))
			curString--;
		if (curString != endString)
			*(curString + 1) = 0;

		// Remove leading spaces
		curString = String;
		while(isspace(*curString) && *curString)
			curString++;
		if (curString != String)
			strcpy(String, curString);
	}
}

char *util_SplitChar(register char **curString, register char divider)
{
	register char *ptr, *wordString;

	if (curString) {
		ptr = *curString;
		for (wordString = ptr; *ptr && (*ptr != divider); ptr++);
		if (*ptr == divider) {
			*ptr++ = 0;
			*curString = ptr;
			return wordString;
		}
	}
	return NULL;
}

char *util_SplitWord(register char **curString)
{
	register char *ptr, *wordString;

	if (!curString)
		return *curString = "";
	for (ptr = *curString; isspace(*ptr); ptr++);
	for (wordString = ptr; *ptr && !isspace(*ptr); ptr++);
	if (*ptr)
		*ptr++ = 0;
	*curString = ptr;
	return wordString;
}

char *util_SplitQuote(register char **curString)
{
	register char *ptr, *quoteString;

	if (!curString)
		return *curString = "";
	for (ptr = *curString; isspace(*ptr); ptr++);
	if (*ptr == '"') {
		ptr++;
		for (quoteString = ptr; *ptr && (*ptr != '"'); ptr++);
		if (*ptr != '"')
			return NULL;
	} else 
		for (quoteString = ptr; !isspace(*ptr); ptr++);
	if (*ptr)
		*ptr++ = 0;
	*curString = ptr;
	return quoteString;
}

// Convert a string into a number by using desired radix.
int util_GetInteger(char *cptr, int radix, int max, int *status)
{
	char *tptr;
	int  val;

	val = util_ToInteger(cptr, &tptr, radix);
	if ((cptr == tptr) || (val > max))
		*status = EMU_ARG;
	else
		*status = EMU_OK;
	return val;
}

// Convert a string into a number by using desired radix.
int util_ToInteger(char *cptr, char **eptr, int radix)
{
	int c;
	int digit, nodigit;
	int val;

	*eptr = cptr;
	if ((radix < 2) || (radix > 36))
		return 0;

	while (isspace(*cptr))
		cptr++;

	val = 0;
	nodigit = 1;
	while (isalnum(c = *cptr)) {
		if (islower(c))
			c = toupper(c);
		if (isdigit(c))
			digit = c - '0';
		else
			digit = c + 10 - 'A';
		if (digit >= radix)
			return 0;
		val = (val * radix) + digit;
		cptr++;
		nodigit = 0;
	}
	if (nodigit)
		return 0;
	*eptr = cptr;
	return val;
}

// Convert a number into a string using base 10.
char *util_ToBase10(uint32 value)
{
	static char outBuffer[32];
	int idx = 31;

	outBuffer[31] = '\0';
	if (!value) {
		outBuffer[30] = '0';
		return outBuffer+30;
	}
	while (value) {
		idx--;
		outBuffer[idx] = '0' + (value % 10);
		value /= 10;
	}
	return outBuffer + idx;
}

void util_ToUpper(register char *curString)
{
	register int idx;

	for (idx = 0; curString[idx]; idx++)
		curString[idx] = toupper(curString[idx]);
}

// DEC Core-Dump file.
// Format for storing 36-bits into 5 disk/tape frames.
//
//    DEC Core-Dump Mode          ANSI Compatible Mode
// |00 01 02 03 04 05 06|07     |__ 00 01 02 03 04 05 06|
//  08 09 10 11 12 13|14 15     |__ 07 08 09 10 11 12 13|
//  16 17 18 19 20|21 22 23     |__ 14 15 16 17 18 19 20|
//  24 25 26 27|28 29 30 31     |__ 21 22 23 24 25 26 27|
//  __ __ __ __ 32 33 34|35|    |35|28 29 30 31 32 33 34|
//
// Note: "|" separate the 7-bit bytes,
// "__" are unused bits (which is zeros).

int36 util_Convert8to36(uchar *data8)
{
	int36 data36;

	data36 = data8[0];
	data36 = (data36 << 8) | data8[1];
	data36 = (data36 << 8) | data8[2];
	data36 = (data36 << 8) | data8[3];
	data36 = (data36 << 4) | data8[4];

	return data36;
}

void util_Convert36to8(int36 data36, uchar *data8)
{
	data8[0] = (data36 >> 28) & 0xFF;
	data8[1] = (data36 >> 20) & 0xFF;
	data8[2] = (data36 >> 12) & 0xFF;
	data8[3] = (data36 >>  4) & 0xFF;
	data8[4] = data36 & 0xF;
}

int36 util_PackedASCII6(uchar *data8)
{
	int36 data36;
	int   idx;

	// Return zero for empty string or null
	if ((data8 == NULL) || (data8[0] == '\0'))
		return 0;

	// Fill first six characters
	data36 = 0;
	for (idx = 0; data8[idx] && (idx < 6); idx++)
		data36 = (data36 << 6) | ((data8[idx] - 040) & 077);

	// Fill zeros rest of 36-bit word
	if (idx < 6)
		data36 <<= (6 - idx) * 6;

	return data36;
}

int36 util_PackedASCII7(uchar *data8)
{
	int36 data36;
	int   idx;

	// Return zero for empty string or null
	if ((data8 == NULL) || (data8[0] == '\0'))
		return 0;

	// Fill first six characters
	data36 = 0;
	for (idx = 0; data8[idx] && (idx < 5); idx++)
		data36 = (data36 << 7) | (data8[idx] & 0177);

	// Fill zeros rest of 36-bit word
	if (idx < 5)
		data36 <<= (5 - idx) * 7;
	data36 <<= 1;

	return data36;
}

char *emu_nowTime(cchar *Format)
{
	time_t now = time(NULL);
	
	if (Format == NULL)
		return ctime(&now);
	else {
		static char tmpTime[1024];
		struct tm   *tmTime;

		tmTime = localtime(&now);
		strftime(tmpTime, 1023, Format, tmTime);

		return tmpTime;
	}
}

void emu_Printf(cchar *Format, ...)
{
	char tmpBuffer[1024];
	va_list Args;
	int len;

	va_start(Args, Format);
	len = vsnprintf(tmpBuffer, 1023, Format, Args);
	tmpBuffer[1023] = 0;
	va_end(Args);

	printf(tmpBuffer);
	if (emu_logFile >= 0)
		write(emu_logFile, tmpBuffer, len);
}

char *StrChar(char *str, char delim)
{
	while ((*str != delim) && (*str != '\0'))
		str++;

	return str;
}
