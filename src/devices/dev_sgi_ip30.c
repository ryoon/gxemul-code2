/*
 *  Copyright (C) 2004-2005  Anders Gavare.  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright  
 *     notice, this list of conditions and the following disclaimer in the 
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
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
 *  $Id: dev_sgi_ip30.c,v 1.13 2005-01-16 15:30:01 debug Exp $
 *  
 *  SGI IP30 stuff.
 *
 *  This is just comprised of hardcoded guesses so far. (Ugly.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "devices.h"
#include "memory.h"
#include "misc.h"


void dev_sgi_ip30_tick(struct cpu *cpu, void *extra)
{
	struct sgi_ip30_data *d = extra;

	d->reg_0x20000 += 1000;

	if (d->imask0 & ((int64_t)1<<50)) {
		/*  TODO: Only interrupt if reg 0x20000 (the counter)
			has passed the compare (0x30000).  */
		cpu_interrupt(cpu, 8+1 + 50);
	}
}


/*
 *  dev_sgi_ip30_access():
 */
int dev_sgi_ip30_access(struct cpu *cpu, struct memory *mem,
	uint64_t relative_addr, unsigned char *data, size_t len,
	int writeflag, void *extra)
{
	struct sgi_ip30_data *d = (struct sgi_ip30_data *) extra;
	uint64_t idata = 0, odata = 0;

	idata = memory_readmax64(cpu, data, len);

	switch (relative_addr) {
	case 0x20:
		/*  Memory bank configuration:  */
		odata = 0x80010000ULL;
		break;
	case 0x10000:	/*  Interrupt mask register 0:  */
		if (writeflag == MEM_WRITE) {
			d->imask0 = idata;
		} else {
			odata = d->imask0;
		}
		break;
	case 0x10018:
		/*
		 *  If this is not implemented, the IP30 PROM complains during bootup:
		 *
		 *           *FAILED*
		 *    Address: 0xffffffffaff10018, Expected: 0x0000000000000001, Received: 0x0000000000000000
		 */
		if (writeflag == MEM_WRITE) {
			d->reg_0x10018 = idata;
		} else {
			odata = d->reg_0x10018;
		}
		break;
	case 0x10020:	/*  Set ISR, according to Linux/IP30  */
		d->isr = idata;
		/*  Recalculate CPU interrupt assertions:  */
		cpu_interrupt(cpu, 8);
		break;
	case 0x10028:	/*  Clear ISR, according to Linux/IP30  */
		d->isr &= ~idata;
		/*  Recalculate CPU interrupt assertions:  */
		cpu_interrupt(cpu, 8);
		break;
	case 0x10030:	/*  Interrupt Status Register  */
		if (writeflag == MEM_WRITE) {
			/*  Clear-on-write  (TODO: is this correct?)  */
			d->isr &= ~idata;
			/*  Recalculate CPU interrupt assertions:  */
			cpu_interrupt(cpu, 8);
		} else {
			odata = d->isr;
		}
		break;
	case 0x20000:
		/*  A counter  */
		if (writeflag == MEM_WRITE) {
			d->reg_0x20000 = idata;
		} else {
			odata = d->reg_0x20000;
		}
		break;
	case 0x30000:
		if (writeflag == MEM_WRITE) {
			d->reg_0x30000 = idata;
		} else {
			odata = d->reg_0x30000;
		}
		break;
	default:
		if (writeflag == MEM_WRITE) {
			debug("[ sgi_ip30: unimplemented write to address 0x%x, data=0x%02x ]\n", relative_addr, idata);
		} else {
			debug("[ sgi_ip30: unimplemented read from address 0x%x ]\n", relative_addr);
		}
	}

	if (writeflag == MEM_READ)
		memory_writemax64(cpu, data, len, odata);

	return 1;
}


/*
 *  dev_sgi_ip30_2_access():
 */
int dev_sgi_ip30_2_access(struct cpu *cpu, struct memory *mem,
	uint64_t relative_addr, unsigned char *data, size_t len,
	int writeflag, void *extra)
{
	struct sgi_ip30_data *d = (struct sgi_ip30_data *) extra;
	uint64_t idata = 0, odata = 0;

	idata = memory_readmax64(cpu, data, len);

	switch (relative_addr) {
	case 0x0029c:
		/*
		 *  If this is not implemented, the IP30 PROM complains during bootup:
		 *
		 *           *FAILED*
		 *    Address: 0xffffffffb000029c, Expected: 0x0000000000000001, Received: 0x0000000000000000
		 */
		if (writeflag == MEM_WRITE) {
			d->reg_0x0029c = idata;
		} else {
			odata = d->reg_0x0029c;
		}
		break;
	default:
		if (writeflag == MEM_WRITE) {
			debug("[ sgi_ip30_2: unimplemented write to address 0x%x, data=0x%02x ]\n", relative_addr, idata);
		} else {
			debug("[ sgi_ip30_2: unimplemented read from address 0x%x ]\n", relative_addr);
		}
	}

	if (writeflag == MEM_READ)
		memory_writemax64(cpu, data, len, odata);

	return 1;
}


/*
 *  dev_sgi_ip30_3_access():
 */
int dev_sgi_ip30_3_access(struct cpu *cpu, struct memory *mem,
	uint64_t relative_addr, unsigned char *data, size_t len,
	int writeflag, void *extra)
{
	struct sgi_ip30_data *d = (struct sgi_ip30_data *) extra;
	uint64_t idata = 0, odata = 0;

	idata = memory_readmax64(cpu, data, len);

	switch (relative_addr) {
	case 0xb4:
		if (writeflag == MEM_WRITE) {
			debug("[ sgi_ip30_3: unimplemented write to address 0x%x, data=0x%02x ]\n", relative_addr, idata);
		} else {
			odata = 2;	/*  should be 2, or Irix loops  */
		}
		break;
	case 0x00104:
		if (writeflag == MEM_WRITE) {
			debug("[ sgi_ip30_3: unimplemented write to address 0x%x, data=0x%02x ]\n", relative_addr, idata);
		} else {
			odata = 64;	/*  should be 64, or the PROM complains  */
		}
		break;
	case 0x00284:
		/*
		 *  If this is not implemented, the IP30 PROM complains during bootup:
		 *
		 *           *FAILED*
		 *    Address: 0xffffffffbf000284, Expected: 0x0000000000000001, Received: 0x0000000000000000
		 */
		if (writeflag == MEM_WRITE) {
			d->reg_0x00284 = idata;
		} else {
			odata = d->reg_0x00284;
		}
		break;
	default:
		if (writeflag == MEM_WRITE) {
			debug("[ sgi_ip30_3: unimplemented write to address 0x%x, data=0x%02x ]\n", relative_addr, idata);
		} else {
			debug("[ sgi_ip30_3: unimplemented read from address 0x%x ]\n", relative_addr);
		}
	}

	if (writeflag == MEM_READ)
		memory_writemax64(cpu, data, len, odata);

	return 1;
}


/*
 *  dev_sgi_ip30_4_access():
 */
int dev_sgi_ip30_4_access(struct cpu *cpu, struct memory *mem,
	uint64_t relative_addr, unsigned char *data, size_t len,
	int writeflag, void *extra)
{
	struct sgi_ip30_data *d = (struct sgi_ip30_data *) extra;
	uint64_t idata = 0, odata = 0;

	idata = memory_readmax64(cpu, data, len);

	switch (relative_addr) {
	case 0x000b0:
		/*
		 *  If this is not implemented, the IP30 PROM complains during bootup:
		 *
		 *           *FAILED*
		 *    Address: 0xffffffffbf6000b0, Expected: 0x0000000000000001, Received: 0x0000000000000000
		 */
		if (writeflag == MEM_WRITE) {
			d->reg_0x000b0 = idata;
		} else {
			odata = d->reg_0x000b0;
		}
		break;
	default:
		if (writeflag == MEM_WRITE) {
			debug("[ sgi_ip30_4: unimplemented write to address 0x%x, data=0x%02x ]\n", relative_addr, idata);
		} else {
			debug("[ sgi_ip30_4: unimplemented read from address 0x%x ]\n", relative_addr);
		}
	}

	if (writeflag == MEM_READ)
		memory_writemax64(cpu, data, len, odata);

	return 1;
}


/*
 *  dev_sgi_ip30_5_access():
 */
int dev_sgi_ip30_5_access(struct cpu *cpu, struct memory *mem,
	uint64_t relative_addr, unsigned char *data, size_t len,
	int writeflag, void *extra)
{
	struct sgi_ip30_data *d = (struct sgi_ip30_data *) extra;
	uint64_t idata = 0, odata = 0;

	idata = memory_readmax64(cpu, data, len);

	switch (relative_addr) {
	case 0x00000:
		if (writeflag == MEM_WRITE) {
			d->reg_0x00000 = idata;
		} else {
			odata = d->reg_0x00000;
		}
		break;
	default:
		if (writeflag == MEM_WRITE) {
			debug("[ sgi_ip30_5: unimplemented write to address 0x%x, data=0x%02x ]\n", relative_addr, idata);
		} else {
			debug("[ sgi_ip30_5: unimplemented read from address 0x%x ]\n", relative_addr);
		}
	}

	if (writeflag == MEM_READ)
		memory_writemax64(cpu, data, len, odata);

	return 1;
}


/*
 *  dev_sgi_ip30_init():
 */
struct sgi_ip30_data *dev_sgi_ip30_init(struct cpu *cpu, struct memory *mem, uint64_t baseaddr)
{
	struct sgi_ip30_data *d = malloc(sizeof(struct sgi_ip30_data));
	if (d == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	memset(d, 0, sizeof(struct sgi_ip30_data));

	memory_device_register(mem, "sgi_ip30_1", baseaddr,
	    DEV_SGI_IP30_LENGTH, dev_sgi_ip30_access, (void *)d, MEM_DEFAULT, NULL);
	memory_device_register(mem, "sgi_ip30_2", 0x10000000,
	    0x10000, dev_sgi_ip30_2_access, (void *)d, MEM_DEFAULT, NULL);
	memory_device_register(mem, "sgi_ip30_3", 0x1f000000,
	    0x10000, dev_sgi_ip30_3_access, (void *)d, MEM_DEFAULT, NULL);
	memory_device_register(mem, "sgi_ip30_4", 0x1f600000,
	    0x10000, dev_sgi_ip30_4_access, (void *)d, MEM_DEFAULT, NULL);
	memory_device_register(mem, "sgi_ip30_5", 0x1f6c0000,
	    0x10000, dev_sgi_ip30_5_access, (void *)d, MEM_DEFAULT, NULL);

	cpu_add_tickfunction(cpu, dev_sgi_ip30_tick, d, 16);

	return d;
}

