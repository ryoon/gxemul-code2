/*
 *  Copyright (C) 2007  Anders Gavare.  All rights reserved.
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
 *  $Id: cpu_m88k_instr_loadstore.c,v 1.1 2007-05-11 09:28:35 debug Exp $
 *
 *  M88K load/store instructions; the following args are used:
 *  
 *  arg[0] = pointer to the register to load to or store from (d)
 *  arg[1] = pointer to the base register (s1)
 *  arg[2] = pointer to the offset register (s2), or an uint32_t offset
 *
 *  The GENERIC function always checks for alignment, and supports both big
 *  and little endian byte order.
 *
 *  The quick function is included twice (big/little endian) for each
 *  GENERIC function.
 *
 *
 *  Defines:
 *	LS_LOAD or LS_STORE (only one)
 *	LS_INCLUDE_GENERIC (to generate the generic function)
 *	LS_GENERIC_N is defined as the name of the generic function
 *	LS_N is defined as the name of the fast function
 *	LS_1, LS_2, LS_4, or LS_8 (only one)
 *	LS_SIZE is defined to 1, 2, 4, or 8
 *	LS_SIGNED is defined for signed loads
 *	LS_LE or LS_BE (only one)
 *	LS_SCALED for scaled accesses
 *	LS_USR for usr accesses
 *	LS_REGOFS is defined when arg[2] is a register pointer
 *
 *
 *  TODO:
 *	USR bit
 *	32-bit aligned 64-bit loads/stores (because these are allowed
 *		by the m88k architecture!)
 */


#ifdef LS_INCLUDE_GENERIC
void LS_GENERIC_N(struct cpu *cpu, struct m88k_instr_call *ic)
{
	uint32_t addr = reg(ic->arg[1]) +
#ifdef LS_REGOFS
#ifdef LS_SCALED
	    LS_SIZE *
#endif
	    reg(ic->arg[2]);
#else
	    ic->arg[2];
#endif
	uint8_t data[LS_SIZE];
#ifdef LS_LOAD
	uint64_t x;
#endif

	/*  Synchronize the PC:  */
	int low_pc = ((size_t)ic - (size_t)cpu->cd.m88k.cur_ic_page)
	    / sizeof(struct m88k_instr_call);
	cpu->pc &= ~((M88K_IC_ENTRIES_PER_PAGE-1)<<M88K_INSTR_ALIGNMENT_SHIFT);
	cpu->pc += (low_pc << M88K_INSTR_ALIGNMENT_SHIFT);

#ifndef LS_1
	/*  Check alignment:  */
	if (addr & (LS_SIZE - 1)) {
#if 0
		/*  Cause an address alignment exception:  */
		m88k_cpu_exception(cpu, ....
#else
		fatal("{ m88k dyntrans alignment exception, size = %i,"
		    " addr = %016"PRIx64", pc = %016"PRIx64" }\n", LS_SIZE,
		    (uint64_t) addr, cpu->pc);

		/*  TODO: Generalize this into a abort_call, or similar:  */
		cpu->running = 0;
		debugger_n_steps_left_before_interaction = 0;
		cpu->cd.m88k.next_ic = &nothing_call;
#endif
		return;
	}
#endif

#ifdef LS_LOAD
	if (!cpu->memory_rw(cpu, cpu->mem, addr, data, sizeof(data),
	    MEM_READ, CACHE_DATA)) {
		/*  Exception.  */
		return;
	}
	x = memory_readmax64(cpu, data, LS_SIZE);
#ifdef LS_SIGNED
#ifdef LS_1
	x = (int8_t)x;
#endif
#ifdef LS_2
	x = (int16_t)x;
#endif
#ifdef LS_4
	x = (int32_t)x;
#endif
#endif
	reg(ic->arg[0]) = x;
#else	/*  LS_STORE:  */
	memory_writemax64(cpu, data, LS_SIZE, reg(ic->arg[0]));
	if (!cpu->memory_rw(cpu, cpu->mem, addr, data, sizeof(data),
	    MEM_WRITE, CACHE_DATA)) {
		/*  Exception.  */
		return;
	}
#endif
}
#endif	/*  LS_INCLUDE_GENERIC  */


void LS_N(struct cpu *cpu, struct m88k_instr_call *ic)
{
	uint32_t addr = reg(ic->arg[1]) +
#ifdef LS_REGOFS
#ifdef LS_SCALED
	    LS_SIZE *
#endif
	    reg(ic->arg[2]);
#else
	    ic->arg[2];
#endif
	unsigned char *p;
#ifdef LS_LOAD
	p = cpu->cd.m88k.host_load[addr >> 12];
#else
	p = cpu->cd.m88k.host_store[addr >> 12];
#endif

#ifdef LS_USR
fatal("USR bit: TODO\n");
exit(1);
#endif

	if (p == NULL
#ifndef LS_1
	    || addr & (LS_SIZE - 1)
#endif
	    ) {
		LS_GENERIC_N(cpu, ic);
		return;
	}

	addr &= 0xfff;

#ifdef LS_LOAD
	/*  Load:  */

#ifdef LS_1
	reg(ic->arg[0]) =
#ifdef LS_SIGNED
	    (int8_t)
#endif
	    p[addr];
#endif	/*  LS_1  */

#ifdef LS_2
	reg(ic->arg[0]) =
#ifdef LS_SIGNED
	    (int16_t)
#endif
#ifdef LS_BE
#ifdef HOST_BIG_ENDIAN
	    ( *(uint16_t *)(p + addr) );
#else
	    ((p[addr]<<8) + p[addr+1]);
#endif
#else
#ifdef HOST_LITTLE_ENDIAN
	    ( *(uint16_t *)(p + addr) );
#else
	    (p[addr] + (p[addr+1]<<8));
#endif
#endif
#endif	/*  LS_2  */

#ifdef LS_4
	reg(ic->arg[0]) =
#ifdef LS_SIGNED
	    (int32_t)
#else
	    (uint32_t)
#endif
#ifdef LS_BE
#ifdef HOST_BIG_ENDIAN
	    ( *(uint32_t *)(p + addr) );
#else
	    ((p[addr]<<24) + (p[addr+1]<<16) + (p[addr+2]<<8) + p[addr+3]);
#endif
#else
#ifdef HOST_LITTLE_ENDIAN
	    ( *(uint32_t *)(p + addr) );
#else
	    (p[addr] + (p[addr+1]<<8) + (p[addr+2]<<16) + (p[addr+3]<<24));
#endif
#endif
#endif	/*  LS_4  */

#ifdef LS_8
	*((uint64_t *)ic->arg[0]) =
#ifdef LS_BE
#ifdef HOST_BIG_ENDIAN
	    ( *(uint64_t *)(p + addr) );
#else
	    ((uint64_t)p[addr] << 56) + ((uint64_t)p[addr+1] << 48) +
	    ((uint64_t)p[addr+2] << 40) + ((uint64_t)p[addr+3] << 32) +
	    ((uint64_t)p[addr+4] << 24) +
	    (p[addr+5] << 16) + (p[addr+6] << 8) + p[addr+7];
#endif
#else
#ifdef HOST_LITTLE_ENDIAN
	    ( *(uint64_t *)(p + addr) );
#else
	    p[addr+0] + (p[addr+1] << 8) + (p[addr+2] << 16) +
	    ((uint64_t)p[addr+3] << 24) + ((uint64_t)p[addr+4] << 32) +
	    ((uint64_t)p[addr+5] << 40) + ((uint64_t)p[addr+6] << 48) +
	    ((uint64_t)p[addr+7] << 56);
#endif
#endif
#endif	/*  LS_8  */

#else
	/*  Store: */

#ifdef LS_1
	p[addr] = reg(ic->arg[0]);
#endif
#ifdef LS_2
	{ uint32_t x = reg(ic->arg[0]);
#ifdef LS_BE
#ifdef HOST_BIG_ENDIAN
	*((uint16_t *)(p+addr)) = x; }
#else
	p[addr] = x >> 8; p[addr+1] = x; }
#endif
#else
#ifdef HOST_LITTLE_ENDIAN
	*((uint16_t *)(p+addr)) = x; }
#else
	p[addr] = x; p[addr+1] = x >> 8; }
#endif
#endif
#endif  /*  LS_2  */
#ifdef LS_4
	{ uint32_t x = reg(ic->arg[0]);
#ifdef LS_BE
#ifdef HOST_BIG_ENDIAN
	*((uint32_t *)(p+addr)) = x; }
#else
	p[addr] = x >> 24; p[addr+1] = x >> 16; 
	p[addr+2] = x >> 8; p[addr+3] = x; }
#endif
#else
#ifdef HOST_LITTLE_ENDIAN
	*((uint32_t *)(p+addr)) = x; }
#else
	p[addr] = x; p[addr+1] = x >> 8; 
	p[addr+2] = x >> 16; p[addr+3] = x >> 24; }
#endif
#endif
#endif  /*  LS_4  */
#ifdef LS_8
	{ uint64_t x = *(uint64_t *)(ic->arg[0]);
#ifdef LS_BE
#ifdef HOST_BIG_ENDIAN
	*((uint64_t *)(p+addr)) = x; }
#else
	p[addr]   = x >> 56; p[addr+1] = x >> 48; p[addr+2] = x >> 40;
	p[addr+3] = x >> 32; p[addr+4] = x >> 24; p[addr+5] = x >> 16;
	p[addr+6] = x >> 8;  p[addr+7] = x; }
#endif
#else
#ifdef HOST_LITTLE_ENDIAN
	*((uint64_t *)(p+addr)) = x; }
#else
	p[addr]   = x;       p[addr+1] = x >>  8; p[addr+2] = x >> 16;
	p[addr+3] = x >> 24; p[addr+4] = x >> 32; p[addr+5] = x >> 40;
	p[addr+6] = x >> 48; p[addr+7] = x >> 56; }
#endif
#endif
#endif	/*  LS_8  */

#endif	/*  store  */
}

