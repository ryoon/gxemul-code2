#ifndef	CPU_IA64_H
#define	CPU_IA64_H

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
 *  $Id: cpu_ia64.h,v 1.1 2005-08-07 23:36:50 debug Exp $
 */

#include "misc.h"


struct cpu_family;

#define	IA64_N_IC_ARGS			3
#define	IA64_IC_ENTRIES_SHIFT		8
#define	IA64_IC_ENTRIES_PER_PAGE	(1 << IA64_IC_ENTRIES_SHIFT)
#define	IA64_PC_TO_IC_ENTRY(a)		(((a)>>4)&(IA64_IC_ENTRIES_PER_PAGE-1))
#define	IA64_ADDR_TO_PAGENR(a)		((a) >> (IA64_IC_ENTRIES_SHIFT+2))

/*  TODO  */
struct ia64_instr_call {
	void	(*f)(struct cpu *, struct ia64_instr_call *);
	size_t	arg[IA64_N_IC_ARGS];
};

/*  Translation cache struct for each physical page:  */
struct ia64_tc_physpage {
	uint32_t	next_ofs;	/*  or 0 for end of chain  */
	uint32_t	physaddr;
	int		flags;
	struct ia64_instr_call ics[IA64_IC_ENTRIES_PER_PAGE + 1];
};


/*
 *  Virtual->physical->host page entry:
 *
 *	38 + 14 + 12 bits = 64 bits
 *
 *  TODO!!!!
 */
#define	IA64_LEVEL0_SHIFT		26
#define	IA64_LEVEL0			8192
#define	IA64_LEVEL1_SHIFT		12
#define	IA64_LEVEL1			16384
struct ia64_vph_page {
	void		*host_load[IA64_LEVEL1];
	void		*host_store[IA64_LEVEL1];
	uint64_t	phys_addr[IA64_LEVEL1];
	struct ia64_tc_physpage *phys_page[IA64_LEVEL1];
	int		refcount;
	struct ia64_vph_page	*next;	/*  Freelist, used if refcount = 0.  */
};


#define	IA64_MAX_VPH_TLB_ENTRIES	48
struct ia64_vpg_tlb_entry {
	int		valid;
	int		writeflag;
	int64_t		timestamp;
	unsigned char	*host_page;
	uint64_t	vaddr_page;
	uint64_t	paddr_page;
};

struct ia64_cpu {
	/*  TODO  */
	uint64_t	r[128];


	/*
	 *  Instruction translation cache:
	 */

	/*  cur_ic_page is a pointer to an array of IA64_IC_ENTRIES_PER_PAGE
	    instruction call entries. next_ic points to the next such
	    call to be executed.  */
	struct ia64_tc_physpage *cur_physpage;
	struct ia64_instr_call	*cur_ic_page;
	struct ia64_instr_call	*next_ic;


	/*
	 *  Virtual -> physical -> host address translation:
	 */
	struct ia64_vpg_tlb_entry vph_tlb_entry[IA64_MAX_VPH_TLB_ENTRIES];
	struct ia64_vph_page	*vph_default_page;
	struct ia64_vph_page	*vph_next_free_page;
	struct ia64_vph_table	*vph_next_free_table;
	struct ia64_vph_page	*vph_table0[IA64_LEVEL0];
	struct ia64_vph_page	*vph_table0_kernel[IA64_LEVEL0];
};


/*  cpu_ia64.c:  */
void ia64_update_translation_table(struct cpu *cpu, uint64_t vaddr_page,
	unsigned char *host_page, int writeflag, uint64_t paddr_page);
void ia64_invalidate_translation_caches_paddr(struct cpu *cpu, uint64_t paddr);
void ia64_invalidate_code_translation_caches(struct cpu *cpu);
int ia64_memory_rw(struct cpu *cpu, struct memory *mem, uint64_t vaddr,
	unsigned char *data, size_t len, int writeflag, int cache_flags);
int ia64_userland_memory_rw(struct cpu *cpu, struct memory *mem,
	uint64_t vaddr, unsigned char *data, size_t len, int writeflag,
	int cache_flags);
int ia64_cpu_family_init(struct cpu_family *);


#endif	/*  CPU_IA64_H  */