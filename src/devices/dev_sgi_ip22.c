/*
 *  Copyright (C) 2004 by Anders Gavare.  All rights reserved.
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
 *  $Id: dev_sgi_ip22.c,v 1.9 2004-06-10 04:23:04 debug Exp $
 *  
 *  SGI IP22 stuff.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "imcreg.h"
#include "memory.h"
#include "misc.h"
#include "devices.h"


struct sgi_ip22_data {
	uint32_t	reg[DEV_SGI_IP22_LENGTH / 4];

	uint32_t	imc_cpuctrl0;
	uint32_t	imc_wdog;
};


/*
 *  dev_sgi_ip22_tick():
 */
void dev_sgi_ip22_tick(struct cpu *cpu, void *extra)
{
	struct sgi_ip22_data *d = (struct sgi_ip22_data *) extra;

	if (d->reg[0x38 / 4] != 0)
		d->reg[0x38 / 4] --;
}


/*
 *  dev_sgi_ip22_memctl_access():
 *
 *  Returns 1 if ok, 0 on error.
 */
int dev_sgi_ip22_memctl_access(struct cpu *cpu, struct memory *mem, uint64_t relative_addr, unsigned char *data, size_t len, int writeflag, void *extra)
{
	struct sgi_ip22_data *d = (struct sgi_ip22_data *) extra;
	uint64_t idata = 0, odata = 0;

	idata = memory_readmax64(cpu, data, len);

	switch (relative_addr) {
	case (IMC_CPUCTRL0 - 0x1fa00000):
		if (writeflag == MEM_WRITE) {
			d->imc_cpuctrl0 = idata;
			/*  debug("[ sgi_ip22_memctl: write to IMC_CPUCTRL0, data=0x%08x ]\n", (int)idata);  */
		} else {
			odata = d->imc_cpuctrl0;
			/*  debug("[ sgi_ip22_memctl: read from IMC_CPUCTRL0, data=0x%08x ]\n", (int)odata);  */
		}
		break;
	case (IMC_SYSID - 0x1fa00000):
		if (writeflag == MEM_WRITE) {
			debug("[ sgi_ip22_memctl: unimplemented write IMC_SYSID, data=0x%08x ]\n", (int)idata);
		} else {
			/*  Lowest 4 bits are the revision bits.  */
			odata = 3 + IMC_SYSID_HAVEISA;
			/*  debug("[ sgi_ip22_memctl: read from IMC_SYSID, data=0x%08x ]\n", (int)odata);  */
		}
		break;
	case (IMC_WDOG - 0x1fa00000):
		if (writeflag == MEM_WRITE) {
			d->imc_wdog = idata;
			/*  debug("[ sgi_ip22_memctl: write to IMC_WDOG, data=0x%08x ]\n", (int)idata);  */
		} else {
			odata = d->imc_wdog;
			/*  debug("[ sgi_ip22_memctl: read from IMC_WDOG, data=0x%08x ]\n", (int)odata);  */
		}
		break;
	default:
		if (writeflag == MEM_WRITE) {
			debug("[ sgi_ip22_memctl: unimplemented write to address 0x%x, data=0x%08x ]\n", relative_addr, (int)idata);
		} else {
			debug("[ sgi_ip22_memctl: unimplemented read from address 0x%x, data=0x%08x ]\n", relative_addr, (int)odata);
		}
	}

	if (writeflag == MEM_READ)
		memory_writemax64(cpu, data, len, odata);

	return 1;
}


/*
 *  dev_sgi_ip22_sysid_access():
 *
 *  Returns 1 if ok, 0 on error.
 */
int dev_sgi_ip22_sysid_access(struct cpu *cpu, struct memory *mem, uint64_t relative_addr, unsigned char *data, size_t len, int writeflag, void *extra)
{
	uint64_t idata = 0, odata = 0;

	idata = memory_readmax64(cpu, data, len);

	if (writeflag == MEM_WRITE) {
		debug("[ sgi_ip22_sysid: write to address 0x%x, data=0x%08x ]\n", relative_addr, (int)idata);
	} else {
		debug("[ sgi_ip22_sysid: read from address 0x%x, data=0x%08x ]\n", relative_addr, (int)odata);
		/*
		 *  According to NetBSD's sgimips/ip22.c:
		 *        printf("IOC rev %d, machine %s, board rev %d\n", (sysid >> 5) & 0x07,
		 *                        (sysid & 1) ? "Indigo2 (Fullhouse)" : "Indy (Guiness)",
		 *                        (sysid >> 1) & 0x0f);
		 */

		/*  IOC rev 1, Guiness, board rev 3:  */
		odata = (1 << 5) + (3 << 1) + 0;
	}

	if (writeflag == MEM_READ)
		memory_writemax64(cpu, data, len, odata);

	return 1;
}


/*
 *  dev_sgi_ip22_access():
 *
 *  Returns 1 if ok, 0 on error.
 */
int dev_sgi_ip22_access(struct cpu *cpu, struct memory *mem, uint64_t relative_addr, unsigned char *data, size_t len, int writeflag, void *extra)
{
	struct sgi_ip22_data *d = (struct sgi_ip22_data *) extra;
	uint64_t idata = 0, odata = 0;
	int regnr;

	idata = memory_readmax64(cpu, data, len);
	regnr = relative_addr / sizeof(uint32_t);

	if (writeflag == MEM_WRITE)
		d->reg[regnr] = idata;
	else
		odata = d->reg[regnr];

	/*  Read from/write to the sgi_ip22:  */
	switch (relative_addr) {
	case 0x38:	/*  timer count  */
		if (writeflag == MEM_WRITE) {
			/*  Two byte values are written to this address, sequentially...  TODO  */
		} else {
			/*  The timer is decreased by the tick function.  */
		}
		break;
	case 0x3c:	/*  timer control  */
		break;
	default:
		if (writeflag == MEM_WRITE) {
			debug("[ sgi_ip22: unimplemented write to address 0x%x, data=0x%02x ]\n", relative_addr, idata);
		} else {
			debug("[ sgi_ip22: unimplemented read from address 0x%x ]\n", relative_addr);
		}
	}

	if (writeflag == MEM_READ)
		memory_writemax64(cpu, data, len, odata);

	return 1;
}


/*
 *  dev_sgi_ip22_init():
 */
void dev_sgi_ip22_init(struct cpu *cpu, struct memory *mem, uint64_t baseaddr)
{
	struct sgi_ip22_data *d = malloc(sizeof(struct sgi_ip22_data));
	if (d == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	memset(d, 0, sizeof(struct sgi_ip22_data));

	memory_device_register(mem, "sgi_ip22", baseaddr, DEV_SGI_IP22_LENGTH, dev_sgi_ip22_access, (void *)d);
	cpu_add_tickfunction(cpu, dev_sgi_ip22_tick, d, 10);

	memory_device_register(mem, "sgi_ip22_sysid", 0x1fbd9858, 0x8, dev_sgi_ip22_sysid_access, (void *)d);

	memory_device_register(mem, "sgi_ip22_memctl", 0x1fa00000, DEV_SGI_IP22_MEMCTL_LENGTH, dev_sgi_ip22_memctl_access, (void *)d);
}

