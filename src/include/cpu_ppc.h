#ifndef	CPU_PPC_H
#define	CPU_PPC_H

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
 *  $Id: cpu_ppc.h,v 1.1 2005-01-30 14:06:43 debug Exp $
 */

#include "misc.h"

#define	N_PPC_GPRS		32

struct ppc_cpu {
	/*  General purpose registers:  */
	uint64_t	gpr[N_PPC_GPRS];
};


/*  cpu_ppc.c:  */
struct cpu *mips_cpu_new(struct memory *mem, struct machine *machine,
        int cpu_id, char *cpu_type_name);
void mips_cpu_show_full_statistics(struct machine *m);
void mips_cpu_tlbdump(struct machine *m, int x, int rawflag);
void mips_cpu_register_match(struct machine *m, char *name, 
	int writeflag, uint64_t *valuep, int *match_register);
void mips_cpu_register_dump(struct cpu *cpu, int gprs, int coprocs);
void mips_cpu_disassemble_instr(struct cpu *cpu, unsigned char *instr,
        int running, uint64_t addr, int bintrans);
int mips_cpu_interrupt(struct cpu *cpu, int irq_nr);
int mips_cpu_interrupt_ack(struct cpu *cpu, int irq_nr);
void mips_cpu_exception(struct cpu *cpu, int exccode, int tlb, uint64_t vaddr,
        /*  uint64_t pagemask,  */  int coproc_nr, uint64_t vaddr_vpn2,
        int vaddr_asid, int x_64);
void mips_cpu_cause_simple_exception(struct cpu *cpu, int exc_code);
void mips_cpu_run_init(struct emul *emul, struct machine *machine);
int mips_cpu_run(struct emul *emul, struct machine *machine);
void mips_cpu_run_deinit(struct emul *emul, struct machine *machine);
void mips_cpu_dumpinfo(struct cpu *cpu);
void ppc_cpu_list_available_types(void);


#endif	/*  CPU_PPC_H  */
