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
 *  $Id: of.c,v 1.5 2005-11-27 06:17:01 debug Exp $
 *
 *  OpenFirmware emulation.
 *
 *  NOTE: OpenFirmware is used on quite a variety of different hardware archs,
 *        at least POWER/PowerPC, ARM, and SPARC, so the code in this module
 *        must always remain architecture agnostic.
 *
 *  TODO: o) 64-bit OpenFirmware?
 *        o) More devices...
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define OF_C

#include "console.h"
#include "cpu.h"
#include "machine.h"
#include "memory.h"
#include "misc.h"
#include "of.h"


/*  #define debug fatal  */

extern int quiet_mode;
extern int verbose;


/*
 *  readstr():
 *
 *  Helper function to read a string from emulated memory.
 */
static void readstr(struct cpu *cpu, uint64_t addr, char *strbuf,
	int bufsize)
{
	int i;
	for (i=0; i<bufsize; i++) {
		unsigned char ch;
		cpu->memory_rw(cpu, cpu->mem, addr + i,
		    &ch, sizeof(ch), MEM_READ, CACHE_DATA | NO_EXCEPTIONS);
		strbuf[i] = '\0';
		if (ch >= 1 && ch < 32)
			ch = 0;
		strbuf[i] = ch;
		if (strbuf[i] == '\0')
			break;
	}

	strbuf[bufsize - 1] = '\0';
}


/*
 *  find_device_handle():
 *
 *  name may consist of multiple names, separaed with slashes.
 */
static int find_device_handle(struct of_data *ofd, char *name)
{
	int handle = 0, cur_parent = 0;

	for (;;) {
		struct of_device *od = ofd->of_devices;
		char tmp[200];
		int i;

		/*  fatal("find_device_handle(): '%s'\n", name);  */
		while (name[0] == '/')
			name++;
		if (name[0] == '\0')
			break;
		snprintf(tmp, sizeof(tmp), "%s", name);
		i = 0;
		while (tmp[i] != '\0' && tmp[i] != '/')
			i++;
		tmp[i] = '\0';

		OF_FIND(od, strcmp(od->name, tmp) == 0 &&
		    od->parent == cur_parent);
		if (od == NULL)
			return 0;

		handle = cur_parent = od->handle;
		name += strlen(tmp);
	}

	/*  fatal("find_device_handle(): returning %i\n", handle);  */
	return handle;
}


/*****************************************************************************/


OF_SERVICE(call_method_2_2)
{
	fatal("[ of: call_method_2_2('%s'): TODO ]\n", arg[0]);
	return -1;
}


OF_SERVICE(call_method_3_4)
{
	fatal("[ of: call_method_3_4('%s'): TODO ]\n", arg[0]);
	return -1;
}


OF_SERVICE(call_method_6_2)
{
	fatal("[ of: call_method_6_2('%s'): TODO ]\n", arg[0]);
	return -1;
}


OF_SERVICE(child)
{
	struct of_device *od = cpu->machine->of_data->of_devices;
	int handle = OF_GET_ARG(0);
	OF_FIND(od, od->parent == handle);
	store_32bit_word(cpu, base + retofs, od == NULL? 0 : od->handle);
	return 0;
}


OF_SERVICE(exit)
{
	cpu->running = 0;
	return 0;
}


OF_SERVICE(finddevice)
{
	int h = find_device_handle(cpu->machine->of_data, arg[0]);
	store_32bit_word(cpu, base + retofs, h);
	return h>0? 0 : (strcmp(arg[0], "/")==0? 0 : -1);
}


OF_SERVICE(getprop)
{
	struct of_device *od = cpu->machine->of_data->of_devices;
	struct of_device_property *pr;
	int handle = OF_GET_ARG(0), i, len_returned = 0;
	uint64_t buf = OF_GET_ARG(2);
	uint64_t max = OF_GET_ARG(3);

	OF_FIND(od, od->handle == handle);
	if (od == NULL) {
		fatal("[ of: WARNING: getprop handle=%i; no such handle ]\n",
		    handle);
		return -1;
	}

	pr = od->properties;
	OF_FIND(pr, strcmp(pr->name, arg[1]) == 0);
	if (pr == NULL) {
		fatal("[ of: WARNING: getprop: no property '%s' at handle"
		    " %i (device '%s') ]\n", arg[1], handle, od->name);
		/*  exit(1);  */
		return -1;
	}

	if (pr->data == NULL) {
		fatal("[ of: WARNING: property '%s' of '%s' has no data! ]\n",
		    arg[1], od->name);
		goto ret;
	}

	/*  Return the property into emulated RAM:  */
	len_returned = pr->len <= max? pr->len : max;

	for (i=0; i<len_returned; i++) {
		if (!cpu->memory_rw(cpu, cpu->mem, buf + i, pr->data + i,
		    1, MEM_WRITE, CACHE_DATA | NO_EXCEPTIONS)) {
			fatal("[ of: getprop memory_rw() error ]\n");
			exit(1);
		}
	}

ret:
	store_32bit_word(cpu, base + retofs, len_returned);
	return 0;
}


OF_SERVICE(getproplen)
{
	struct of_device *od = cpu->machine->of_data->of_devices;
	struct of_device_property *pr;
	int handle = OF_GET_ARG(0);

	OF_FIND(od, od->handle == handle);
	if (od == NULL) {
		fatal("[ of: WARNING: getproplen handle=%i; no such handle ]\n",
		    handle);
		exit(1);
		/*  return -1;  */
	}

	pr = od->properties;
	OF_FIND(pr, strcmp(pr->name, arg[1]) == 0);
	if (pr == NULL) {
		fatal("[ of: WARNING: getproplen: no property '%s' at handle"
		    " %i (device '%s') ]\n", arg[1], handle, od->name);
		exit(1);
	}

	store_32bit_word(cpu, base + retofs, pr->len);
	return 0;
}


OF_SERVICE(instance_to_package)
{
	int handle = OF_GET_ARG(0);
	/*  TODO: actually do something here? :-)  */
	store_32bit_word(cpu, base + retofs, handle);
	return 0;
}


OF_SERVICE(interpret_1)
{
	if (strcmp(arg[0], "#lines 2 - to line#") == 0) {
	} else {
		fatal("[ of: interpret_1('%s'): TODO ]\n", arg[0]);
		return -1;
	}
	return 0;
}


OF_SERVICE(interpret_2)
{
	store_32bit_word(cpu, base + retofs, 0);	/*  ?  TODO  */
	if (strcmp(arg[0], "#columns") == 0) {
		store_32bit_word(cpu, base + retofs + 4, 80);
	} else if (strcmp(arg[0], "#lines") == 0) {
		store_32bit_word(cpu, base + retofs + 4, 40);
	} else if (strcmp(arg[0], "char-height") == 0) {
		store_32bit_word(cpu, base + retofs + 4, 15);
	} else if (strcmp(arg[0], "char-width") == 0) {
		store_32bit_word(cpu, base + retofs + 4, 10);
	} else if (strcmp(arg[0], "line#") == 0) {
		store_32bit_word(cpu, base + retofs + 4, 0);
	} else if (strcmp(arg[0], "font-adr") == 0) {
		store_32bit_word(cpu, base + retofs + 4, 0);
	} else {
		fatal("[ of: interpret_2('%s'): TODO ]\n", arg[0]);
		return -1;
	}
	return 0;
}


OF_SERVICE(parent)
{
	struct of_device *od = cpu->machine->of_data->of_devices;
	int handle = OF_GET_ARG(0);
	OF_FIND(od, od->handle == handle);
	store_32bit_word(cpu, base + retofs, od == NULL? 0 : od->parent);
	return 0;
}


OF_SERVICE(peer)
{
	struct of_device *od = cpu->machine->of_data->of_devices;
	int handle = OF_GET_ARG(0), parent = 0, peer = 0, seen_self = 1;

	if (handle != 0) {
		OF_FIND(od, od->handle == handle);
		if (od == NULL) {
			fatal("[ of: peer(): can't find handle %i ]\n", handle);
			exit(1);
		}
		parent = od->parent;
		seen_self = 0;
	}

	od = cpu->machine->of_data->of_devices;

	while (od != NULL) {
		if (od->parent == parent) {
			if (seen_self) {
				peer = od->handle;
				break;
			}
			if (od->handle == handle)
				seen_self = 1;
		}
		od = od->next;
	}
	store_32bit_word(cpu, base + retofs, peer);
	return 0;
}


OF_SERVICE(read)
{
	int handle = OF_GET_ARG(0);
	uint64_t ptr = OF_GET_ARG(1);
	int len = OF_GET_ARG(2);
	char ch = -1;

	/*  TODO: check handle! This just reads chars from the console!  */
	/*  TODO: This is blocking!  */

	ch = console_readchar(cpu->machine->main_console_handle);
	if (!cpu->memory_rw(cpu, cpu->mem, ptr, &ch, 1, MEM_WRITE,
	    CACHE_DATA | NO_EXCEPTIONS)) {
		fatal("[ of: read: memory_rw() error ]\n");
		exit(1);
	}

	store_32bit_word(cpu, base + retofs, ch==-1? 0 : 1);
	return ch==-1? -1 : 0;
}


OF_SERVICE(write)
{
	int handle = OF_GET_ARG(0);
	uint64_t ptr = OF_GET_ARG(1);
	int n_written = 0, i, len = OF_GET_ARG(2);

	/*  TODO: check handle! This just dumps the data to the console!  */

	for (i=0; i<len; i++) {
		unsigned char ch;
		if (!cpu->memory_rw(cpu, cpu->mem, ptr + i, &ch,
		    1, MEM_READ, CACHE_DATA | NO_EXCEPTIONS)) {
			fatal("[ of: write: memory_rw() error ]\n");
			exit(1);
		}
		if (ch != 7)
			console_putchar(cpu->machine->main_console_handle, ch);
		n_written ++;
	}

	store_32bit_word(cpu, base + retofs, n_written);
	return 0;
}


/*****************************************************************************/


/*
 *  of_get_unused_device_handle():
 *
 *  Returns an unused device handle number (1 or higher).
 */
static int of_get_unused_device_handle(struct of_data *of_data)
{
	int max_handle = 0;
	struct of_device *od = of_data->of_devices;

	while (od != NULL) {
		if (od->handle > max_handle)
			max_handle = od->handle;
		od = od->next;
	}

	return max_handle + 1;
}


/*
 *  of_add_device():
 *
 *  Adds a device.
 */
static struct of_device *of_add_device(struct of_data *of_data, char *name,
	char *parentname)
{
	struct of_device *od = malloc(sizeof(struct of_device));
	if (od == NULL)
		goto bad;
	memset(od, 0, sizeof(struct of_device));

	od->name = strdup(name);
	if (od->name == NULL)
		goto bad;

	od->handle = of_get_unused_device_handle(of_data);
	od->parent = find_device_handle(of_data, parentname);

	od->next = of_data->of_devices;
	of_data->of_devices = od;
	return od;

bad:
	fatal("of_add_device(): out of memory\n");
	exit(1);
}


/*
 *  of_add_prop():
 *
 *  Adds a property to a device.
 */
static void of_add_prop(struct of_data *of_data, char *devname,
	char *propname, unsigned char *data, uint32_t len)
{
	struct of_device_property *pr =
	    malloc(sizeof(struct of_device_property));
	struct of_device *od = of_data->of_devices;
	int h = find_device_handle(of_data, devname);

	OF_FIND(od, od->handle == h);
	if (od == NULL) {
		fatal("of_add_prop(): device '%s' not registered\n", devname);
		exit(1);
	}

	if (pr == NULL)
		goto bad;
	memset(pr, 0, sizeof(struct of_device_property));

	pr->name = strdup(propname);
	if (pr->name == NULL)
		goto bad;
	pr->data = data;
	pr->len = len;

	pr->next = od->properties;
	od->properties = pr;
	return;

bad:
	fatal("of_add_device(): out of memory\n");
	exit(1);
}


/*
 *  of_add_service():
 *
 *  Adds a service.
 */
static void of_add_service(struct of_data *of_data, char *name,
	int (*f)(OF_SERVICE_ARGS), int n_args, int n_ret_args)
{
	struct of_service *os = malloc(sizeof(struct of_service));
	if (os == NULL)
		goto bad;
	memset(os, 0, sizeof(struct of_service));

	os->name = strdup(name);
	if (os->name == NULL)
		goto bad;

	os->f = f;
	os->n_args = n_args;
	os->n_ret_args = n_ret_args;

	os->next = of_data->of_services;
	of_data->of_services = os;
	return;

bad:
	fatal("of_add_service(): out of memory\n");
	exit(1);
}


/*
 *  of_dump_devices():
 *
 *  Debug dump helper.
 */
static void of_dump_devices(struct of_data *ofd, int parent)
{
	int iadd = 4;
	struct of_device *od = ofd->of_devices;

	while (od != NULL) {
		struct of_device_property *pr = od->properties;
		if (od->parent != parent) {
			od = od->next;
			continue;
		}
		debug("\"%s\"\n", od->name, od->handle);
		debug_indentation(iadd);
		while (pr != NULL) {
			debug("(%s: %i bytes)\n", pr->name,
			    pr->len, pr->data);
			pr = pr->next;
		}
		of_dump_devices(ofd, od->handle);
		debug_indentation(-iadd);
		od = od->next;
	}
}


/*
 *  of_dump_all():
 *
 *  Debug dump.
 */
static void of_dump_all(struct of_data *ofd)
{
	int iadd = 4;
	struct of_service *os;

	debug("openfirmware debug dump:\n");
	debug_indentation(iadd);

	/*  Devices:  */
	of_dump_devices(ofd, 0);

	/*  Services:  */
	os = ofd->of_services;
	while (os != NULL) {
		debug("service '%s'", os->name);
		if (os->n_ret_args > 0 || os->n_args > 0) {
			debug(" (");
			if (os->n_args > 0) {
				debug("%i arg%s", os->n_args,
				    os->n_args > 1? "s" : "");
				if (os->n_ret_args > 0)
					debug(", ");
			}
			if (os->n_ret_args > 0)
				debug("%i return value%s", os->n_ret_args,
				    os->n_ret_args > 1? "s" : "");
			debug(")");
		}
		debug("\n");
		os = os->next;
	}

	debug_indentation(-iadd);
}


/*
 *  of_add_prop_int32():
 *
 *  Helper function.
 */
static void of_add_prop_int32(struct machine *machine, struct of_data *ofd,
	char *devname, char *propname, int32_t x)
{
	unsigned char *p = malloc(sizeof(int32_t));
	if (p == NULL) {
		fatal("of_add_prop_int32(): out of memory\n");
		exit(1);
	}
	store_32bit_word_in_host(machine->cpus[0], p, x);
	of_add_prop(ofd, devname, propname, p, sizeof(int32_t));
}


/*
 *  of_add_prop_str():
 *
 *  Helper function.
 */
static void of_add_prop_str(struct machine *machine, struct of_data *ofd,
	char *devname, char *propname, char *data, int maxlen)
{
	unsigned char *p = malloc(maxlen);
	if (p == NULL) {
		fatal("of_add_prop_str(): out of memory\n");
		exit(1);
	}
	memset(p, 0, maxlen);
	snprintf(p, maxlen, "%s", data);
	of_add_prop(ofd, devname, propname, p, maxlen);
}


/*
 *  of_emul_init():
 *
 *  This function creates an OpenFirmware emulation instance.
 */
struct of_data *of_emul_init(struct machine *machine)
{
	unsigned char *memory_reg, *memory_av;
	unsigned char *zs_assigned_addresses;
	struct of_device *mmu, *devstdout, *devstdin;
	struct of_data *ofd = malloc(sizeof(struct of_data));
	int i;

	if (ofd == NULL)
		goto bad;
	memset(ofd, 0, sizeof(struct of_data));

	/*  Devices:  */
	of_add_device(ofd, "io", "/");
	devstdin  = of_add_device(ofd, "stdin", "/io");
	devstdout = of_add_device(ofd, "stdout", "/io");

	if (machine->use_x11) {
		fatal("!\n!  TODO: keyboard + framebuffer for MacPPC\n!\n");

		of_add_prop_str(machine, ofd, "/io/stdin", "name",
		    "keyboard", 16);
		of_add_prop_str(machine, ofd, "/io", "name", "adb", 16);

		of_add_prop_str(machine, ofd, "/io/stdout", "device_type",
		    "display", 16);
		of_add_prop_int32(machine, ofd, "/io/stdout", "width", 800);
		of_add_prop_int32(machine, ofd, "/io/stdout", "height", 600);
		of_add_prop_int32(machine, ofd, "/io/stdout",
		    "linebytes", 800 * 1);
		of_add_prop_int32(machine, ofd, "/io/stdout", "depth", 8);
		of_add_prop_int32(machine, ofd, "/io/stdout", "address",
		    0xf1000000);
	} else {
		zs_assigned_addresses = malloc(12);
		if (zs_assigned_addresses == NULL)
			goto bad;
		memset(zs_assigned_addresses, 0, 12);
		of_add_prop_str(machine, ofd, "/io/stdin", "name",
		    "zs", 16);
		of_add_prop_str(machine, ofd, "/io/stdin", "device_type",
		    "serial", 16);
		of_add_prop_int32(machine, ofd, "/io/stdin", "reg",
		    0xf0000000);
		of_add_prop(ofd, "/io/stdin", "assigned-addresses",
		    zs_assigned_addresses, 12);

		of_add_prop_str(machine, ofd, "/io/stdout", "device_type",
		    "serial", 16);
	}

	of_add_device(ofd, "cpus", "/");
	for (i=0; i<machine->ncpus; i++) {
		char tmp[50];
		snprintf(tmp, sizeof(tmp), "@%x", i);
		of_add_device(ofd, tmp, "/cpus");
		snprintf(tmp, sizeof(tmp), "/cpus/@%x", i);
		of_add_prop_str(machine, ofd, tmp, "device_type", "cpu", 16);
		of_add_prop_int32(machine, ofd, tmp, "timebase-frequency",
		    machine->emulated_hz / 4);
		of_add_prop_int32(machine, ofd, tmp, "reg", i);
	}

	mmu = of_add_device(ofd, "mmu", "/");
	of_add_prop(ofd, "/mmu", "translations", NULL /* TODO */, 0);

	of_add_device(ofd, "chosen", "/");
	of_add_prop_int32(machine, ofd, "/chosen", "mmu", mmu->handle);
	of_add_prop_int32(machine, ofd, "/chosen", "stdin", devstdin->handle);
	of_add_prop_int32(machine, ofd, "/chosen", "stdout", devstdout->handle);

	of_add_device(ofd, "memory", "/");
	memory_reg = malloc(2 * sizeof(uint32_t));
	memory_av = malloc(2 * sizeof(uint32_t));
	if (memory_reg == NULL || memory_av == NULL)
		goto bad;
	store_32bit_word_in_host(machine->cpus[0], memory_reg + 0, 0);
	store_32bit_word_in_host(machine->cpus[0], memory_reg + 4,
	    machine->physical_ram_in_mb << 20);
	store_32bit_word_in_host(machine->cpus[0], memory_av + 0, 10 << 20);
	store_32bit_word_in_host(machine->cpus[0], memory_av + 4,
	    (machine->physical_ram_in_mb - 10) << 20);
	of_add_prop(ofd, "/memory", "reg", memory_reg, 2 * sizeof(uint32_t));
	of_add_prop(ofd, "/memory", "available", memory_av, 2*sizeof(uint32_t));
	of_add_prop_str(machine, ofd, "/memory","device_type","memory"/*?*/,16);

	/*  Services:  */
	of_add_service(ofd, "call-method", of__call_method_2_2, 2, 2);
	of_add_service(ofd, "call-method", of__call_method_3_4, 3, 4);
	of_add_service(ofd, "call-method", of__call_method_6_2, 6, 2);
	of_add_service(ofd, "child", of__child, 1, 1);
	of_add_service(ofd, "exit", of__exit, 0, 0);
	of_add_service(ofd, "finddevice", of__finddevice, 1, 1);
	of_add_service(ofd, "getprop", of__getprop, 4, 1);
	of_add_service(ofd, "getproplen", of__getproplen, 2, 1);
	of_add_service(ofd, "instance-to-package",
	    of__instance_to_package, 1, 1);
	of_add_service(ofd, "interpret", of__interpret_1, 1, 1);
	of_add_service(ofd, "interpret", of__interpret_2, 1, 2);
	of_add_service(ofd, "parent", of__parent, 1, 1);
	of_add_service(ofd, "peer", of__peer, 1, 1);
	of_add_service(ofd, "read", of__read, 3, 1);
	of_add_service(ofd, "write", of__write, 3, 1);

	if (verbose >= 2)
		of_dump_all(ofd);

	machine->of_data = ofd;
	return ofd;

bad:
	fatal("of_emul_init(): out of memory\n");
	exit(1);
}


/*
 *  of_emul():
 *
 *  OpenFirmware call emulation.
 */
int of_emul(struct cpu *cpu)
{
	int i, nargs, nret, ofs, retval = 0;
	char service[50];
	char *arg[OF_N_MAX_ARGS];
	uint64_t base, ptr;
	struct of_service *os;
	struct of_data *of_data = cpu->machine->of_data;

	if (of_data == NULL) {
		fatal("of_emul(): no of_data struct?\n");
		exit(1);
	}

	/*
	 *  The first argument register points to "prom_args":
	 *
	 *	char *service;		(probably 32 bit)
	 *	int nargs;
	 *	int nret;
	 *	char *args[10];
	 */

	switch (cpu->machine->arch) {
	case ARCH_ARM:
		base = cpu->cd.arm.r[0];
		break;
	case ARCH_PPC:
		base = cpu->cd.ppc.gpr[3];
		break;
	default:fatal("of_emul(): unimplemented arch (TODO)\n");
		exit(1);
	}

	/*  TODO: how about 64-bit OpenFirmware?  */
	ptr   = load_32bit_word(cpu, base);
	nargs = load_32bit_word(cpu, base + 4);
	nret  = load_32bit_word(cpu, base + 8);

	readstr(cpu, ptr, service, sizeof(service));

	debug("[ of: %s(", service);
	ofs = 12;
	for (i=0; i<nargs; i++) {
		int x;
		if (i > 0)
			debug(", ");
		if (i >= OF_N_MAX_ARGS) {
			fatal("TOO MANY ARGS!");
			continue;
		}
		ptr = load_32bit_word(cpu, base + ofs);
		arg[i] = malloc(OF_ARG_MAX_LEN + 1);
		if (arg[i] == NULL) {
			fatal("out of memory\n");
			exit(1);
		}
		memset(arg[i], 0, OF_ARG_MAX_LEN + 1);
		x = ptr;
		if (x > -256 && x < 256) {
			debug("%i", x);
		} else {
			readstr(cpu, ptr, arg[i], OF_ARG_MAX_LEN);
			if (arg[i][0])
				debug("\"%s\"", arg[i]);
			else
				debug("0x%x", x);
		}
		ofs += sizeof(uint32_t);
	}
	debug(") ]\n");

	/*  Note: base + ofs points to the first return slot.  */

	os = of_data->of_services;
	while (os != NULL) {
		if (strcmp(service, os->name) == 0 &&
		    nargs == os->n_args && nret == os->n_ret_args) {
			retval = os->f(cpu, arg, base, ofs);
			break;
		}
		os = os->next;
	}

	if (os == NULL) {
		quiet_mode = 0;
		cpu_register_dump(cpu->machine, cpu, 1, 0);
		printf("\n");
		fatal("[ of: unimplemented service \"%s\" with %i input "
		    "args and %i output values ]\n", service, nargs, nret);
		cpu->running = 0;
		cpu->dead = 1;
	}

	for (i=0; i<nargs; i++)
		free(arg[i]);

	/*  Return:  */
	switch (cpu->machine->arch) {
	case ARCH_ARM:
		cpu->cd.arm.r[0] = retval;
		break;
	case ARCH_PPC:
		cpu->cd.ppc.gpr[3] = retval;
		break;
	default:fatal("of_emul(): TODO: unimplemented arch (Retval)\n");
		exit(1);
	}

	return 1;
}

