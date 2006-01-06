/*
 *  Copyright (C) 2005-2006  Anders Gavare.  All rights reserved.
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
 *  $Id: machine_prep.c,v 1.1 2006-01-01 20:56:24 debug Exp $
 */

#include <stdio.h>
#include <string.h>

#include "bus_isa.h"
#include "bus_pci.h"
#include "cpu.h"
#include "device.h"
#include "devices.h"
#include "machine.h"
#include "machine_interrupts.h"
#include "memory.h"
#include "misc.h"



MACHINE_SETUP(prep)
{
	struct pci_data *pci_data;

	/*
	 *  NetBSD/prep (http://www.netbsd.org/Ports/prep/)
	 */

	machine->machine_name = "PowerPC Reference Platform";
	machine->stable = 1;

	if (machine->emulated_hz == 0)
		machine->emulated_hz = 20000000;

	machine->md_int.bebox_data = device_add(machine, "prep");
	machine->isa_pic_data.native_irq = 1;	/*  Semi-bogus  */
	machine->md_interrupt = isa32_interrupt;

	pci_data = dev_eagle_init(machine, machine->memory,
	    32 /*  isa irq base */, 0 /*  pci irq: TODO */);

	bus_isa_init(machine, BUS_ISA_IDE0 | BUS_ISA_IDE1,
	    0x80000000, 0xc0000000, 32, 48);

	bus_pci_add(machine, pci_data, machine->memory, 0, 13, 0, "dec21143");

	if (machine->use_x11) {
		bus_pci_add(machine, pci_data, machine->memory,
		    0, 14, 0, "s3_virge");
	}

	if (machine->prom_emulation) {
		/*  Linux on PReP has 0xdeadc0de at address 0? (See
		    http://joshua.raleigh.nc.us/docs/linux-2.4.10_html/
		    113568.html)  */
		store_32bit_word(cpu, 0, 0xdeadc0de);

		/*
		 *  r4 should point to first free byte after the loaded kernel.
		 *  r6 should point to bootinfo.
		 */
		cpu->cd.ppc.gpr[4] = 6 * 1048576;
		cpu->cd.ppc.gpr[6] = machine->physical_ram_in_mb*1048576-0x8000;

		/*
		 *  (See NetBSD's prep/include/bootinfo.h for details.)
		 *
		 *  32-bit "next" offset;
		 *  32-bit "type";
		 *  type-specific data...
		 */

		/*  type: clock  */
		store_32bit_word(cpu, cpu->cd.ppc.gpr[6]+ 0, 12);
		store_32bit_word(cpu, cpu->cd.ppc.gpr[6]+ 4, 2);
		store_32bit_word(cpu, cpu->cd.ppc.gpr[6]+ 8,
		    machine->emulated_hz);

		/*  type: console  */
		store_32bit_word(cpu, cpu->cd.ppc.gpr[6]+12, 20);
		store_32bit_word(cpu, cpu->cd.ppc.gpr[6]+16, 1);
		store_buf(cpu, cpu->cd.ppc.gpr[6] + 20,
		    machine->use_x11? "vga" : "com", 4);
		store_32bit_word(cpu, cpu->cd.ppc.gpr[6]+24, 0x3f8);/*  addr  */
		store_32bit_word(cpu, cpu->cd.ppc.gpr[6]+28, 9600);/*  speed  */

		/*  type: residual  */
		store_32bit_word(cpu, cpu->cd.ppc.gpr[6]+32, 0);
		store_32bit_word(cpu, cpu->cd.ppc.gpr[6]+36, 0);
		store_32bit_word(cpu, cpu->cd.ppc.gpr[6]+40,/*  addr of data  */
		    cpu->cd.ppc.gpr[6] + 0x100);

		/*  Residual data:  (TODO)  */
		store_32bit_word(cpu, cpu->cd.ppc.gpr[6]+0x100, 0x200);
		/*  store_string(cpu, cpu->cd.ppc.gpr[6]+0x100+0x8,
		    "IBM PPS Model 7248 (E)");  */
		store_string(cpu, cpu->cd.ppc.gpr[6]+0x100+0x8,
		    "IBM PPS Model 6050/6070 (E)");
		store_32bit_word(cpu, cpu->cd.ppc.gpr[6]+0x100+0x1f8,
		    machine->physical_ram_in_mb * 1048576);  /*  memsize  */
	}
}


MACHINE_DEFAULT_CPU(prep)
{
	/*  NOTE/TODO: The actual CPU type differs between different
	    PReP models!  */
	machine->cpu_name = strdup("PPC604");
}


MACHINE_DEFAULT_RAM(prep)
{
	machine->physical_ram_in_mb = 64;
}


MACHINE_REGISTER(prep)
{
	MR_DEFAULT(prep, "PowerPC Reference Platform", ARCH_PPC,
	    MACHINE_PREP, 1, 0);
	me->aliases[0] = "prep";
	me->set_default_ram = machine_default_ram_prep;
	machine_entry_add(me, ARCH_PPC);
}
