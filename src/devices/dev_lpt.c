/*
 *  Copyright (C) 2005  Anders Gavare.  All rights reserved.
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
 *  $Id: dev_lpt.c,v 1.5 2005-11-13 00:14:09 debug Exp $
 *
 *  LPT (parallel printer) controller.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "console.h"
#include "cpu.h"
#include "device.h"
#include "machine.h"
#include "memory.h"
#include "misc.h"

#include "lptreg.h"


/*  #define debug fatal  */

#define	TICK_SHIFT		18
#define	DEV_LPT_LENGTH		3

struct lpt_data {
	int		in_use;
	int		irqnr;
	char		*name;
	int		console_handle;

	unsigned char	data;
	unsigned char	control;
};


/*
 *  dev_lpt_tick():
 */
void dev_lpt_tick(struct cpu *cpu, void *extra)
{
	/*  struct lpt_data *d = extra;  */

	/*  TODO  */
}


/*
 *  dev_lpt_access():
 */
int dev_lpt_access(struct cpu *cpu, struct memory *mem,
	uint64_t relative_addr, unsigned char *data, size_t len,
	int writeflag, void *extra)
{
	uint64_t idata = 0, odata=0;
	struct lpt_data *d = extra;

	if (writeflag == MEM_WRITE)
		idata = memory_readmax64(cpu, data, len);

	switch (relative_addr) {

	case LPT_DATA:
		if (writeflag == MEM_READ)
			odata = d->data;
		else
			d->data = idata;
		break;

	case LPT_STATUS:
		odata = LPS_NBSY | LPS_NACK | LPS_SELECT | LPS_NERR;
		break;

	case LPT_CONTROL:
		if (writeflag == MEM_WRITE) {
			if (idata != d->control) {
				if (idata & LPC_STROBE) {
					/*  data strobe  */
					console_putchar(d->console_handle,
					    d->data);
				}
			}
			d->control = idata;
		}
		break;
	}

	if (writeflag == MEM_READ)
		memory_writemax64(cpu, data, len, odata);

	return 1;
}


/*
 *  devinit_lpt():
 */
int devinit_lpt(struct devinit *devinit)
{
	struct lpt_data *d = malloc(sizeof(struct lpt_data));
	size_t nlen;
	char *name;

	if (d == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	memset(d, 0, sizeof(struct lpt_data));
	d->irqnr	= devinit->irq_nr;
	d->in_use	= devinit->in_use;
	d->name		= devinit->name2 != NULL? devinit->name2 : "";
	d->console_handle =
	    console_start_slave(devinit->machine, devinit->name);

	nlen = strlen(devinit->name) + 10;
	if (devinit->name2 != NULL)
		nlen += strlen(devinit->name2);
	name = malloc(nlen);
	if (name == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	if (devinit->name2 != NULL && devinit->name2[0])
		snprintf(name, nlen, "%s [%s]", devinit->name, devinit->name2);
	else
		snprintf(name, nlen, "%s", devinit->name);

	memory_device_register(devinit->machine->memory, name, devinit->addr,
	    DEV_LPT_LENGTH, dev_lpt_access, d, DM_DEFAULT, NULL);
	machine_add_tickfunction(devinit->machine, dev_lpt_tick, d, TICK_SHIFT);

	/*
	 *  NOTE:  Ugly cast into a pointer, because this is a convenient way
	 *         to return the console handle to code in src/machine.c.
	 */
	devinit->return_ptr = (void *)(size_t)d->console_handle;

	return 1;
}
