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
 *  $Id: cpu_ppc_instr.c,v 1.1 2005-08-29 14:36:41 debug Exp $
 *
 *  POWER/PowerPC instructions.
 *
 *  Individual functions should keep track of cpu->n_translated_instrs.
 *  (If no instruction was executed, then it should be decreased. If, say, 4
 *  instructions were combined into one function and executed, then it should
 *  be increased by 3.)
 */


/*
 *  nop:  Do nothing.
 */
X(nop)
{
}


/*
 *  invalid:  To catch bugs.
 */
X(invalid)
{
	fatal("INTERNAL ERROR\n");
	exit(1);
}


/*
 *  addi:  Add immediate.
 *
 *  arg[0] = pointer to source uint64_t
 *  arg[1] = immediate value (int32_t or larger)
 *  arg[2] = pointer to destination uint64_t
 */
X(addi)
{
	reg(ic->arg[2]) = reg(ic->arg[0]) + (int32_t)ic->arg[1];
}


/*
 *  andi_dot:  AND immediate, update CR.
 *
 *  arg[0] = pointer to source uint64_t
 *  arg[1] = immediate value (uint32_t)
 *  arg[2] = pointer to destination uint64_t
 */
X(andi_dot)
{
	MODE_uint_t tmp = reg(ic->arg[0]) & (uint32_t)ic->arg[1];
	reg(ic->arg[2]) = tmp;
	update_cr0(cpu, tmp);
}


/*
 *  addic:  Add immediate, Carry.
 *
 *  arg[0] = pointer to source uint64_t
 *  arg[1] = immediate value (int32_t or larger)
 *  arg[2] = pointer to destination uint64_t
 */
X(addic)
{
	/*  TODO/NOTE: Only for 32-bit mode, so far!  */
	uint64_t tmp = (int32_t)reg(ic->arg[0]);
	uint64_t tmp2 = tmp;

	tmp2 += (int32_t)ic->arg[1];

	/*  NOTE: CA is never cleared, just set.  */
	/*  TODO: Is this correct?  */
	if ((tmp2 >> 32) != (tmp >> 32))
		cpu->cd.ppc.xer |= PPC_XER_CA;

	reg(ic->arg[2]) = (uint32_t)tmp2;
}


/*
 *  subfic:  Subtract from immediate, Carry.
 *
 *  arg[0] = pointer to source uint64_t
 *  arg[1] = immediate value (int32_t or larger)
 *  arg[2] = pointer to destination uint64_t
 */
X(subfic)
{
	/*  TODO/NOTE: Only for 32-bit mode, so far!  */
	uint64_t tmp = (int32_t)(~reg(ic->arg[0]));
	uint64_t tmp2 = tmp;

	tmp2 += (int32_t)ic->arg[1] + 1;

	/*  NOTE: CA is never cleared, just set. TODO: Is this right?  */
	/*  TODO: Is this correct?  */
	if ((tmp2 >> 32) != (tmp >> 32))
		cpu->cd.ppc.xer |= PPC_XER_CA;

	reg(ic->arg[2]) = (uint32_t)tmp2;
}


/*
 *  addic_dot:  Add immediate, Carry.
 *
 *  arg[0] = pointer to source uint64_t
 *  arg[1] = immediate value (int32_t or larger)
 *  arg[2] = pointer to destination uint64_t
 */
X(addic_dot)
{
	/*  TODO/NOTE: Only for 32-bit mode, so far!  */
	uint64_t tmp = (uint32_t)reg(ic->arg[0]);
	uint64_t tmp2 = tmp;

	tmp2 += (int32_t)ic->arg[1];

	/*  NOTE: CA is never cleared, just set.  */
	/*  TODO: Is this correct?  */
	if ((tmp2 >> 32) != (tmp >> 32))
		cpu->cd.ppc.xer |= PPC_XER_CA;

	reg(ic->arg[2]) = (uint32_t)tmp2;
	update_cr0(cpu, tmp2);
}


/*
 *  bclr:  Branch Conditional to Link Register
 *
 *  arg[0] = bo
 *  arg[1] = bi
 *  arg[2] = bh
 */
X(bclr)
{
	int bo = ic->arg[0], bi = ic->arg[1]  /* , bh = ic->arg[2]  */;
	int ctr_ok, cond_ok;
	uint64_t old_pc = cpu->pc;
	MODE_uint_t tmp, addr = cpu->cd.ppc.lr;
	if (!(bo & 4))
		cpu->cd.ppc.ctr --;
	ctr_ok = (bo >> 2) & 1;
	tmp = cpu->cd.ppc.ctr;
	ctr_ok |= ( (tmp != 0) ^ ((bo >> 1) & 1) );
	cond_ok = (bo >> 4) & 1;
	cond_ok |= ( ((bo >> 3) & 1) == ((cpu->cd.ppc.cr >> (31-bi)) & 1) );
	if (ctr_ok && cond_ok) {
		uint64_t mask_within_page =
		    ((PPC_IC_ENTRIES_PER_PAGE-1) << PPC_INSTR_ALIGNMENT_SHIFT)
		    | ((1 << PPC_INSTR_ALIGNMENT_SHIFT) - 1);
		cpu->pc = addr & ~((1 << PPC_INSTR_ALIGNMENT_SHIFT) - 1);
		/*  TODO: trace in separate (duplicate) function?  */
		if (cpu->machine->show_trace_tree)
			cpu_functioncall_trace_return(cpu);
		if ((old_pc  & ~mask_within_page) ==
		    (cpu->pc & ~mask_within_page)) {
			cpu->cd.ppc.next_ic =
			    cpu->cd.ppc.cur_ic_page +
			    ((cpu->pc & mask_within_page) >>
			    PPC_INSTR_ALIGNMENT_SHIFT);
		} else {
			/*  Find the new physical page and update pointers:  */
			DYNTRANS_PC_TO_POINTERS(cpu);
		}
	}
}
X(bclr_l)
{
	uint64_t low_pc, old_pc = cpu->pc;
	int bo = ic->arg[0], bi = ic->arg[1]  /* , bh = ic->arg[2]  */;
	int ctr_ok, cond_ok;
	MODE_uint_t tmp, addr = cpu->cd.ppc.lr;
	if (!(bo & 4))
		cpu->cd.ppc.ctr --;
	ctr_ok = (bo >> 2) & 1;
	tmp = cpu->cd.ppc.ctr;
	ctr_ok |= ( (tmp != 0) ^ ((bo >> 1) & 1) );
	cond_ok = (bo >> 4) & 1;
	cond_ok |= ( ((bo >> 3) & 1) == ((cpu->cd.ppc.cr >> (31-bi)) & 1) );

	/*  Calculate return PC:  */
	low_pc = ((size_t)cpu->cd.ppc.next_ic - (size_t)
	    cpu->cd.ppc.cur_ic_page) / sizeof(struct ppc_instr_call);
	cpu->cd.ppc.lr = cpu->pc & ~((PPC_IC_ENTRIES_PER_PAGE-1) << 2);
	cpu->cd.ppc.lr += (low_pc << 2);

	if (ctr_ok && cond_ok) {
		uint64_t mask_within_page =
		    ((PPC_IC_ENTRIES_PER_PAGE-1) << PPC_INSTR_ALIGNMENT_SHIFT)
		    | ((1 << PPC_INSTR_ALIGNMENT_SHIFT) - 1);
		cpu->pc = addr & ~((1 << PPC_INSTR_ALIGNMENT_SHIFT) - 1);
		/*  TODO: trace in separate (duplicate) function?  */
		if (cpu->machine->show_trace_tree)
			cpu_functioncall_trace_return(cpu);
		if (cpu->machine->show_trace_tree)
			cpu_functioncall_trace(cpu, cpu->pc);
		if ((old_pc  & ~mask_within_page) ==
		    (cpu->pc & ~mask_within_page)) {
			cpu->cd.ppc.next_ic =
			    cpu->cd.ppc.cur_ic_page +
			    ((cpu->pc & mask_within_page) >>
			    PPC_INSTR_ALIGNMENT_SHIFT);
		} else {
			/*  Find the new physical page and update pointers:  */
			DYNTRANS_PC_TO_POINTERS(cpu);
		}
	}
}


/*
 *  bcctr:  Branch Conditional to Count register
 *
 *  arg[0] = bo
 *  arg[1] = bi
 *  arg[2] = bh
 */
X(bcctr)
{
	int bo = ic->arg[0], bi = ic->arg[1]  /* , bh = ic->arg[2]  */;
	uint64_t old_pc = cpu->pc;
	MODE_uint_t addr = cpu->cd.ppc.ctr;
	int cond_ok = (bo >> 4) & 1;
	cond_ok |= ( ((bo >> 3) & 1) == ((cpu->cd.ppc.cr >> (31-bi)) & 1) );
	if (cond_ok) {
		uint64_t mask_within_page =
		    ((PPC_IC_ENTRIES_PER_PAGE-1) << PPC_INSTR_ALIGNMENT_SHIFT)
		    | ((1 << PPC_INSTR_ALIGNMENT_SHIFT) - 1);
		cpu->pc = addr & ~((1 << PPC_INSTR_ALIGNMENT_SHIFT) - 1);
		/*  TODO: trace in separate (duplicate) function?  */
		if (cpu->machine->show_trace_tree)
			cpu_functioncall_trace_return(cpu);
		if ((old_pc  & ~mask_within_page) ==
		    (cpu->pc & ~mask_within_page)) {
			cpu->cd.ppc.next_ic =
			    cpu->cd.ppc.cur_ic_page +
			    ((cpu->pc & mask_within_page) >>
			    PPC_INSTR_ALIGNMENT_SHIFT);
		} else {
			/*  Find the new physical page and update pointers:  */
			DYNTRANS_PC_TO_POINTERS(cpu);
		}
	}
}
X(bcctr_l)
{
	uint64_t low_pc, old_pc = cpu->pc;
	int bo = ic->arg[0], bi = ic->arg[1]  /* , bh = ic->arg[2]  */;
	MODE_uint_t addr = cpu->cd.ppc.ctr;
	int cond_ok = (bo >> 4) & 1;
	cond_ok |= ( ((bo >> 3) & 1) == ((cpu->cd.ppc.cr >> (31-bi)) & 1) );

	/*  Calculate return PC:  */
	low_pc = ((size_t)cpu->cd.ppc.next_ic - (size_t)
	    cpu->cd.ppc.cur_ic_page) / sizeof(struct ppc_instr_call);
	cpu->cd.ppc.lr = cpu->pc & ~((PPC_IC_ENTRIES_PER_PAGE-1) << 2);
	cpu->cd.ppc.lr += (low_pc << 2);

	if (cond_ok) {
		uint64_t mask_within_page =
		    ((PPC_IC_ENTRIES_PER_PAGE-1) << PPC_INSTR_ALIGNMENT_SHIFT)
		    | ((1 << PPC_INSTR_ALIGNMENT_SHIFT) - 1);
		cpu->pc = addr & ~((1 << PPC_INSTR_ALIGNMENT_SHIFT) - 1);
		/*  TODO: trace in separate (duplicate) function?  */
		if (cpu->machine->show_trace_tree)
			cpu_functioncall_trace(cpu, cpu->pc);
		if ((old_pc  & ~mask_within_page) ==
		    (cpu->pc & ~mask_within_page)) {
			cpu->cd.ppc.next_ic =
			    cpu->cd.ppc.cur_ic_page +
			    ((cpu->pc & mask_within_page) >>
			    PPC_INSTR_ALIGNMENT_SHIFT);
		} else {
			/*  Find the new physical page and update pointers:  */
			DYNTRANS_PC_TO_POINTERS(cpu);
		}
	}
}


/*
 *  b:  Branch (to a different translated page)
 *
 *  arg[0] = relative offset (as an int32_t)
 */
X(b)
{
	uint64_t low_pc;

	/*  Calculate new PC from this instruction + arg[0]  */
	low_pc = ((size_t)ic - (size_t)
	    cpu->cd.ppc.cur_ic_page) / sizeof(struct ppc_instr_call);
	cpu->pc &= ~((PPC_IC_ENTRIES_PER_PAGE-1) << 2);
	cpu->pc += (low_pc << 2);
	cpu->pc += (int32_t)ic->arg[0];

	/*  Find the new physical page and update the translation pointers:  */
	DYNTRANS_PC_TO_POINTERS(cpu);
}


/*
 *  bc:  Branch Conditional (to a different translated page)
 *
 *  arg[0] = relative offset (as an int32_t)
 *  arg[1] = bo
 *  arg[2] = bi
 */
X(bc)
{
	MODE_uint_t tmp;
	int ctr_ok, cond_ok, bi = ic->arg[2], bo = ic->arg[1];
	if (!(bo & 4))
		cpu->cd.ppc.ctr --;
	ctr_ok = (bo >> 2) & 1;
	tmp = cpu->cd.ppc.ctr;
	ctr_ok |= ( (tmp != 0) ^ ((bo >> 1) & 1) );
	cond_ok = (bo >> 4) & 1;
	cond_ok |= ( ((bo >> 3) & 1) ==
	    ((cpu->cd.ppc.cr >> (31-bi)) & 1)  );
	if (ctr_ok && cond_ok)
		instr(b)(cpu,ic);
}


/*
 *  b_samepage:  Branch (to within the same translated page)
 *
 *  arg[0] = pointer to new ppc_instr_call
 */
X(b_samepage)
{
	cpu->cd.ppc.next_ic = (struct ppc_instr_call *) ic->arg[0];
}


/*
 *  bc_samepage:  Branch Conditional (to within the same page)
 *
 *  arg[0] = new ic ptr
 *  arg[1] = bo
 *  arg[2] = bi
 */
X(bc_samepage)
{
	MODE_uint_t tmp;
	int ctr_ok, cond_ok, bi = ic->arg[2], bo = ic->arg[1];
	if (!(bo & 4))
		cpu->cd.ppc.ctr --;
	ctr_ok = (bo >> 2) & 1;
	tmp = cpu->cd.ppc.ctr;
	ctr_ok |= ( (tmp != 0) ^ ((bo >> 1) & 1) );
	cond_ok = (bo >> 4) & 1;
	cond_ok |= ( ((bo >> 3) & 1) ==
	    ((cpu->cd.ppc.cr >> (31-bi)) & 1)  );
	if (ctr_ok && cond_ok)
		instr(b_samepage)(cpu,ic);
}


/*
 *  bl:  Branch and Link (to a different translated page)
 *
 *  arg[0] = relative offset (as an int32_t)
 */
X(bl)
{
	uint32_t low_pc;

	/*  Calculate LR:  */
	low_pc = ((size_t)cpu->cd.ppc.next_ic - (size_t)
	    cpu->cd.ppc.cur_ic_page) / sizeof(struct ppc_instr_call);
	cpu->cd.ppc.lr = cpu->pc & ~((PPC_IC_ENTRIES_PER_PAGE-1) << 2);
	cpu->cd.ppc.lr += (low_pc << 2);

	/*  Calculate new PC from this instruction + arg[0]  */
	low_pc = ((size_t)ic - (size_t)
	    cpu->cd.ppc.cur_ic_page) / sizeof(struct ppc_instr_call);
	cpu->pc &= ~((PPC_IC_ENTRIES_PER_PAGE-1) << 2);
	cpu->pc += (low_pc << 2);
	cpu->pc += (int32_t)ic->arg[0];

	/*  Find the new physical page and update the translation pointers:  */
	DYNTRANS_PC_TO_POINTERS(cpu);
}


/*
 *  bl_trace:  Branch and Link (to a different translated page)  (with trace)
 *
 *  arg[0] = relative offset (as an int32_t)
 */
X(bl_trace)
{
	uint32_t low_pc;

	/*  Calculate LR:  */
	low_pc = ((size_t)cpu->cd.ppc.next_ic - (size_t)
	    cpu->cd.ppc.cur_ic_page) / sizeof(struct ppc_instr_call);
	cpu->cd.ppc.lr = cpu->pc & ~((PPC_IC_ENTRIES_PER_PAGE-1) << 2);
	cpu->cd.ppc.lr += (low_pc << 2);

	/*  Calculate new PC from this instruction + arg[0]  */
	low_pc = ((size_t)ic - (size_t)
	    cpu->cd.ppc.cur_ic_page) / sizeof(struct ppc_instr_call);
	cpu->pc &= ~((PPC_IC_ENTRIES_PER_PAGE-1) << 2);
	cpu->pc += (low_pc << 2);
	cpu->pc += (int32_t)ic->arg[0];

	cpu_functioncall_trace(cpu, cpu->pc);

	/*  Find the new physical page and update the translation pointers:  */
	DYNTRANS_PC_TO_POINTERS(cpu);
}


/*
 *  bl_samepage:  Branch and Link (to within the same translated page)
 *
 *  arg[0] = pointer to new ppc_instr_call
 */
X(bl_samepage)
{
	uint32_t low_pc;

	/*  Calculate LR:  */
	low_pc = ((size_t)cpu->cd.ppc.next_ic - (size_t)
	    cpu->cd.ppc.cur_ic_page) / sizeof(struct ppc_instr_call);
	cpu->cd.ppc.lr = cpu->pc & ~((PPC_IC_ENTRIES_PER_PAGE-1) << 2);
	cpu->cd.ppc.lr += (low_pc << 2);

	cpu->cd.ppc.next_ic = (struct ppc_instr_call *) ic->arg[0];
}


/*
 *  bl_samepage_trace:  Branch and Link (to within the same translated page)
 *
 *  arg[0] = pointer to new ppc_instr_call
 */
X(bl_samepage_trace)
{
	uint32_t low_pc;

	/*  Calculate LR:  */
	low_pc = ((size_t)cpu->cd.ppc.next_ic - (size_t)
	    cpu->cd.ppc.cur_ic_page) / sizeof(struct ppc_instr_call);
	cpu->cd.ppc.lr = cpu->pc & ~((PPC_IC_ENTRIES_PER_PAGE-1) << 2);
	cpu->cd.ppc.lr += (low_pc << 2);

	cpu->cd.ppc.next_ic = (struct ppc_instr_call *) ic->arg[0];

	/*  Calculate new PC (for the trace)  */
	low_pc = ((size_t)cpu->cd.ppc.next_ic - (size_t)
	    cpu->cd.ppc.cur_ic_page) / sizeof(struct ppc_instr_call);
	cpu->pc &= ~((PPC_IC_ENTRIES_PER_PAGE-1) << 2);
	cpu->pc += (low_pc << 2);
	cpu_functioncall_trace(cpu, cpu->pc);
}


/*
 *  cntlzw:  Count leading zeroes.
 *
 *  arg[0] = ptr to rs
 *  arg[1] = ptr to ra
 */
X(cntlzw)
{
	uint32_t tmp = reg(ic->arg[0]);
	int n = 0, i;
	for (i=0; i<32; i++) {
		if (!(tmp & 0x80000000))
			n++;
		else
			break;
		tmp <<= 1;
	}
	reg(ic->arg[1]) = n;
}


/*
 *  cmpd:  Compare Doubleword
 *
 *  arg[0] = ptr to ra
 *  arg[1] = ptr to rb
 *  arg[2] = bf
 */
X(cmpd)
{
	int64_t tmp = reg(ic->arg[0]), tmp2 = reg(ic->arg[1]);
	int bf = ic->arg[2], c;
	if (tmp < tmp2)
		c = 8;
	else if (tmp > tmp2)
		c = 4;
	else
		c = 2;
	c |= ((cpu->cd.ppc.xer >> 31) & 1);  /*  SO bit, copied from XER  */
	cpu->cd.ppc.cr &= ~(0xf << (28 - 4*bf));
	cpu->cd.ppc.cr |= (c << (28 - 4*bf));
}


/*
 *  cmpld:  Compare Doubleword, unsigned
 *
 *  arg[0] = ptr to ra
 *  arg[1] = ptr to rb
 *  arg[2] = bf
 */
X(cmpld)
{
	uint64_t tmp = reg(ic->arg[0]), tmp2 = reg(ic->arg[1]);
	int bf = ic->arg[2], c;
	if (tmp < tmp2)
		c = 8;
	else if (tmp > tmp2)
		c = 4;
	else
		c = 2;
	c |= ((cpu->cd.ppc.xer >> 31) & 1);  /*  SO bit, copied from XER  */
	cpu->cd.ppc.cr &= ~(0xf << (28 - 4*bf));
	cpu->cd.ppc.cr |= (c << (28 - 4*bf));
}


/*
 *  cmpdi:  Compare Doubleword immediate
 *
 *  arg[0] = ptr to ra
 *  arg[1] = int32_t imm
 *  arg[2] = bf
 */
X(cmpdi)
{
	int64_t tmp = reg(ic->arg[0]), imm = (int32_t)ic->arg[1];
	int bf = ic->arg[2], c;
	if (tmp < imm)
		c = 8;
	else if (tmp > imm)
		c = 4;
	else
		c = 2;
	c |= ((cpu->cd.ppc.xer >> 31) & 1);  /*  SO bit, copied from XER  */
	cpu->cd.ppc.cr &= ~(0xf << (28 - 4*bf));
	cpu->cd.ppc.cr |= (c << (28 - 4*bf));
}


/*
 *  cmpldi:  Compare Doubleword immediate, logical
 *
 *  arg[0] = ptr to ra
 *  arg[1] = int32_t imm
 *  arg[2] = bf
 */
X(cmpldi)
{
	uint64_t tmp = reg(ic->arg[0]), imm = (uint32_t)ic->arg[1];
	int bf = ic->arg[2], c;
	if (tmp < imm)
		c = 8;
	else if (tmp > imm)
		c = 4;
	else
		c = 2;
	c |= ((cpu->cd.ppc.xer >> 31) & 1);  /*  SO bit, copied from XER  */
	cpu->cd.ppc.cr &= ~(0xf << (28 - 4*bf));
	cpu->cd.ppc.cr |= (c << (28 - 4*bf));
}


/*
 *  cmpw:  Compare Word
 *
 *  arg[0] = ptr to ra
 *  arg[1] = ptr to rb
 *  arg[2] = bf
 */
X(cmpw)
{
	int32_t tmp = reg(ic->arg[0]), tmp2 = reg(ic->arg[1]);
	int bf = ic->arg[2], c;
	if (tmp < tmp2)
		c = 8;
	else if (tmp > tmp2)
		c = 4;
	else
		c = 2;
	c |= ((cpu->cd.ppc.xer >> 31) & 1);  /*  SO bit, copied from XER  */
	cpu->cd.ppc.cr &= ~(0xf << (28 - 4*bf));
	cpu->cd.ppc.cr |= (c << (28 - 4*bf));
}


/*
 *  cmplw:  Compare Word, unsigned
 *
 *  arg[0] = ptr to ra
 *  arg[1] = ptr to rb
 *  arg[2] = bf
 */
X(cmplw)
{
	uint32_t tmp = reg(ic->arg[0]), tmp2 = reg(ic->arg[1]);
	int bf = ic->arg[2], c;
	if (tmp < tmp2)
		c = 8;
	else if (tmp > tmp2)
		c = 4;
	else
		c = 2;
	c |= ((cpu->cd.ppc.xer >> 31) & 1);  /*  SO bit, copied from XER  */
	cpu->cd.ppc.cr &= ~(0xf << (28 - 4*bf));
	cpu->cd.ppc.cr |= (c << (28 - 4*bf));
}


/*
 *  cmpwi:  Compare Word immediate
 *
 *  arg[0] = ptr to ra
 *  arg[1] = int32_t imm
 *  arg[2] = bf
 */
X(cmpwi)
{
	int32_t tmp = reg(ic->arg[0]), imm = ic->arg[1];
	int bf = ic->arg[2], c;
	if (tmp < imm)
		c = 8;
	else if (tmp > imm)
		c = 4;
	else
		c = 2;
	c |= ((cpu->cd.ppc.xer >> 31) & 1);  /*  SO bit, copied from XER  */
	cpu->cd.ppc.cr &= ~(0xf << (28 - 4*bf));
	cpu->cd.ppc.cr |= (c << (28 - 4*bf));
}


/*
 *  cmplwi:  Compare Word immediate, logical
 *
 *  arg[0] = ptr to ra
 *  arg[1] = int32_t imm
 *  arg[2] = bf
 */
X(cmplwi)
{
	uint32_t tmp = reg(ic->arg[0]), imm = ic->arg[1];
	int bf = ic->arg[2], c;
	if (tmp < imm)
		c = 8;
	else if (tmp > imm)
		c = 4;
	else
		c = 2;
	c |= ((cpu->cd.ppc.xer >> 31) & 1);  /*  SO bit, copied from XER  */
	cpu->cd.ppc.cr &= ~(0xf << (28 - 4*bf));
	cpu->cd.ppc.cr |= (c << (28 - 4*bf));
}


/*
 *  mtsr:  Move To Segment Register
 *
 *  arg[0] = segment register nr (0..15)
 *  arg[1] = ptr to rt
 */
X(mtsr)
{
	/*  TODO: This only works for 32-bit mode  */
	cpu->cd.ppc.sr[ic->arg[0]] = reg(ic->arg[1]);
}


/*
 *  mfsrin, mtsrin:  Move From/To Segment Register Indirect
 *
 *  arg[0] = ptr to rb
 *  arg[1] = ptr to rt
 */
X(mfsrin)
{
	/*  TODO: This only works for 32-bit mode  */
	uint32_t sr_num = reg(ic->arg[0]) >> 28;
	reg(ic->arg[1]) = cpu->cd.ppc.sr[sr_num];
}
X(mtsrin)
{
	/*  TODO: This only works for 32-bit mode  */
	uint32_t sr_num = reg(ic->arg[0]) >> 28;
	cpu->cd.ppc.sr[sr_num] = reg(ic->arg[1]);
}


/*
 *  rldicr:
 *
 *  arg[0] = copy of the instruction word
 */
X(rldicr)
{
	int rs = (ic->arg[0] >> 21) & 31;
	int ra = (ic->arg[0] >> 16) & 31;
	int sh = ((ic->arg[0] >> 11) & 31) | ((ic->arg[0] & 2) << 4);
	int me = ((ic->arg[0] >> 6) & 31) | (ic->arg[0] & 0x20);
	int rc = ic->arg[0] & 1;
	uint64_t tmp = cpu->cd.ppc.gpr[rs];
	/*  TODO: Fix this, its performance is awful:  */
	while (sh-- != 0) {
		int b = (tmp >> 63) & 1;
		tmp = (tmp << 1) | b;
	}
	while (me++ < 63)
		tmp &= ~((uint64_t)1 << (63-me));
	cpu->cd.ppc.gpr[ra] = tmp;
	if (rc)
		update_cr0(cpu, tmp);
}


/*
 *  rlwinm:
 *
 *  arg[0] = ptr to rs
 *  arg[1] = ptr to ra
 *  arg[2] = copy of the instruction word
 */
X(rlwinm)
{
	MODE_uint_t tmp = reg(ic->arg[0]), ra = 0;
	uint32_t iword = ic->arg[2];
	int sh, mb, me, rc;

	sh = (iword >> 11) & 31;
	mb = (iword >> 6) & 31;
	me = (iword >> 1) & 31;   
	rc = iword & 1;

	/*  TODO: Fix this, its performance is awful:  */
	while (sh-- != 0) {
		int b = (tmp >> 31) & 1;
		tmp = (tmp << 1) | b;
	}
	for (;;) {
		uint64_t mask;
		mask = (uint64_t)1 << (31-mb);
		ra |= (tmp & mask);
		if (mb == me)
			break;
		mb ++;
		if (mb == 32)
			mb = 0;
	}
	reg(ic->arg[1]) = ra;
	if (rc)
		update_cr0(cpu, ra);
}


/*
 *  rlwimi:
 *
 *  arg[0] = ptr to rs
 *  arg[1] = ptr to ra
 *  arg[2] = copy of the instruction word
 */
X(rlwimi)
{
	MODE_uint_t tmp = reg(ic->arg[0]), ra = reg(ic->arg[1]);
	uint32_t iword = ic->arg[2];
	int sh, mb, me, rc;

	sh = (iword >> 11) & 31;
	mb = (iword >> 6) & 31;
	me = (iword >> 1) & 31;   
	rc = iword & 1;

	/*  TODO: Fix this, its performance is awful:  */
	while (sh-- != 0) {
		int b = (tmp >> 31) & 1;
		tmp = (tmp << 1) | b;
	}
	for (;;) {
		uint64_t mask;
		mask = (uint64_t)1 << (31-mb);
		ra &= ~mask;
		ra |= (tmp & mask);
		if (mb == me)
			break;
		mb ++;
		if (mb == 32)
			mb = 0;
	}
	reg(ic->arg[1]) = ra;
	if (rc)
		update_cr0(cpu, ra);
}


/*
 *  srawi:
 *
 *  arg[0] = ptr to rs
 *  arg[1] = ptr to ra
 *  arg[2] = sh (shift amount)
 */
X(srawi)
{
	uint32_t tmp = reg(ic->arg[0]);
	int i = 0, j = 0, sh = ic->arg[2];

	cpu->cd.ppc.xer &= ~PPC_XER_CA;
	if (tmp & 0x80000000)
		i = 1;
	while (sh-- > 0) {
		if (tmp & 1)
			j ++;
		tmp >>= 1;
		if (tmp & 0x40000000)
			tmp |= 0x80000000;
	}
	if (i && j>0)
		cpu->cd.ppc.xer |= PPC_XER_CA;
	reg(ic->arg[1]) = (int64_t)(int32_t)tmp;
}
X(srawi_dot) { instr(srawi)(cpu,ic); update_cr0(cpu, reg(ic->arg[1])); }


/*
 *  mcrf:  Move inside condition register
 *
 *  arg[0] = bf,  arg[1] = bfa
 */
X(mcrf)
{
	int bf = ic->arg[0], bfa = ic->arg[1];
	uint32_t tmp = (cpu->cd.ppc.cr >> (28 - bfa*4)) & 0xf;
	cpu->cd.ppc.cr &= ~(0xf << (28 - bf*4));
	cpu->cd.ppc.cr |= (tmp << (28 - bf*4));
}


/*
 *  crand, crxor etc:  Condition Register operations
 *
 *  arg[0] = copy of the instruction word
 */
X(crand)
{
	uint32_t iword = ic->arg[0]; int bt = (iword >> 21) & 31;
	int ba = (iword >> 16) & 31, bb = (iword >> 11) & 31;
	ba = (cpu->cd.ppc.cr >> (31-ba)) & 1;
	bb = (cpu->cd.ppc.cr >> (31-bb)) & 1;
	cpu->cd.ppc.cr &= ~(1 << (31-bt));
	if (ba & bb)
		cpu->cd.ppc.cr |= (1 << (31-bt));
}
X(cror)
{
	uint32_t iword = ic->arg[0]; int bt = (iword >> 21) & 31;
	int ba = (iword >> 16) & 31, bb = (iword >> 11) & 31;
	ba = (cpu->cd.ppc.cr >> (31-ba)) & 1;
	bb = (cpu->cd.ppc.cr >> (31-bb)) & 1;
	cpu->cd.ppc.cr &= ~(1 << (31-bt));
	if (ba | bb)
		cpu->cd.ppc.cr |= (1 << (31-bt));
}
X(crxor)
{
	uint32_t iword = ic->arg[0]; int bt = (iword >> 21) & 31;
	int ba = (iword >> 16) & 31, bb = (iword >> 11) & 31;
	ba = (cpu->cd.ppc.cr >> (31-ba)) & 1;
	bb = (cpu->cd.ppc.cr >> (31-bb)) & 1;
	cpu->cd.ppc.cr &= ~(1 << (31-bt));
	if (ba ^ bb)
		cpu->cd.ppc.cr |= (1 << (31-bt));
}


/*
 *  mflr, etc:  Move from Link Register etc.
 *
 *  arg[0] = pointer to destination register
 */
X(mflr) {	reg(ic->arg[0]) = cpu->cd.ppc.lr; }
X(mfctr) {	reg(ic->arg[0]) = cpu->cd.ppc.ctr; }
/*  TODO: Check privilege level for mfsprg*  */
X(mfsdr1) {	reg(ic->arg[0]) = cpu->cd.ppc.sdr1; }
X(mfdbsr) {	reg(ic->arg[0]) = cpu->cd.ppc.dbsr; }
X(mfsprg0) {	reg(ic->arg[0]) = cpu->cd.ppc.sprg0; }
X(mfsprg1) {	reg(ic->arg[0]) = cpu->cd.ppc.sprg1; }
X(mfsprg2) {	reg(ic->arg[0]) = cpu->cd.ppc.sprg2; }
X(mfsprg3) {	reg(ic->arg[0]) = cpu->cd.ppc.sprg3; }
X(mfibatu) {	reg(ic->arg[0]) = cpu->cd.ppc.ibat_u[ic->arg[1]]; }
X(mfibatl) {	reg(ic->arg[0]) = cpu->cd.ppc.ibat_l[ic->arg[1]]; }
X(mfdbatu) {	reg(ic->arg[0]) = cpu->cd.ppc.dbat_u[ic->arg[1]]; }
X(mfdbatl) {	reg(ic->arg[0]) = cpu->cd.ppc.dbat_l[ic->arg[1]]; }


/*
 *  mtlr etc.:  Move to Link Register (or other special register)
 *
 *  arg[0] = pointer to source register
 */
X(mtlr) {	cpu->cd.ppc.lr = reg(ic->arg[0]); }
X(mtctr) {	cpu->cd.ppc.ctr = reg(ic->arg[0]); }
/*  TODO: Check privilege level for these:  */
X(mtsdr1) {	cpu->cd.ppc.sdr1 = reg(ic->arg[0]); }
X(mtdbsr) {	cpu->cd.ppc.dbsr = reg(ic->arg[0]); }
X(mtsprg0) {	cpu->cd.ppc.sprg0 = reg(ic->arg[0]); }
X(mtsprg1) {	cpu->cd.ppc.sprg1 = reg(ic->arg[0]); }
X(mtsprg2) {	cpu->cd.ppc.sprg2 = reg(ic->arg[0]); }
X(mtsprg3) {	cpu->cd.ppc.sprg3 = reg(ic->arg[0]); }
X(mtibatu) {	cpu->cd.ppc.ibat_u[ic->arg[1]] = reg(ic->arg[0]); }
X(mtibatl) {	cpu->cd.ppc.ibat_l[ic->arg[1]] = reg(ic->arg[0]); }
X(mtdbatu) {	cpu->cd.ppc.dbat_u[ic->arg[1]] = reg(ic->arg[0]); }
X(mtdbatl) {	cpu->cd.ppc.dbat_l[ic->arg[1]] = reg(ic->arg[0]); }


/*
 *  mfcr:  Move From Condition Register
 *
 *  arg[0] = pointer to destination register
 */
X(mfcr)
{
	reg(ic->arg[0]) = cpu->cd.ppc.cr;
}


/*
 *  mfmsr:  Move From MSR
 *
 *  arg[0] = pointer to destination register
 */
X(mfmsr)
{
	reg_access_msr(cpu, (uint64_t*)ic->arg[0], 0);
}


/*
 *  mtmsr:  Move To MSR
 *
 *  arg[0] = pointer to source register
 */
X(mtmsr)
{
	reg_access_msr(cpu, (uint64_t*)ic->arg[0], 1);
}


/*
 *  mtcrf:  Move To Condition Register Fields
 *
 *  arg[0] = pointer to source register
 */
X(mtcrf)
{
	cpu->cd.ppc.cr &= ~ic->arg[1];
	cpu->cd.ppc.cr |= (reg(ic->arg[0]) & ic->arg[1]);
}


/*
 *  mulli:  Multiply Low Immediate.
 *
 *  arg[0] = pointer to source register ra
 *  arg[1] = int32_t immediate
 *  arg[2] = pointer to destination register rt
 */
X(mulli)
{
	reg(ic->arg[2]) = (uint32_t)(reg(ic->arg[0]) * ic->arg[1]);
}


/*
 *  Shifts, and, or, xor, etc.
 *
 *  arg[0] = pointer to source register rs
 *  arg[1] = pointer to source register rb
 *  arg[2] = pointer to destination register ra
 */
X(slw) {	reg(ic->arg[2]) = (uint64_t)reg(ic->arg[0])
		    << (reg(ic->arg[1]) & 63); }
X(slw_dot) {	instr(slw)(cpu,ic); update_cr0(cpu, reg(ic->arg[2])); }
X(sraw) {	reg(ic->arg[2]) =
#ifdef MODE32
		    (int32_t)
#else
		    (int64_t)
#endif
		reg(ic->arg[0]) >> (reg(ic->arg[1]) & 63); }
X(sraw_dot) {	instr(sraw)(cpu,ic); update_cr0(cpu, reg(ic->arg[2])); }
X(srw) {	reg(ic->arg[2]) = (uint64_t)reg(ic->arg[0])
		    >> (reg(ic->arg[1]) & 63); }
X(srw_dot) {	instr(srw)(cpu,ic); update_cr0(cpu, reg(ic->arg[2])); }
X(and) {	reg(ic->arg[2]) = reg(ic->arg[0]) & reg(ic->arg[1]); }
X(and_dot) {	reg(ic->arg[2]) = reg(ic->arg[0]) & reg(ic->arg[1]);
		update_cr0(cpu, reg(ic->arg[2])); }
X(nand) {	reg(ic->arg[2]) = ~(reg(ic->arg[0]) & reg(ic->arg[1])); }
X(nand_dot) {	reg(ic->arg[2]) = ~(reg(ic->arg[0]) & reg(ic->arg[1]));
		update_cr0(cpu, reg(ic->arg[2])); }
X(andc) {	reg(ic->arg[2]) = reg(ic->arg[0]) & (~reg(ic->arg[1])); }
X(andc_dot) {	reg(ic->arg[2]) = reg(ic->arg[0]) & (~reg(ic->arg[1]));
		update_cr0(cpu, reg(ic->arg[2])); }
X(nor) {	reg(ic->arg[2]) = ~(reg(ic->arg[0]) | reg(ic->arg[1])); }
X(nor_dot) {	reg(ic->arg[2]) = ~(reg(ic->arg[0]) | reg(ic->arg[1]));
		update_cr0(cpu, reg(ic->arg[2])); }
X(or) {		reg(ic->arg[2]) = reg(ic->arg[0]) | reg(ic->arg[1]); }
X(or_dot) {	reg(ic->arg[2]) = reg(ic->arg[0]) | reg(ic->arg[1]);
		update_cr0(cpu, reg(ic->arg[2])); }
X(orc) {	reg(ic->arg[2]) = reg(ic->arg[0]) | (~reg(ic->arg[1])); }
X(orc_dot) {	reg(ic->arg[2]) = reg(ic->arg[0]) | (~reg(ic->arg[1]));
		update_cr0(cpu, reg(ic->arg[2])); }
X(xor) {	reg(ic->arg[2]) = reg(ic->arg[0]) ^ reg(ic->arg[1]); }
X(xor_dot) {	reg(ic->arg[2]) = reg(ic->arg[0]) ^ reg(ic->arg[1]);
		update_cr0(cpu, reg(ic->arg[2])); }


/*
 *  neg:
 *
 *  arg[0] = pointer to source register ra
 *  arg[1] = pointer to destination register rt
 */
X(neg) {	reg(ic->arg[1]) = ~reg(ic->arg[0]) + 1; }
X(neg_dot) {	reg(ic->arg[1]) = ~reg(ic->arg[0]) + 1;
		update_cr0(cpu, reg(ic->arg[1])); }


/*
 *  mullw, mulhw[u], divw[u]:
 *
 *  arg[0] = pointer to source register ra
 *  arg[1] = pointer to source register rb
 *  arg[2] = pointer to destination register rt
 */
X(mullw)
{
	int32_t sum = (int32_t)reg(ic->arg[0]) * (int32_t)reg(ic->arg[1]);
	reg(ic->arg[2]) = (int32_t)sum;
}
X(mulhw)
{
	int64_t sum;
	sum = (int64_t)(int32_t)reg(ic->arg[0])
	    * (int64_t)(int32_t)reg(ic->arg[1]);
	reg(ic->arg[2]) = sum >> 32;
}
X(mulhwu)
{
	uint64_t sum;
	sum = (uint64_t)(uint32_t)reg(ic->arg[0])
	    * (uint64_t)(uint32_t)reg(ic->arg[1]);
	reg(ic->arg[2]) = sum >> 32;
}
X(divw)
{
	int32_t a = reg(ic->arg[0]), b = reg(ic->arg[1]);
	int32_t sum;
	if (b == 0)
		sum = 0;
	else
		sum = a / b;
	reg(ic->arg[2]) = (uint32_t)sum;
}
X(divwu)
{
	uint32_t a = reg(ic->arg[0]), b = reg(ic->arg[1]);
	uint32_t sum;
	if (b == 0)
		sum = 0;
	else
		sum = a / b;
	reg(ic->arg[2]) = sum;
}


/*
 *  add:  Add.
 *
 *  arg[0] = pointer to source register ra
 *  arg[1] = pointer to source register rb
 *  arg[2] = pointer to destination register rt
 */
X(add)
{
	reg(ic->arg[2]) = reg(ic->arg[0]) + reg(ic->arg[1]);
}


/*
 *  addc:  Add carrying.
 *
 *  arg[0] = pointer to source register ra
 *  arg[1] = pointer to source register rb
 *  arg[2] = pointer to destination register rt
 */
X(addc)
{
	/*  TODO: this only works in 32-bit mode  */
	uint64_t tmp = (uint32_t)reg(ic->arg[0]);
	uint64_t tmp2 = tmp;
	cpu->cd.ppc.xer &= PPC_XER_CA;
	tmp += (uint32_t)reg(ic->arg[1]);
	if ((tmp >> 32) == (tmp2 >> 32))
		cpu->cd.ppc.xer |= PPC_XER_CA;
	reg(ic->arg[2]) = (uint32_t)tmp;
}


/*
 *  adde:  Add extended, etc.
 *
 *  arg[0] = pointer to source register ra
 *  arg[1] = pointer to source register rb
 *  arg[2] = pointer to destination register rt
 */
X(adde)
{
	int old_ca = cpu->cd.ppc.xer & PPC_XER_CA;
	uint64_t tmp = (uint32_t)reg(ic->arg[0]);
	uint64_t tmp2 = tmp;
	cpu->cd.ppc.xer &= PPC_XER_CA;
	tmp += (uint32_t)reg(ic->arg[1]);
	if (old_ca)
		tmp ++;
	if ((tmp >> 32) == (tmp2 >> 32))
		cpu->cd.ppc.xer |= PPC_XER_CA;

	reg(ic->arg[2]) = (uint32_t)tmp;
}
X(addze)
{
	int old_ca = cpu->cd.ppc.xer & PPC_XER_CA;
	uint64_t tmp = (uint32_t)reg(ic->arg[0]);
	uint64_t tmp2 = tmp;
	cpu->cd.ppc.xer &= PPC_XER_CA;
	if (old_ca)
		tmp ++;
	if ((tmp >> 32) == (tmp2 >> 32))
		cpu->cd.ppc.xer |= PPC_XER_CA;
	reg(ic->arg[2]) = (uint32_t)tmp;
}


/*
 *  subf:  Subf, etc.
 *
 *  arg[0] = pointer to source register ra
 *  arg[1] = pointer to source register rb
 *  arg[2] = pointer to destination register rt
 */
X(subf) {	reg(ic->arg[2]) = ~reg(ic->arg[0]) + reg(ic->arg[1]) + 1; }
X(subf_dot) {	instr(subf)(cpu,ic); update_cr0(cpu, reg(ic->arg[2])); }
X(subfc)
{
	uint64_t tmp = (uint32_t)(~reg(ic->arg[0]));
	uint64_t tmp2 = tmp;
	cpu->cd.ppc.xer &= PPC_XER_CA;
	tmp += (uint32_t)reg(ic->arg[1]) + 1;
	if ((tmp >> 32) == (tmp2 >> 32))
		cpu->cd.ppc.xer |= PPC_XER_CA;
	reg(ic->arg[2]) = (uint32_t)tmp;
}
X(subfe)
{
	int old_ca = cpu->cd.ppc.xer & PPC_XER_CA;
	uint64_t tmp = (uint32_t)(~reg(ic->arg[0]));
	uint64_t tmp2 = tmp;

	cpu->cd.ppc.xer &= PPC_XER_CA;
	tmp += (uint32_t)reg(ic->arg[1]);
	if (old_ca)
		tmp ++;
	if ((tmp >> 32) == (tmp2 >> 32))
		cpu->cd.ppc.xer |= PPC_XER_CA;
	reg(ic->arg[2]) = (uint32_t)tmp;
}
X(subfze)
{
	int old_ca = cpu->cd.ppc.xer & PPC_XER_CA;
	uint64_t tmp = (uint32_t)(~reg(ic->arg[0]));
	uint64_t tmp2 = tmp;
	cpu->cd.ppc.xer &= PPC_XER_CA;
	if (old_ca)
		tmp ++;
	if ((tmp >> 32) == (tmp2 >> 32))
		cpu->cd.ppc.xer |= PPC_XER_CA;
	reg(ic->arg[2]) = (uint32_t)tmp;
}


/*
 *  ori:  OR immediate.
 *
 *  arg[0] = pointer to source uint64_t
 *  arg[1] = immediate value (uint32_t or larger)
 *  arg[2] = pointer to destination uint64_t
 */
X(ori)
{
	reg(ic->arg[2]) = reg(ic->arg[0]) | (uint32_t)ic->arg[1];
}


/*
 *  user_syscall:  Userland syscall.
 *
 *  arg[0] = syscall "level" (usually 0)
 */
X(user_syscall)
{
	useremul_syscall(cpu, ic->arg[0]);
}


/*
 *  xori:  XOR immediate.
 *
 *  arg[0] = pointer to source uint64_t
 *  arg[1] = immediate value (uint32_t or larger)
 *  arg[2] = pointer to destination uint64_t
 */
X(xori)
{
	reg(ic->arg[2]) = reg(ic->arg[0]) ^ (uint32_t)ic->arg[1];
}


#include "tmp_ppc_loadstore.c"


/*****************************************************************************/


/*
 *  byte_fill_loop:
 *
 *  A byte-fill loop. Fills at most one page at a time. If the page was not
 *  in the host_store table, then the original sequence (beginning with
 *  cmpwi crX,rY,0) is executed instead.
 *
 *  L:  cmpwi   crX,rY,0		ic[0]
 *      stb     rW,0(rZ)		ic[1]
 *      subi    rY,rY,1			ic[2]
 *      addi    rZ,rZ,1			ic[3]
 *      bc      12,4*X+1,L		ic[4]
 */
X(byte_fill_loop)
{
	unsigned int x = ic[0].arg[2], n, ofs, maxlen, c;
	uint64_t *y = (uint64_t *)ic[0].arg[0];
	uint64_t *z = (uint64_t *)ic[1].arg[1];
	uint64_t *w = (uint64_t *)ic[1].arg[0];
	unsigned char *page;
#ifdef MODE32
	uint32_t addr = reg(z);
#else
	uint64_t addr = reg(z);
	fatal("byte_fill_loop: not for 64-bit mode yet\n");
	exit(1);
#endif
	/*  TODO: This only work with 32-bit addressing:  */
	page = cpu->cd.ppc.host_store[addr >> 12];
	if (page == NULL) {
		instr(cmpwi)(cpu, ic);
		return;
	}

	n = reg(y) + 1; ofs = addr & 0xfff; maxlen = 0x1000 - ofs;
	if (n > maxlen)
		n = maxlen;

	/*  fatal("FILL A: x=%i n=%i ofs=0x%x y=0x%x z=0x%x w=0x%x\n", x,
	    n, ofs, (int)reg(y), (int)reg(z), (int)reg(w));  */

	memset(page + ofs, *w, n);

	reg(z) = addr + n;
	reg(y) -= n;

	if ((int32_t)reg(y) + 1 < 0)
		c = 8;
	else if ((int32_t)reg(y) + 1 > 0)
		c = 4;
	else
		c = 2;
	c |= ((cpu->cd.ppc.xer >> 31) & 1);  /*  SO bit, copied from XER  */
	cpu->cd.ppc.cr &= ~(0xf << (28 - 4*x));
	cpu->cd.ppc.cr |= (c << (28 - 4*x));

	/*  NOTE: 5*n-1  */
	cpu->n_translated_instrs += (5 * n - 1);
	if ((int32_t)reg(y) > 0)
		cpu->cd.ppc.next_ic --;
	else
		cpu->cd.ppc.next_ic += 4;

	/*  fatal("FILL B: x=%i n=%i ofs=0x%x y=0x%x z=0x%x w=0x%x\n", x, n,
	    ofs, (int)reg(y), (int)reg(z), (int)reg(w));  */
}


/*****************************************************************************/


X(end_of_page)
{
	/*  Update the PC:  (offset 0, but on the next page)  */
	cpu->pc &= ~((PPC_IC_ENTRIES_PER_PAGE-1) << 2);
	cpu->pc += (PPC_IC_ENTRIES_PER_PAGE << 2);

	/*  Find the new physical page and update the translation pointers:  */
	DYNTRANS_PC_TO_POINTERS(cpu);

	/*  end_of_page doesn't count as an executed instruction:  */
	cpu->n_translated_instrs --;
}


/*****************************************************************************/


/*
 *  ppc_combine_instructions():
 *
 *  Combine two or more instructions, if possible, into a single function call.
 */
void COMBINE_INSTRUCTIONS(struct cpu *cpu, struct ppc_instr_call *ic,
	uint32_t addr)
{
	int n_back;
	n_back = (addr >> PPC_INSTR_ALIGNMENT_SHIFT)
	    & (PPC_IC_ENTRIES_PER_PAGE-1);

	if (n_back >= 4) {
		/*
		 *  L:  cmpwi   crX,rY,0		ic[-4]
		 *      stb     rW,0(rZ)		ic[-3]
		 *      subi    rY,rY,1			ic[-2]
		 *      addi    rZ,rZ,1			ic[-1]
		 *      bc      12,4*X+1,L		ic[0]
		 */
		if (ic[-4].f == instr(cmpwi) &&
		    ic[-4].arg[0] == ic[-2].arg[0] && ic[-4].arg[1] == 0 &&

		    ic[-3].f == instr(stb_0) &&
		    ic[-3].arg[1] == ic[-1].arg[0] && ic[-3].arg[2] == 0 &&

		    ic[-2].f == instr(addi) &&
		    ic[-2].arg[0] == ic[-2].arg[2] && ic[-2].arg[1] == -1 &&

		    ic[-1].f == instr(addi) &&
		    ic[-1].arg[0] == ic[-1].arg[2] && ic[-1].arg[1] ==  1 &&

		    ic[0].f == instr(bc_samepage) &&
		    ic[0].arg[0] == (size_t)&ic[-4] &&
		    ic[0].arg[1] == 12 && ic[0].arg[2] == 4*ic[-4].arg[2] + 1) {
			ic[-4].f = instr(byte_fill_loop);
			combined;
		}
	}

	/*  TODO: Combine forward as well  */
}


/*****************************************************************************/


/*
 *  ppc_instr_to_be_translated():
 *
 *  Translate an instruction word into an ppc_instr_call. ic is filled in with
 *  valid data for the translated instruction, or a "nothing" instruction if
 *  there was a translation failure. The newly translated instruction is then
 *  executed.
 */
X(to_be_translated)
{
	uint64_t addr, low_pc, tmp_addr;
	uint32_t iword;
	unsigned char *page;
	unsigned char ib[4];
	int main_opcode, rt, rs, ra, rb, rc, aa_bit, l_bit, lk_bit, spr, sh,
	    xo, imm, load, size, update, zero, bf, bo, bi, bh, oe_bit, n64=0,
	    bfa;
	void (*samepage_function)(struct cpu *, struct ppc_instr_call *);
	void (*rc_f)(struct cpu *, struct ppc_instr_call *);

	/*  Figure out the (virtual) address of the instruction:  */
	low_pc = ((size_t)ic - (size_t)cpu->cd.ppc.cur_ic_page)
	    / sizeof(struct ppc_instr_call);
	addr = cpu->pc & ~((PPC_IC_ENTRIES_PER_PAGE-1)
	    << PPC_INSTR_ALIGNMENT_SHIFT);
	addr += (low_pc << PPC_INSTR_ALIGNMENT_SHIFT);
	cpu->pc = addr;
	addr &= ~0x3;

	/*  Read the instruction word from memory:  */
	page = cpu->cd.ppc.host_load[addr >> 12];

	if (page != NULL) {
		/*  fatal("TRANSLATION HIT!\n");  */
		memcpy(ib, page + (addr & 0xfff), sizeof(ib));
	} else {
		/*  fatal("TRANSLATION MISS!\n");  */
		if (!cpu->memory_rw(cpu, cpu->mem, addr, ib,
		    sizeof(ib), MEM_READ, CACHE_INSTRUCTION)) {
			fatal("to_be_translated(): "
			    "read failed: TODO\n");
			goto bad;
		}
	}

	iword = *((uint32_t *)&ib[0]);

#ifdef HOST_LITTLE_ENDIAN
	if (cpu->byte_order == EMUL_BIG_ENDIAN)
#else
	if (cpu->byte_order == EMUL_LITTLE_ENDIAN)
#endif
		iword = ((iword & 0xff) << 24) |
			((iword & 0xff00) << 8) |
			((iword & 0xff0000) >> 8) |
			((iword & 0xff000000) >> 24);


#define DYNTRANS_TO_BE_TRANSLATED_HEAD
#include "cpu_dyntrans.c"
#undef  DYNTRANS_TO_BE_TRANSLATED_HEAD


	/*
	 *  Translate the instruction:
	 */

	main_opcode = iword >> 26;

	switch (main_opcode) {

	case PPC_HI6_MULLI:
		rt = (iword >> 21) & 31;
		ra = (iword >> 16) & 31;
		imm = (int16_t)(iword & 0xffff);
		ic->f = instr(mulli);
		ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[ra]);
		ic->arg[1] = (ssize_t)imm;
		ic->arg[2] = (size_t)(&cpu->cd.ppc.gpr[rt]);
		break;

	case PPC_HI6_SUBFIC:
		rt = (iword >> 21) & 31;
		ra = (iword >> 16) & 31;
		imm = (int16_t)(iword & 0xffff);
		ic->f = instr(subfic);
		ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[ra]);
		ic->arg[1] = (ssize_t)imm;
		ic->arg[2] = (size_t)(&cpu->cd.ppc.gpr[rt]);
		break;

	case PPC_HI6_CMPLI:
	case PPC_HI6_CMPI:
		bf = (iword >> 23) & 7;
		l_bit = (iword >> 21) & 1;
		ra = (iword >> 16) & 31;
		if (main_opcode == PPC_HI6_CMPLI) {
			imm = iword & 0xffff;
			if (l_bit)
				ic->f = instr(cmpldi);
			else
				ic->f = instr(cmplwi);
		} else {
			imm = (int16_t)(iword & 0xffff);
			if (l_bit)
				ic->f = instr(cmpdi);
			else
				ic->f = instr(cmpwi);
		}
		ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[ra]);
		ic->arg[1] = (ssize_t)imm;
		ic->arg[2] = bf;
		break;

	case PPC_HI6_ADDIC:
	case PPC_HI6_ADDIC_DOT:
		if (cpu->cd.ppc.bits == 64) {
			fatal("addic for 64-bit: TODO\n");
			goto bad;
		}
		rt = (iword >> 21) & 31;
		ra = (iword >> 16) & 31;
		imm = (int16_t)(iword & 0xffff);
		if (main_opcode == PPC_HI6_ADDIC)
			ic->f = instr(addic);
		else
			ic->f = instr(addic_dot);
		ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[ra]);
		ic->arg[1] = (ssize_t)imm;
		ic->arg[2] = (size_t)(&cpu->cd.ppc.gpr[rt]);
		break;

	case PPC_HI6_ADDI:
	case PPC_HI6_ADDIS:
		rt = (iword >> 21) & 31; ra = (iword >> 16) & 31;
		ic->f = instr(addi);
		if (ra == 0)
			ic->arg[0] = (size_t)(&cpu->cd.ppc.zero);
		else
			ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[ra]);
		ic->arg[1] = (ssize_t)(int16_t)(iword & 0xffff);
		if (main_opcode == PPC_HI6_ADDIS)
			ic->arg[1] <<= 16;
		ic->arg[2] = (size_t)(&cpu->cd.ppc.gpr[rt]);
		break;

	case PPC_HI6_ANDI_DOT:
	case PPC_HI6_ANDIS_DOT:
		rs = (iword >> 21) & 31; ra = (iword >> 16) & 31;
		ic->f = instr(andi_dot);
		ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[rs]);
		ic->arg[1] = iword & 0xffff;
		if (main_opcode == PPC_HI6_ANDIS_DOT)
			ic->arg[1] <<= 16;
		ic->arg[2] = (size_t)(&cpu->cd.ppc.gpr[ra]);
		break;

	case PPC_HI6_ORI:
	case PPC_HI6_ORIS:
	case PPC_HI6_XORI:
	case PPC_HI6_XORIS:
		rs = (iword >> 21) & 31; ra = (iword >> 16) & 31;
		if (main_opcode == PPC_HI6_ORI ||
		    main_opcode == PPC_HI6_ORIS)
			ic->f = instr(ori);
		else
			ic->f = instr(xori);
		ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[rs]);
		ic->arg[1] = iword & 0xffff;
		if (main_opcode == PPC_HI6_ORIS ||
		    main_opcode == PPC_HI6_XORIS)
			ic->arg[1] <<= 16;
		ic->arg[2] = (size_t)(&cpu->cd.ppc.gpr[ra]);
		break;

	case PPC_HI6_LBZ:
	case PPC_HI6_LBZU:
	case PPC_HI6_LHZ:
	case PPC_HI6_LHZU:
	case PPC_HI6_LWZ:
	case PPC_HI6_LWZU:
	case PPC_HI6_STB:
	case PPC_HI6_STBU:
	case PPC_HI6_STH:
	case PPC_HI6_STHU:
	case PPC_HI6_STW:
	case PPC_HI6_STWU:
		rs = (iword >> 21) & 31;
		ra = (iword >> 16) & 31;
		imm = (int16_t)(iword & 0xffff);
		load = 0; zero = 1; size = 0; update = 0;
		switch (main_opcode) {
		case PPC_HI6_LBZ:  load = 1; break;
		case PPC_HI6_LBZU: load = 1; update = 1; break;
		case PPC_HI6_LHZ:  load = 1; size = 1; break;
		case PPC_HI6_LHZU: load = 1; size = 1; update = 1; break;
		case PPC_HI6_LWZ:  load = 1; size = 2; break;
		case PPC_HI6_LWZU: load = 1; size = 2; update = 1; break;
		case PPC_HI6_STB:  break;
		case PPC_HI6_STBU: update = 1; break;
		case PPC_HI6_STH:  size = 1; break;
		case PPC_HI6_STHU: size = 1; update = 1; break;
		case PPC_HI6_STW:  size = 2; break;
		case PPC_HI6_STWU: size = 2; update = 1; break;
		}
		ic->f =
#ifdef MODE32
		    ppc32_loadstore
#else
		    ppc_loadstore
#endif
		    [size + 4*zero + 8*load + (imm==0? 16 : 0) + 32*update];

		if (ra == 0 && update) {
			fatal("TODO: ra=0 && update?\n");
			goto bad;
		}
		ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[rs]);
		if (ra == 0)
			ic->arg[1] = (size_t)(&cpu->cd.ppc.zero);
		else
			ic->arg[1] = (size_t)(&cpu->cd.ppc.gpr[ra]);
		ic->arg[2] = (ssize_t)imm;
		break;

	case PPC_HI6_BC:
		aa_bit = (iword >> 1) & 1;
		lk_bit = iword & 1;
		bo = (iword >> 21) & 31;
		bi = (iword >> 16) & 31;
		tmp_addr = (int64_t)(int16_t)(iword & 0xfffc);
		if (lk_bit) {
			fatal("lk_bit: NOT YET\n");
			goto bad;
		}
		if (aa_bit) {
			fatal("aa_bit: NOT YET\n");
			goto bad;
		}
		ic->f = instr(bc);
		samepage_function = instr(bc_samepage);
		ic->arg[0] = (ssize_t)tmp_addr;
		ic->arg[1] = bo;
		ic->arg[2] = bi;
		/*  Branches are calculated as cur PC + offset.  */
		/*  Special case: branch within the same page:  */
		{
			uint64_t mask_within_page =
			    ((PPC_IC_ENTRIES_PER_PAGE-1) << 2) | 3;
			uint64_t old_pc = addr;
			uint64_t new_pc = old_pc + (int32_t)ic->arg[0];
			if ((old_pc & ~mask_within_page) ==
			    (new_pc & ~mask_within_page)) {
				ic->f = samepage_function;
				ic->arg[0] = (size_t) (
				    cpu->cd.ppc.cur_ic_page +
				    ((new_pc & mask_within_page) >> 2));
			}
		}
		break;

	case PPC_HI6_SC:
		ic->arg[0] = (iword >> 5) & 0x7f;
		if (cpu->machine->userland_emul != NULL)
			ic->f = instr(user_syscall);
		else {
			fatal("PPC non-userland SYSCALL: TODO\n");
			goto bad;
		}
		break;

	case PPC_HI6_B:
		aa_bit = (iword & 2) >> 1;
		lk_bit = iword & 1;
		if (aa_bit) {
			fatal("aa_bit: NOT YET\n");
			goto bad;
		}
		tmp_addr = (int64_t)(int32_t)((iword & 0x03fffffc) << 6);
		tmp_addr = (int64_t)tmp_addr >> 6;
		if (lk_bit) {
			if (cpu->machine->show_trace_tree) {
				ic->f = instr(bl_trace);
				samepage_function = instr(bl_samepage_trace);
			} else {
				ic->f = instr(bl);
				samepage_function = instr(bl_samepage);
			}
		} else {
			ic->f = instr(b);
			samepage_function = instr(b_samepage);
		}
		ic->arg[0] = (ssize_t)tmp_addr;
		/*  Branches are calculated as cur PC + offset.  */
		/*  Special case: branch within the same page:  */
		{
			uint64_t mask_within_page =
			    ((PPC_IC_ENTRIES_PER_PAGE-1) << 2) | 3;
			uint64_t old_pc = addr;
			uint64_t new_pc = old_pc + (int32_t)ic->arg[0];
			if ((old_pc & ~mask_within_page) ==
			    (new_pc & ~mask_within_page)) {
				ic->f = samepage_function;
				ic->arg[0] = (size_t) (
				    cpu->cd.ppc.cur_ic_page +
				    ((new_pc & mask_within_page) >> 2));
			}
		}
		break;

	case PPC_HI6_19:
		xo = (iword >> 1) & 1023;
		switch (xo) {

		case PPC_19_BCLR:
		case PPC_19_BCCTR:
			bo = (iword >> 21) & 31;
			bi = (iword >> 16) & 31;
			bh = (iword >> 11) & 3;
			lk_bit = iword & 1;
			if (xo == PPC_19_BCLR) {
				if (lk_bit)
					ic->f = instr(bclr_l);
				else
					ic->f = instr(bclr);
			} else {
				if (lk_bit)
					ic->f = instr(bcctr_l);
				else
					ic->f = instr(bcctr);
			}
			ic->arg[0] = bo;
			ic->arg[1] = bi;
			ic->arg[2] = bh;
			break;

		case PPC_19_ISYNC:
			/*  TODO  */
			ic->f = instr(nop);
			break;

		case PPC_19_MCRF:
			bf = (iword >> 23) & 7;
			bfa = (iword >> 18) & 7;
			ic->arg[0] = bf;
			ic->arg[1] = bfa;
			ic->f = instr(mcrf);
			break;

		case PPC_19_CRAND:
		case PPC_19_CROR:
		case PPC_19_CRXOR:
			switch (xo) {
			case PPC_19_CRAND: ic->f = instr(crand); break;
			case PPC_19_CROR:  ic->f = instr(cror); break;
			case PPC_19_CRXOR: ic->f = instr(crxor); break;
			}
			ic->arg[0] = iword;
			break;

		default:goto bad;
		}
		break;

	case PPC_HI6_RLWIMI:
	case PPC_HI6_RLWINM:
		rs = (iword >> 21) & 31;
		ra = (iword >> 16) & 31;
		if (main_opcode == PPC_HI6_RLWIMI)
			ic->f = instr(rlwimi);
		else
			ic->f = instr(rlwinm);
		ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[rs]);
		ic->arg[1] = (size_t)(&cpu->cd.ppc.gpr[ra]);
		ic->arg[2] = (uint32_t)iword;
		break;

	case PPC_HI6_30:
		xo = (iword >> 2) & 7;
		switch (xo) {

		case PPC_30_RLDICR:
			ic->f = instr(rldicr);
			ic->arg[0] = iword;
			if (cpu->cd.ppc.bits == 32) {
				fatal("TODO: rldicr in 32-bit mode?\n");
				goto bad;
			}
			break;

		default:goto bad;
		}
		break;

	case PPC_HI6_31:
		xo = (iword >> 1) & 1023;
		switch (xo) {

		case PPC_31_CMPL:
		case PPC_31_CMP:
			bf = (iword >> 23) & 7;
			l_bit = (iword >> 21) & 1;
			ra = (iword >> 16) & 31;
			rb = (iword >> 11) & 31;
			if (xo == PPC_31_CMPL) {
				if (l_bit)
					ic->f = instr(cmpld);
				else
					ic->f = instr(cmplw);
			} else {
				if (l_bit)
					ic->f = instr(cmpd);
				else
					ic->f = instr(cmpw);
			}
			ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[ra]);
			ic->arg[1] = (size_t)(&cpu->cd.ppc.gpr[rb]);
			ic->arg[2] = bf;
			break;

		case PPC_31_CNTLZW:
			rs = (iword >> 21) & 31;
			ra = (iword >> 16) & 31;
			rc = iword & 1;
			if (rc) {
				fatal("TODO: rc\n");
				goto bad;
			}
			ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[rs]);
			ic->arg[1] = (size_t)(&cpu->cd.ppc.gpr[ra]);
			ic->f = instr(cntlzw);
			break;

		case PPC_31_MFSPR:
			rt = (iword >> 21) & 31;
			spr = ((iword >> 6) & 0x3e0) + ((iword >> 16) & 31);
			ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[rt]);
			switch (spr) {
			case 8:	  ic->f = instr(mflr); break;
			case 9:	  ic->f = instr(mfctr); break;
			case 25:  ic->f = instr(mfsdr1); break;
			case 272: ic->f = instr(mfsprg0); break;
			case 273: ic->f = instr(mfsprg1); break;
			case 274: ic->f = instr(mfsprg2); break;
			case 275: ic->f = instr(mfsprg3); break;
			case 1008:ic->f = instr(mfdbsr); break;
			default:if (spr >= 528 && spr < 544) {
					if (spr & 1) {
						if (spr & 16)
							ic->f = instr(mfdbatl);
						else
							ic->f = instr(mfibatl);
					} else {
						if (spr & 16)
							ic->f = instr(mfdbatu);
						else
							ic->f = instr(mfibatu);
					}
					ic->arg[1] = (spr >> 1) & 3;
				} else {
					fatal("UNIMPLEMENTED spr %i\n", spr);
					goto bad;
				}
			}
			break;

		case PPC_31_MTSPR:
			rs = (iword >> 21) & 31;
			spr = ((iword >> 6) & 0x3e0) + ((iword >> 16) & 31);
			ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[rs]);
			switch (spr) {
			case 8:	  ic->f = instr(mtlr); break;
			case 9:	  ic->f = instr(mtctr); break;
			case 25:  ic->f = instr(mtsdr1); break;
			case 272: ic->f = instr(mtsprg0); break;
			case 273: ic->f = instr(mtsprg1); break;
			case 274: ic->f = instr(mtsprg2); break;
			case 275: ic->f = instr(mtsprg3); break;
			case 1008:ic->f = instr(mtdbsr); break;
			default:if (spr >= 528 && spr < 544) {
					if (spr & 1) {
						if (spr & 16)
							ic->f = instr(mtdbatl);
						else
							ic->f = instr(mtibatl);
					} else {
						if (spr & 16)
							ic->f = instr(mtdbatu);
						else
							ic->f = instr(mtibatu);
					}
					ic->arg[1] = (spr >> 1) & 3;
				} else {
					fatal("UNIMPLEMENTED spr %i\n", spr);
					goto bad;
				}
			}
			break;

		case PPC_31_MFCR:
			rt = (iword >> 21) & 31;
			ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[rt]);
			ic->f = instr(mfcr);
			break;

		case PPC_31_MFMSR:
			rt = (iword >> 21) & 31;
			ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[rt]);
			ic->f = instr(mfmsr);
			break;

		case PPC_31_MTMSR:
			rs = (iword >> 21) & 31;
			l_bit = (iword >> 16) & 1;
			if (l_bit) {
				fatal("TODO: mtmsr l-bit\n");
				goto bad;
			}
			ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[rs]);
			ic->f = instr(mtmsr);
			break;

		case PPC_31_MTCRF:
			rs = (iword >> 21) & 31;
			{
				int i, fxm = (iword >> 12) & 255;
				uint32_t tmp = 0;
				for (i=0; i<8; i++, fxm <<= 1) {
					tmp <<= 4;
					if (fxm & 128)
						tmp |= 0xf;
				}
				ic->arg[1] = (uint32_t)tmp;
			}
			ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[rs]);
			ic->f = instr(mtcrf);
			break;

		case PPC_31_MFSRIN:
		case PPC_31_MTSRIN:
			rt = (iword >> 21) & 31;
			rb = (iword >> 11) & 31;
			ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[rb]);
			ic->arg[1] = (size_t)(&cpu->cd.ppc.gpr[rt]);
			switch (xo) {
			case PPC_31_MFSRIN: ic->f = instr(mfsrin); break;
			case PPC_31_MTSRIN: ic->f = instr(mtsrin); break;
			}
			if (cpu->cd.ppc.bits == 64) {
				fatal("Not yet for 64-bit mode\n");
				goto bad;
			}
			break;

		case PPC_31_MTSR:
			rt = (iword >> 21) & 31;
			ic->arg[0] = (iword >> 16) & 15;
			ic->arg[1] = (size_t)(&cpu->cd.ppc.gpr[rt]);
			ic->f = instr(mtsr);
			if (cpu->cd.ppc.bits == 64) {
				fatal("Not yet for 64-bit mode\n");
				goto bad;
			}
			break;

		case PPC_31_SRAWI:
			rs = (iword >> 21) & 31;
			ra = (iword >> 16) & 31;
			sh = (iword >> 11) & 31;
			rc = iword & 1;
			ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[rs]);
			ic->arg[1] = (size_t)(&cpu->cd.ppc.gpr[ra]);
			ic->arg[2] = sh;
			if (rc)
				ic->f = instr(srawi_dot);
			else
				ic->f = instr(srawi);
			break;

		case PPC_31_SYNC:
		case PPC_31_TLBSYNC:
		case PPC_31_EIEIO:
		case PPC_31_DCBST:
		case PPC_31_ICBI:
			/*  TODO  */
			ic->f = instr(nop);
			break;

		case PPC_31_NEG:
			rt = (iword >> 21) & 31;
			ra = (iword >> 16) & 31;
			rc = iword & 1;
			ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[ra]);
			ic->arg[1] = (size_t)(&cpu->cd.ppc.gpr[rt]);
			if (rc)
				ic->f = instr(neg_dot);
			else
				ic->f = instr(neg);
			break;

		case PPC_31_LBZX:
		case PPC_31_LBZUX:
		case PPC_31_LHZX:
		case PPC_31_LHZUX:
		case PPC_31_LWZX:
		case PPC_31_LWZUX:
		case PPC_31_STBX:
		case PPC_31_STBUX:
		case PPC_31_STHX:
		case PPC_31_STHUX:
		case PPC_31_STWX:
		case PPC_31_STWUX:
		case PPC_31_STDX:
		case PPC_31_STDUX:
			rs = (iword >> 21) & 31;
			ra = (iword >> 16) & 31;
			rb = (iword >> 11) & 31;
			ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[rs]);
			if (ra == 0)
				ic->arg[1] = (size_t)(&cpu->cd.ppc.zero);
			else
				ic->arg[1] = (size_t)(&cpu->cd.ppc.gpr[ra]);
			ic->arg[2] = (size_t)(&cpu->cd.ppc.gpr[rb]);
			load = 0; zero = 1; size = 0; update = 0;
			switch (xo) {
			case PPC_31_LBZX:  load = 1; break;
			case PPC_31_LBZUX: load = update = 1; break;
			case PPC_31_LHZX:  size = 1; load = 1; break;
			case PPC_31_LHZUX: size = 1; load = update = 1; break;
			case PPC_31_LWZX:  size = 2; load = 1; break;
			case PPC_31_LWZUX: size = 2; load = update = 1; break;
			case PPC_31_STBX:  break;
			case PPC_31_STBUX: update = 1; break;
			case PPC_31_STHX:  size = 1; break;
			case PPC_31_STHUX: size = 1; update = 1; break;
			case PPC_31_STWX:  size = 2; break;
			case PPC_31_STWUX: size = 2; update = 1; break;
			case PPC_31_STDX:  size = 3; break;
			case PPC_31_STDUX: size = 3; update = 1; break;
			}
			ic->f =
#ifdef MODE32
			    ppc32_loadstore_indexed
#else
			    ppc_loadstore_indexed
#endif
			    [size + 4*zero + 8*load + 16*update];
			if (ra == 0 && update) {
				fatal("TODO: ra=0 && update?\n");
				goto bad;
			}
			break;

		case PPC_31_SLW:
		case PPC_31_SRAW:
		case PPC_31_SRW:
		case PPC_31_AND:
		case PPC_31_NAND:
		case PPC_31_ANDC:
		case PPC_31_NOR:
		case PPC_31_OR:
		case PPC_31_ORC:
		case PPC_31_XOR:
			rs = (iword >> 21) & 31;
			ra = (iword >> 16) & 31;
			rb = (iword >> 11) & 31;
			rc = iword & 1;
			rc_f = NULL;
			switch (xo) {
			case PPC_31_SLW:  ic->f = instr(slw);
					  rc_f  = instr(slw_dot); break;
			case PPC_31_SRAW: ic->f = instr(sraw);
					  rc_f  = instr(sraw_dot); break;
			case PPC_31_SRW:  ic->f = instr(srw);
					  rc_f  = instr(srw_dot); break;
			case PPC_31_AND:  ic->f = instr(and);
					  rc_f  = instr(and_dot); break;
			case PPC_31_NAND: ic->f = instr(nand);
					  rc_f  = instr(nand_dot); break;
			case PPC_31_ANDC: ic->f = instr(andc);
					  rc_f  = instr(andc_dot); break;
			case PPC_31_NOR:  ic->f = instr(nor);
					  rc_f  = instr(nor_dot); break;
			case PPC_31_OR:   ic->f = instr(or);
					  rc_f  = instr(or_dot); break;
			case PPC_31_ORC:  ic->f = instr(orc);
					  rc_f  = instr(orc_dot); break;
			case PPC_31_XOR:  ic->f = instr(xor);
					  rc_f  = instr(xor_dot); break;
			}
			ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[rs]);
			ic->arg[1] = (size_t)(&cpu->cd.ppc.gpr[rb]);
			ic->arg[2] = (size_t)(&cpu->cd.ppc.gpr[ra]);
			if (rc)
				ic->f = rc_f;
			break;

		case PPC_31_MULLW:
		case PPC_31_MULHW:
		case PPC_31_MULHWU:
		case PPC_31_DIVW:
		case PPC_31_DIVWU:
		case PPC_31_ADD:
		case PPC_31_ADDC:
		case PPC_31_ADDE:
		case PPC_31_ADDZE:
		case PPC_31_SUBF:
		case PPC_31_SUBFC:
		case PPC_31_SUBFE:
		case PPC_31_SUBFZE:
			rt = (iword >> 21) & 31;
			ra = (iword >> 16) & 31;
			rb = (iword >> 11) & 31;
			oe_bit = (iword >> 10) & 1;
			rc = iword & 1;
			if (oe_bit) {
				fatal("oe_bit not yet implemented\n");
				goto bad;
			}
			switch (xo) {
			case PPC_31_MULLW:  ic->f = instr(mullw); break;
			case PPC_31_MULHW:  ic->f = instr(mulhw); break;
			case PPC_31_MULHWU: ic->f = instr(mulhwu); break;
			case PPC_31_DIVW:   ic->f = instr(divw); n64=1; break;
			case PPC_31_DIVWU:  ic->f = instr(divwu); n64=1; break;
			case PPC_31_ADD:    ic->f = instr(add); break;
			case PPC_31_ADDC:   ic->f = instr(addc); n64=1; break;
			case PPC_31_ADDE:   ic->f = instr(adde); n64=1; break;
			case PPC_31_ADDZE:  ic->f = instr(addze); n64=1; break;
			case PPC_31_SUBF:   ic->f = instr(subf); break;
			case PPC_31_SUBFC:  ic->f = instr(subfc); n64=1; break;
			case PPC_31_SUBFE:  ic->f = instr(subfe); n64=1; break;
			case PPC_31_SUBFZE: ic->f = instr(subfze); n64=1;break;
			}
			if (rc) {
				switch (xo) {
				case PPC_31_SUBF:
					ic->f = instr(subf_dot);
					break;
				default:fatal("RC bit not yet implemented\n");
					goto bad;
				}
			}
			ic->arg[0] = (size_t)(&cpu->cd.ppc.gpr[ra]);
			ic->arg[1] = (size_t)(&cpu->cd.ppc.gpr[rb]);
			ic->arg[2] = (size_t)(&cpu->cd.ppc.gpr[rt]);
			if (cpu->cd.ppc.bits == 64 && n64) {
				fatal("Not yet for 64-bit mode\n");
				goto bad;
			}
			break;

		default:goto bad;
		}
		break;

	default:goto bad;
	}


#define	DYNTRANS_TO_BE_TRANSLATED_TAIL
#include "cpu_dyntrans.c" 
#undef	DYNTRANS_TO_BE_TRANSLATED_TAIL
}
