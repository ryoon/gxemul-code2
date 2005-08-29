#ifndef	CPU_SPARC_H
#define	CPU_SPARC_H

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
 *  $Id: cpu_sparc.h,v 1.9 2005-08-28 20:16:24 debug Exp $
 */

#include "misc.h"


struct cpu_family;


#define	SPARC_N_IC_ARGS			3
#define	SPARC_INSTR_ALIGNMENT_SHIFT	2
#define	SPARC_IC_ENTRIES_SHIFT		10
#define	SPARC_IC_ENTRIES_PER_PAGE	(1 << SPARC_IC_ENTRIES_SHIFT)
#define	SPARC_PC_TO_IC_ENTRY(a)		(((a)>>SPARC_INSTR_ALIGNMENT_SHIFT) \
					& (SPARC_IC_ENTRIES_PER_PAGE-1))
#define	SPARC_ADDR_TO_PAGENR(a)		((a) >> (SPARC_IC_ENTRIES_SHIFT \
					+ SPARC_INSTR_ALIGNMENT_SHIFT))

struct sparc_instr_call {
	void	(*f)(struct cpu *, struct sparc_instr_call *);
	size_t	arg[SPARC_N_IC_ARGS];
};

/*  Translation cache struct for each physical page:  */
struct sparc_tc_physpage {
	uint32_t	next_ofs;	/*  or 0 for end of chain  */
	uint64_t	physaddr;
	int		flags;
	struct sparc_instr_call ics[SPARC_IC_ENTRIES_PER_PAGE + 1];
};

#define	SPARC_N_VPH_ENTRIES		1048576

#define	SPARC_MAX_VPH_TLB_ENTRIES		256
struct sparc_vpg_tlb_entry {
	int		valid;
	int		writeflag;
	int64_t		timestamp;
	unsigned char	*host_page;
	uint64_t	vaddr_page;
	uint64_t	paddr_page;
};

struct sparc_cpu {
	/*  TODO  */
	uint64_t	r_i[8];


	/*
	 *  Instruction translation cache:
	 */

	/*  cur_ic_page is a pointer to an array of SPARC_IC_ENTRIES_PER_PAGE
	    instruction call entries. next_ic points to the next such
	    call to be executed.  */
	struct sparc_tc_physpage	*cur_physpage;
	struct sparc_instr_call	*cur_ic_page;
	struct sparc_instr_call	*next_ic;


	/*
	 *  Virtual -> physical -> host address translation:
	 *
	 *  host_load and host_store point to arrays of SPARC_N_VPH_ENTRIES
	 *  pointers (to host pages); phys_addr points to an array of
	 *  SPARC_N_VPH_ENTRIES uint32_t.
	 */

	struct sparc_vpg_tlb_entry  vph_tlb_entry[SPARC_MAX_VPH_TLB_ENTRIES];
	unsigned char		    *host_load[SPARC_N_VPH_ENTRIES]; 
	unsigned char		    *host_store[SPARC_N_VPH_ENTRIES];
	uint32_t		    phys_addr[SPARC_N_VPH_ENTRIES]; 
	struct sparc_tc_physpage    *phys_page[SPARC_N_VPH_ENTRIES];
};


/*  cpu_sparc.c:  */
void sparc_update_translation_table(struct cpu *cpu, uint64_t vaddr_page,
	unsigned char *host_page, int writeflag, uint64_t paddr_page);
void sparc_invalidate_translation_caches_paddr(struct cpu *cpu, uint64_t, int);
void sparc_invalidate_code_translation(struct cpu *cpu, uint64_t, int);
int sparc_memory_rw(struct cpu *cpu, struct memory *mem, uint64_t vaddr,
	unsigned char *data, size_t len, int writeflag, int cache_flags);
int sparc_cpu_family_init(struct cpu_family *);


#endif	/*  CPU_SPARC_H  */