#ifndef	CPU_M68K_H
#define	CPU_M68K_H

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
 *  $Id: cpu_m68k.h,v 1.1 2005-08-11 09:14:12 debug Exp $
 */

#include "misc.h"


struct cpu_family;

#define	N_M68K_DREGS		8
#define	N_M68K_AREGS		8

#define	M68K_N_IC_ARGS			3
#define	M68K_IC_ENTRIES_SHIFT		11
#define	M68K_IC_ENTRIES_PER_PAGE	(1 << M68K_IC_ENTRIES_SHIFT)
#define	M68K_PC_TO_IC_ENTRY(a)		(((a)>>1)&(M68K_IC_ENTRIES_PER_PAGE-1))
#define	M68K_ADDR_TO_PAGENR(a)		((a) >> (M68K_IC_ENTRIES_SHIFT+2))

struct m68k_instr_call {
	void	(*f)(struct cpu *, struct m68k_instr_call *);
	size_t	arg[M68K_N_IC_ARGS];
};

/*  Translation cache struct for each physical page:  */
struct m68k_tc_physpage {
	uint32_t	next_ofs;	/*  or 0 for end of chain  */
	uint32_t	physaddr;
	int		flags;
	struct m68k_instr_call ics[M68K_IC_ENTRIES_PER_PAGE + 1];
};


#define	M68K_N_VPH_ENTRIES	1048576

#define	M68K_MAX_VPH_TLB_ENTRIES		256
struct m68k_vpg_tlb_entry {
	int		valid;
	int		writeflag;
	int64_t		timestamp;
	unsigned char	*host_page;
	uint32_t	vaddr_page;
	uint32_t	paddr_page;
};


struct m68k_cpu {
	/*
	 *  General Purpose Registers:
	 */

	uint32_t		d[N_M68K_DREGS];
	uint32_t		a[N_M68K_AREGS];


	/*
	 *  Instruction translation cache:
	 */

	/*  cur_ic_page is a pointer to an array of M68K_IC_ENTRIES_PER_PAGE
	    instruction call entries. next_ic points to the next such
	    call to be executed.  */
	struct m68k_tc_physpage	*cur_physpage;
	struct m68k_instr_call	*cur_ic_page;
	struct m68k_instr_call	*next_ic;


	/*
	 *  Virtual -> physical -> host address translation:
	 *
	 *  host_load and host_store point to arrays of M68K_N_VPH_ENTRIES
	 *  pointers (to host pages); phys_addr points to an array of
	 *  M68K_N_VPH_ENTRIES uint32_t.
	 */

	struct m68k_vpg_tlb_entry	vph_tlb_entry[M68K_MAX_VPH_TLB_ENTRIES];
	unsigned char			*host_load[M68K_N_VPH_ENTRIES];
	unsigned char			*host_store[M68K_N_VPH_ENTRIES];
	uint32_t			phys_addr[M68K_N_VPH_ENTRIES];
	struct m68k_tc_physpage		*phys_page[M68K_N_VPH_ENTRIES];
};


/*  cpu_m68k.c:  */
void m68k_update_translation_table(struct cpu *cpu, uint64_t vaddr_page,
	unsigned char *host_page, int writeflag, uint64_t paddr_page);
void m68k_invalidate_translation_caches_paddr(struct cpu *cpu, uint64_t paddr);
void m68k_invalidate_code_translation_caches(struct cpu *cpu);
int m68k_memory_rw(struct cpu *cpu, struct memory *mem, uint64_t vaddr,
	unsigned char *data, size_t len, int writeflag, int cache_flags);
int m68k_cpu_family_init(struct cpu_family *);


#endif	/*  CPU_M68K_H  */