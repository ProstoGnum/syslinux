/* ----------------------------------------------------------------------- *
 *
 *   Copyright 2007-2008 H. Peter Anvin - All Rights Reserved
 *   Copyright 2009 Intel Corporation; author: H. Peter Anvin
 *
 *   Permission is hereby granted, free of charge, to any person
 *   obtaining a copy of this software and associated documentation
 *   files (the "Software"), to deal in the Software without
 *   restriction, including without limitation the rights to use,
 *   copy, modify, merge, publish, distribute, sublicense, and/or
 *   sell copies of the Software, and to permit persons to whom
 *   the Software is furnished to do so, subject to the following
 *   conditions:
 *
 *   The above copyright notice and this permission notice shall
 *   be included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------- */

/*
 * mboot.c
 *
 * Module to load a multiboot kernel
 */

#include "mboot.h"

struct multiboot_info mbinfo;
struct syslinux_pm_regs regs;

struct module_data {
  void *data;
  size_t len;
  const char *cmdline;
};

static int map_modules(struct module_data *modules, int nmodules)
{
  struct mod_list *mod_list;
  addr_t map_list = 0;
  size_t list_size = nmodules * sizeof *mod_list;
  int i;

  mod_list = malloc(list_size);
  if (!mod_list) {
    printf("Failed to allocate module list\n");
    return -1;
  }

  for (i = 0; i < nmodules; i++) {
    addr_t mod_map = 0;
    addr_t cmd_map = 0;

    cmd_map = map_string(modules[i].cmdline);

    mod_map = map_data(modules[i].data, modules[i].len, 4096, true);
    if (!mod_map) {
      printf("Failed to map module (memory fragmentation issue?)\n");
      return -1;
    }
    mod_list[i].mod_start = mod_map;
    mod_list[i].mod_end = mod_map + modules[i].len;
    mod_list[i].cmdline = cmd_map;
    mod_list[i].pad = 0;
  }

  map_list = map_data(mod_list, list_size, 16, false);
  if (!map_list) {
    printf("Cannot map module list\n");
    return -1;
  }

  mbinfo.flags |= MB_INFO_MODS;
  mbinfo.mods_count = nmodules;
  mbinfo.mods_addr = map_list;
  return 0;
}

static int get_modules(char **argv, struct module_data **mdp)
{
  char **argp, **argx;
  struct module_data *md, *mp;
  int rv;
  int module_count = 1;
  int arglen;
  const char module_separator[] = "---";

  for (argp = argv; *argp; argp++) {
    if (!strcmp(*argp, module_separator))
      module_count++;
  }

  *mdp = md = malloc(module_count * sizeof(struct module_data));
  if (!md) {
    error("Out of memory!\n");
    return -1;
  }

  mp = md;
  argp = argv;
  while (*argp) {
    /* Note: it seems Grub transparently decompresses all compressed files,
       not just the primary kernel. */
    printf("Loading %s... ", *argp);
    rv = zloadfile(*argp, &mp->data, &mp->len);

    if (rv) {
      printf("failed!\n");
      return -1;
    }
    printf("ok\n");

    argp++;
    arglen = 0;
    for (argx = argp; *argx && strcmp(*argx, module_separator); argx++)
      arglen += strlen(*argx)+1;

    if (arglen == 0) {
      mp->cmdline = NULL;
    } else {
      char *p;
      md->cmdline = p = malloc(arglen);
      for ( ; *argp && strcmp(*argp, module_separator); argp++) {
	p = strpcpy(p, *argp);
	*p++ = ' ';
      }
      *--p = '\0';
    }
    mp++;
    if (*argp)
      argp++;			/* Advance past module_separator */
  }

  return module_count;
}

int main(int argc, char *argv[])
{
  int nmodules;
  struct module_data *modules;
  bool keeppxe = false;

  openconsole(&dev_null_r, &dev_stdcon_w);

  if (argc < 2) {
    error("Usage: mboot.c32 mboot_file args... [--- module args...]...\n");
    return 1;
  }

  /* Load the files */
  nmodules = get_modules(argv+1, &modules);
  if (nmodules < 1) {
    error("No modules found!\n");
    return 1;			/* Failure */
  }

  /*
   * Map the primary image.  This should be done before mapping anything
   * else, since it will have fixed address requirements.
   */
  if (map_image(modules[0].data, modules[0].len))
    return 1;

  /* Map auxilliary images */
  if (nmodules > 1) {
    if (map_modules(modules+1, nmodules-1))
      return 1;
  }

  /* Map the mbinfo structure */
  regs.ebx = map_data(&mbinfo, sizeof mbinfo, 4, false);
  if (!regs.ebx) {
    error("Failed to map Multiboot info structure!\n");
    return 1;
  }

  /* Map the primary command line */
  if (modules[0].cmdline) {
    mbinfo.cmdline = map_string(modules[0].cmdline);
    if (mbinfo.cmdline)
      mbinfo.flags |= MB_INFO_CMDLINE;
  }

  /* Add auxilliary information */
  mboot_make_memmap();
  mboot_apm();

  /* Run it */
  mboot_run(keeppxe ? 3 : 0);
  error("mboot.c32: boot failed\n");
  return 1;
}
