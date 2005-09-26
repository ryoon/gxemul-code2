/*
 *  Copyright (C) 2003-2005  Anders Gavare.  All rights reserved.
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
 *  $Id: symbol.c,v 1.32 2005-09-26 00:08:02 debug Exp $
 *
 *  Address to symbol translation routines.
 *
 *  This module is (probably) independant from the rest of the emulator.
 *  symbol_init() must be called before any other function in this
 *  file is used.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "misc.h"

#include "symbol.h"


#define	SYMBOLBUF_MAX	100


/*
 *  symbol_nsymbols():
 *
 *  Return n_symbols.
 */
int symbol_nsymbols(struct symbol_context *sc)
{
	return sc->n_symbols;
}


/*
 *  get_symbol_addr():
 *
 *  Find a symbol by name. If addr is non-NULL, *addr is set to the symbol's
 *  address. Return value is 1 if the symbol is found, 0 otherwise.
 *
 *  NOTE:  This is O(n).
 */
int get_symbol_addr(struct symbol_context *sc, char *symbol, uint64_t *addr)
{
	struct symbol *s;

	if (sc->sorted_array) {
		int i;
		for (i=0; i<sc->n_symbols; i++)
			if (strcmp(symbol, sc->first_symbol[i].name) == 0) {
				if (addr != NULL)
					*addr = sc->first_symbol[i].addr;
				return 1;
			}
	} else {
		s = sc->first_symbol;
		while (s != NULL) {
			if (strcmp(symbol, s->name) == 0) {
				if (addr != NULL)
					*addr = s->addr;
				return 1;
			}
			s = s->next;
		}
	}

	return 0;
}


/*
 *  get_symbol_name_and_n_args():
 *
 *  Translate an address into a symbol name.  The return value is a pointer
 *  to a static char array, containing the symbol name.  (In other words,
 *  this function is not reentrant. This removes the need for memory allocation
 *  at the caller's side.)
 *
 *  If offset is not a NULL pointer, *offset is set to the offset within
 *  the symbol. For example, if there is a symbol at address 0x1000 with
 *  length 0x100, and a caller wants to know the symbol name of address
 *  0x1008, the symbol's name will be found in the static char array, and
 *  *offset will be set to 0x8.
 *
 *  If n_argsp is non-NULL, *n_argsp is set to the symbol's n_args value.
 *
 *  If no symbol was found, NULL is returned instead.
 */
static char symbol_buf[SYMBOLBUF_MAX+1];
char *get_symbol_name_and_n_args(struct symbol_context *sc, uint64_t addr,
	uint64_t *offset, int *n_argsp)
{
	struct symbol *s;
	int stepsize, ofs;

	if (sc->n_symbols == 0)
		return NULL;

	if ((addr >> 32) == 0 && (addr & 0x80000000ULL))
		addr |= 0xffffffff00000000ULL;

	symbol_buf[0] = symbol_buf[SYMBOLBUF_MAX] = '\0';
	if (offset != NULL)
		*offset = 0;

	if (!sc->sorted_array) {
		/*  Slow, linear O(n) search:  */
		s = sc->first_symbol;
		while (s != NULL) {
			/*  Found a match?  */
			if (addr >= s->addr && addr < s->addr + s->len) {
				if (addr == s->addr)
					snprintf(symbol_buf, SYMBOLBUF_MAX,
					    "%s", s->name);
				else
					snprintf(symbol_buf, SYMBOLBUF_MAX,
					    "%s+0x%lx", s->name, (long)
					    (addr - s->addr));
				if (offset != NULL)
					*offset = addr - s->addr;
				if (n_argsp != NULL)
					*n_argsp = s->n_args;
				return symbol_buf;
			}
			s = s->next;
		}
	} else {
		/*  Faster, O(log n) search:  */
		stepsize = sc->n_symbols / 2;
		ofs = stepsize;
		while (stepsize > 0 || (stepsize == 0 && ofs == 0)) {
			s = sc->first_symbol + ofs;

			/*  Found a match?  */
			if (addr >= s->addr && addr < s->addr + s->len) {
				if (addr == s->addr)
					snprintf(symbol_buf, SYMBOLBUF_MAX,
					    "%s", s->name);
				else
					snprintf(symbol_buf, SYMBOLBUF_MAX,
					    "%s+0x%lx", s->name, (long)
					    (addr - s->addr));
				if (offset != NULL)
					*offset = addr - s->addr;
				if (n_argsp != NULL)
					*n_argsp = s->n_args;
				return symbol_buf;
			}

			if (ofs == 0)
				break;

			stepsize >>= 1;

			/*  Special case for offset 0 (end of search in
			    the Left direction  */
			if (stepsize == 0)
				ofs = 0;
			else {
				if (addr < s->addr)
					ofs -= stepsize;
				else
					ofs += stepsize;
			}
		}
	}

	/*  Not found? Then return NULL.  */
	return NULL;
}


/*
 *  get_symbol_name():
 *
 *  See get_symbol_name_and_n_args().
 */
char *get_symbol_name(struct symbol_context *sc, uint64_t addr, uint64_t *offs)
{
	return get_symbol_name_and_n_args(sc, addr, offs, NULL);
}


/*
 *  add_symbol_name():
 *
 *  Add a symbol to the symbol list.
 */
void add_symbol_name(struct symbol_context *sc,
	uint64_t addr, uint64_t len, char *name, int type, int n_args)
{
	struct symbol *s;

	if (sc->sorted_array) {
		fprintf(stderr, "add_symbol_name(): Internal error: the "
		    "symbol array is already sorted\n");
		exit(1);
	}

	if (name == NULL) {
		fprintf(stderr, "add_symbol_name(): name = NULL\n");
		exit(1);
	}

	if (name[0] == '\0')
		return;

	/*  TODO: Maybe this should be optional?  */
	if (name[0] == '.' || name[0] == '$')
		return;

	/*  Quick test-hack:  */
	if (n_args < 0) {
		if (strcmp(name, "strlen") == 0)
			n_args = 1;
		if (strcmp(name, "strcmp") == 0)
			n_args = 2;
		if (strcmp(name, "strcpy") == 0)
			n_args = 2;
		if (strcmp(name, "strncpy") == 0)
			n_args = 3;
		if (strcmp(name, "strlcpy") == 0)
			n_args = 3;
		if (strcmp(name, "strlcat") == 0)
			n_args = 3;
		if (strcmp(name, "strncmp") == 0)
			n_args = 3;
		if (strcmp(name, "memset") == 0)
			n_args = 3;
		if (strcmp(name, "memcpy") == 0)
			n_args = 3;
		if (strcmp(name, "bzero") == 0)
			n_args = 2;
		if (strcmp(name, "bcopy") == 0)
			n_args = 3;
	}

	if ((addr >> 32) == 0 && (addr & 0x80000000ULL))
		addr |= 0xffffffff00000000ULL;

	s = malloc(sizeof(struct symbol));
	if (s == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	s->name = strdup(name);
	if (s->name == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	s->addr   = addr;
	s->len    = len;
	s->type   = type;
	s->n_args = n_args;

	sc->n_symbols ++;

	/*  Add first in list:  */
	s->next = sc->first_symbol;
	sc->first_symbol = s;
}


/*
 *  symbol_readfile():
 *
 *  Read 'nm -S' style symbols from a file.
 *
 *  TODO: This function is an ugly hack, and should be replaced
 *  with something that reads symbols directly from the executable
 *  images.
 */
void symbol_readfile(struct symbol_context *sc, char *fname)
{
	FILE *f;
	char b1[80]; uint64_t addr;
	char b2[80]; uint64_t len;
	char b3[80]; int type;
	char b4[80];
	int cur_n_symbols = sc->n_symbols;

	f = fopen(fname, "r");
	if (f == NULL) {
		perror(fname);
		exit(1);
	}

	while (!feof(f)) {
		memset(b1, 0, sizeof(b1));
		memset(b2, 0, sizeof(b2));
		memset(b3, 0, sizeof(b3));
		memset(b4, 0, sizeof(b4));
		fscanf(f, "%s %s\n", b1,b2);
		if (strlen(b2) < 2 && !(b2[0]>='0' && b2[0]<='9')) {
			strlcpy(b3, b2, sizeof(b3));
			strlcpy(b2, "0", sizeof(b2));
			fscanf(f, "%s\n", b4);
		} else {
			fscanf(f, "%s %s\n", b3,b4);
		}

		/*  printf("b1='%s' b2='%s' b3='%s' b4='%s'\n",
		    b1,b2,b3,b4);  */
		addr = strtoull(b1, NULL, 16);
		len  = strtoull(b2, NULL, 16);
		type = b3[0];
		/*  printf("addr=%016llx len=%016llx type=%i\n",
		    addr, len, type);  */

		if (type == 't' || type == 'r' || type == 'g')
			continue;

		add_symbol_name(sc, addr, len, b4, type, -1);
	}

	fclose(f);

	debug("%i symbols\n", sc->n_symbols - cur_n_symbols);
}


/*
 *  sym_addr_compare():
 *
 *  Helper function for sorting symbols according to their address.
 */
int sym_addr_compare(const void *a, const void *b)
{
	struct symbol *p1 = (struct symbol *) a;
	struct symbol *p2 = (struct symbol *) b;

	if (p1->addr < p2->addr)
		return -1;
	if (p1->addr > p2->addr)
		return 1;

	return 0;
}


/*
 *  symbol_recalc_sizes():
 *
 *  Recalculate sizes of symbols that have size = 0, by creating an array
 *  containing all symbols, qsort()-ing that array according to address, and
 *  recalculating the size fields if necessary.
 */
void symbol_recalc_sizes(struct symbol_context *sc)
{
	struct symbol *tmp_array;
	struct symbol *last_ptr;
	struct symbol *tmp_ptr;
	int i;

	tmp_array = malloc(sizeof (struct symbol) * sc->n_symbols);
	if (tmp_array == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	/*  Copy first_symbol --> tmp_array, and remove the old
		first_symbol at the same time:  */
	tmp_ptr = sc->first_symbol;
	i = 0;
	while (tmp_ptr != NULL) {
		tmp_array[i] = *tmp_ptr;
		last_ptr = tmp_ptr;
		tmp_ptr = tmp_ptr->next;
		free(last_ptr);
		i++;
	}

	qsort(tmp_array, sc->n_symbols, sizeof(struct symbol),
	    sym_addr_compare);
	sc->sorted_array = 1;

	/*  Recreate the first_symbol chain:  */
	sc->first_symbol = NULL;
	for (i=0; i<sc->n_symbols; i++) {
		/*  Recalculate size, if 0:  */
		if (tmp_array[i].len == 0) {
			uint64_t len;
			if (i != sc->n_symbols-1)
				len = tmp_array[i+1].addr
				    - tmp_array[i].addr;
			else
				len = 1;
			tmp_array[i].len = len;
		}

		tmp_array[i].next = &tmp_array[i+1];
	}

	sc->first_symbol = tmp_array;
}


/*
 *  symbol_init():
 *
 *  Initialize the symbol hashtables.
 */
void symbol_init(struct symbol_context *sc)
{
	sc->first_symbol = NULL;
	sc->sorted_array = 0;
	sc->n_symbols = 0;
}

