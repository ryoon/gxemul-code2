/*
 *  GXemul demo:  Hello World
 *
 *  This file is in the Public Domain.
 */

#include "dev_cons.h"


#ifdef MIPS
/*  Note: The ugly cast to a signed int (32-bit) causes the address to be
	sign-extended correctly on MIPS when compiled in 64-bit mode  */
#define	PHYSADDR_OFFSET		((signed int)0xa0000000)
#else
#define	PHYSADDR_OFFSET		0
#endif


#define	PUTCHAR_ADDRESS		(PHYSADDR_OFFSET +		\
				DEV_CONS_ADDRESS + DEV_CONS_PUTGETCHAR)
#define	HALT_ADDRESS		(PHYSADDR_OFFSET +		\
				DEV_CONS_ADDRESS + DEV_CONS_HALT)


void printchar(char ch)
{
	*((volatile unsigned char *) PUTCHAR_ADDRESS) = ch;
}


void halt(void)
{
	*((volatile unsigned char *) HALT_ADDRESS) = 0;
}


void printstr(char *s)
{
	while (*s)
		printchar(*s++);
}


void f(void)
{
	printstr("Hello world\n");
	halt();
}

