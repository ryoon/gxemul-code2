/*
 *  Copyright (C) 2006  Anders Gavare.  All rights reserved.
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
 *  $Id: dev_nvram.c,v 1.4 2006-02-09 20:02:59 debug Exp $
 *
 *  NVRAM reached through ISA port 0x74-0x77, and a wrapper for an MK48Txx
 *  RTC. (See dev_pccmos.c for the traditional PC-style CMOS/RTC device.)
 *
 *  TODO: Perhaps implement flags for which parts of ram that are actually
 *        implemented, and warn when accesses occur to other parts?
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "device.h"
#include "devices.h"
#include "emul.h"
#include "machine.h"
#include "memory.h"
#include "misc.h"


#include "mk48txxreg.h"


#define	DEV_NVRAM_LENGTH		4

#define	MK48TXX_FAKE_ADDR	0x1d80000000ULL

struct nvram_data {
	uint16_t	reg_select;
	unsigned char	ram[65536];

	int		rtc_offset;
	int		rtc_irq;
};


/*
 *  dev_nvram_access():
 */
DEVICE_ACCESS(nvram)
{
	struct nvram_data *d = (struct nvram_data *) extra;
	uint64_t idata = 0, odata = 0;

	if (writeflag == MEM_WRITE)
		idata = memory_readmax64(cpu, data, len);

	switch (relative_addr) {

	case 0:	if (writeflag == MEM_WRITE) {
			d->reg_select &= ~0xff;
			d->reg_select |= (idata & 0xff);
		} else {
			odata = d->reg_select & 0xff;
		}
		break;

	case 1:	if (writeflag == MEM_WRITE) {
			d->reg_select &= ~0xff00;
			d->reg_select |= ((idata & 0xff) << 8);
		} else {
			odata = (d->reg_select >> 8) & 0xff;
		}
		break;

	case 3:	if (writeflag == MEM_WRITE) {
			if (d->reg_select >= d->rtc_offset + 8 &&
			    d->reg_select < d->rtc_offset + 0x10) {
				/*  RTC access:  */
				unsigned char b = idata;
				cpu->memory_rw(cpu, cpu->mem,
				    MK48TXX_FAKE_ADDR + d->reg_select -
				    d->rtc_offset, &b, 1, MEM_WRITE,
				    PHYSICAL);
			} else {
				/*  NVRAM access:  */
				d->ram[d->reg_select] = idata;
			}
		} else {
			if (d->reg_select >= d->rtc_offset + 8 &&
			    d->reg_select < d->rtc_offset + 0x10) {
				/*  RTC access:  */
				unsigned char b;
				cpu->memory_rw(cpu, cpu->mem,
				    MK48TXX_FAKE_ADDR + d->reg_select -
				    d->rtc_offset, &b, 1, MEM_READ,
				    PHYSICAL);
				odata = b;
			} else {
				/*  NVRAM access:  */
				odata = d->ram[d->reg_select];
			}
		}
		break;

	default:fatal("[ nvram: unimplemented access to offset %i ]\n",
		    (int)relative_addr);
	}

	if (writeflag == MEM_READ)
		memory_writemax64(cpu, data, len, odata);

	return 1;
}


DEVINIT(nvram)
{
	char tmpstr[100];
	struct nvram_data *d = malloc(sizeof(struct nvram_data));
	if (d == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	memset(d, 0, sizeof(struct nvram_data));

	if (strcmp(devinit->name2, "mvme1600") == 0) {
		/*
		 *  MVME1600 boards have a board ID at 0x1ef8 - 0x1ff7,
		 *  with the following layout:  (see NetBSD/mvmeppc for details)
		 *
		 *  0x1ef8   4 bytes	version
		 *  0x1efc  12 bytes	serial
		 *  0x1f08  16 bytes	id
		 *  0x1f18  16 bytes	pwa
		 *  0x1f28   4 bytes	reserved
		 *  0x1f2c   6 bytes	ethernet address
		 *  0x1f32   2 bytes	reserved
		 *  0x1f34   2 bytes	scsi id
		 *  0x1f36   3 bytes	speed_mpu
		 *  0x1f39   3 bytes	speed_bus
		 *  0x1f3c 187 bytes	reserved
		 *  0x1ff7   1 byte	cksum
		 *  0x1ff8-0x1fff = offsets 8..15 of the MK48T18 RTC
		 *
		 *  Example of values from a real machine (according to Google):
		 *  Model: MVME1603-051, Serial: 2451669, PWA: 01-W3066F01E
		 */

		/*  serial:  */
		memcpy(&d->ram[0x1efc], "1234", 5);  /*  includes nul  */

		/*  id:  */
		memcpy(&d->ram[0x1f08], "MVME1600", 9);  /*  includes nul  */

		/*  pwa:  */
		memcpy(&d->ram[0x1f18], "0", 2);  /*  includes nul  */

		/*  speed_mpu:  */
		memcpy(&d->ram[0x1f36], "33", 3);  /*  includes nul  */

		/*  speed_bus:  */
		memcpy(&d->ram[0x1f39], "33", 3);  /*  includes nul  */

		d->rtc_offset = MK48T18_CLKOFF;
		d->rtc_irq = 32 + 8;
	} else {
		fatal("Unimplemented NVRAM type '%s'\n", devinit->name2);
		exit(1);
	}

	memory_device_register(devinit->machine->memory, devinit->name,
	    devinit->addr, DEV_NVRAM_LENGTH, dev_nvram_access, (void *)d,
	    DM_DEFAULT, NULL);

	snprintf(tmpstr, sizeof(tmpstr), "mk48txx addr=0x%llx irq=%i",
	    (long long)MK48TXX_FAKE_ADDR, d->rtc_irq);
	device_add(devinit->machine, tmpstr);

	return 1;
}
