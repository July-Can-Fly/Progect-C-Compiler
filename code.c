#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include"code.h"

extern char* binary[];
extern char hexChar[]; 

 /*Function that convert Decimal to Binary and Hexadecimal numbers*/
void intToBinArray(unsigned int n, char* binArray, char *hexArray)
{
    int i, j, hexDigit;
	for (i = 0; i < 8; i+=2)
	{
		for (j = 1; j >= 0; j--)
		{
			hexDigit = n % 16;
			memcpy(binArray + (i+j) * 4, binary[hexDigit], 4);
			hexArray[i+j] = hexChar[hexDigit];
			n /= 16;
		}
	}
}
