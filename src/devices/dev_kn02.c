/*
 *  Copyright (C) 2003-2004  Anders Gavare.  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright  
 *     notice, this list of conditions and the following disclaimer in the 
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE   
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 *   
 *
 *  $Id: dev_kn02.c,v 1.11 2004-11-17 20:37:39 debug Exp $
 *  
 *  DEC (KN02) stuff.  See include/dec_kn02.h for more info.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "misc.h"
#include "devices.h"


/*
 *  dev_kn02_access():
 */
int dev_kn02_access(struct cpu *cpu, struct memory *mem,
	uint64_t relative_addr, unsigned char *data, size_t len,
	int writeflag, void *extra)
{
	struct kn02_csr *d = extra;
	uint64_t idata = 0, odata = 0;

	idata = memory_readmax64(cpu, data, len);

	switch (relative_addr) {
	case 0:
		if (writeflag==MEM_READ) {
			odata = d->csr;
			/* debug("[ kn02: read from CSR: 0x%08x ]\n", odata); */
		} else {
			/*
			 *  Only bits 23..8 are considered writable. The
			 *  lowest 8 bits are actually writable, but don't
			 *  affect the interrupt I/O bits; the low 8 bits
			 *  on write turn on and off LEDs.  (There are no
			 *  LEDs in the emulator, so those bits are just
			 *  ignored.)
			 */
			/* fatal("[ kn02: write to CSR: 0x%08x ]\n", idata); */

			idata &= 0x00ffff00;
			d->csr = (d->csr & 0xff0000ffULL) | idata;

			/*  Recalculate interrupt assertions:  */
			cpu_interrupt(cpu, 8);
		}
		break;
	default:
		if (writeflag==MEM_READ) {
			debug("[ kn02: read from 0x%08lx ]\n", (long)relative_addr);
		} else {
			debug("[ kn02: write to  0x%08lx: 0x%08x ]\n", (long)relative_addr, idata);
		}
	}

	if (writeflag == MEM_READ)
		memory_writemax64(cpu, data, len, odata);

	return 1;
}


/*
 *  dev_kn02_init():
 */
struct kn02_csr *dev_kn02_init(struct cpu *cpu, struct memory *mem, uint64_t baseaddr)
{
	struct kn02_csr *d;

	d = malloc(sizeof(struct kn02_csr));
	if (d == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	memset(d, 0, sizeof(struct kn02_csr));

	memory_device_register(mem, "kn02", baseaddr, DEV_KN02_LENGTH,
	    dev_kn02_access, d, MEM_DEFAULT, NULL);

	return d;
}

