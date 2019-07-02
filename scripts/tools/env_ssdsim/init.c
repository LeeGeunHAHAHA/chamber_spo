/*-
 *   BSD LICENSE
 *
 *   Copyright (c) Intel Corporation.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "spdk/stdinc.h"

#include "env_internal.h"

#include "spdk/version.h"

#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/shm.h>

#define SHM_OBJECT                      "ssdsim"
#define SHM_BASE                        0x100000000ULL
#define MMIO_REQ_FIFO                   "/tmp/ssdsim_mmio_req"
#define MMIO_CPL_FIFO                   "/tmp/ssdsim_mmio_cpl"

#define SPDK_ENV_DPDK_DEFAULT_NAME		"spdk"
#define SPDK_ENV_DPDK_DEFAULT_SHM_ID		-1
#define SPDK_ENV_DPDK_DEFAULT_MEM_SIZE		-1
#define SPDK_ENV_DPDK_DEFAULT_MASTER_CORE	-1
#define SPDK_ENV_DPDK_DEFAULT_MEM_CHANNEL	-1
#define SPDK_ENV_DPDK_DEFAULT_CORE_MASK		"0x1"

int mmio_req_fd;
int mmio_cpl_fd;
bool is_primary;

void *shm_base;
uint32_t *shm_used;
uint64_t phys_base;

static char **eal_cmdline;
static int eal_cmdline_argcount;

static char *
_sprintf_alloc(const char *format, ...)
{
	va_list args;
	va_list args_copy;
	char *buf;
	size_t bufsize;
	int rc;

	va_start(args, format);

	/* Try with a small buffer first. */
	bufsize = 32;

	/* Limit maximum buffer size to something reasonable so we don't loop forever. */
	while (bufsize <= 1024 * 1024) {
		buf = malloc(bufsize);
		if (buf == NULL) {
			va_end(args);
			return NULL;
		}

		va_copy(args_copy, args);
		rc = vsnprintf(buf, bufsize, format, args_copy);
		va_end(args_copy);

		/*
		 * If vsnprintf() returned a count within our current buffer size, we are done.
		 * The count does not include the \0 terminator, so rc == bufsize is not OK.
		 */
		if (rc >= 0 && (size_t)rc < bufsize) {
			va_end(args);
			return buf;
		}

		/*
		 * vsnprintf() should return the required space, but some libc versions do not
		 * implement this correctly, so just double the buffer size and try again.
		 *
		 * We don't need the data in buf, so rather than realloc(), use free() and malloc()
		 * again to avoid a copy.
		 */
		free(buf);
		bufsize *= 2;
	}

	va_end(args);
	return NULL;
}

void
spdk_env_opts_init(struct spdk_env_opts *opts)
{
	if (!opts) {
		return;
	}

	memset(opts, 0, sizeof(*opts));

	opts->name = SPDK_ENV_DPDK_DEFAULT_NAME;
	opts->core_mask = SPDK_ENV_DPDK_DEFAULT_CORE_MASK;
	opts->shm_id = SPDK_ENV_DPDK_DEFAULT_SHM_ID;
	opts->mem_size = SPDK_ENV_DPDK_DEFAULT_MEM_SIZE;
	opts->master_core = SPDK_ENV_DPDK_DEFAULT_MASTER_CORE;
	opts->mem_channel = SPDK_ENV_DPDK_DEFAULT_MEM_CHANNEL;
}

static void
spdk_free_args(char **args, int argcount)
{
	int i;

	for (i = 0; i < argcount; i++) {
		free(args[i]);
	}

	if (argcount) {
		free(args);
	}
}

static char **
spdk_push_arg(char *args[], int *argcount, char *arg)
{
	char **tmp;

	if (arg == NULL) {
		fprintf(stderr, "%s: NULL arg supplied\n", __func__);
		spdk_free_args(args, *argcount);
		return NULL;
	}

	tmp = realloc(args, sizeof(char *) * (*argcount + 1));
	if (tmp == NULL) {
		spdk_free_args(args, *argcount);
		return NULL;
	}

	tmp[*argcount] = arg;
	(*argcount)++;

	return tmp;
}

static void
spdk_destruct_eal_cmdline(void)
{
	spdk_free_args(eal_cmdline, eal_cmdline_argcount);
}


static int
spdk_build_eal_cmdline(const struct spdk_env_opts *opts)
{
	int argcount = 0;
	char **args;

	args = NULL;

	/* set the program name */
	args = spdk_push_arg(args, &argcount, _sprintf_alloc("%s", opts->name));
	if (args == NULL) {
		return -1;
	}

	/* set the coremask */
	/* NOTE: If coremask starts with '[' and ends with ']' it is a core list
	 */
	if (opts->core_mask[0] == '[') {
		char *l_arg = _sprintf_alloc("-l %s", opts->core_mask + 1);
		int len = strlen(l_arg);
		if (l_arg[len - 1] == ']') {
			l_arg[len - 1] = '\0';
		}
		args = spdk_push_arg(args, &argcount, l_arg);
	} else {
		args = spdk_push_arg(args, &argcount, _sprintf_alloc("-c %s", opts->core_mask));
	}

	if (args == NULL) {
		return -1;
	}

	/* set the memory channel number */
	if (opts->mem_channel > 0) {
		args = spdk_push_arg(args, &argcount, _sprintf_alloc("-n %d", opts->mem_channel));
		if (args == NULL) {
			return -1;
		}
	}

	/* set the memory size */
	if (opts->mem_size > 0) {
		args = spdk_push_arg(args, &argcount, _sprintf_alloc("-m %d", opts->mem_size));
		if (args == NULL) {
			return -1;
		}
	}

	/* set the master core */
	if (opts->master_core > 0) {
		args = spdk_push_arg(args, &argcount, _sprintf_alloc("--master-lcore=%d",
				     opts->master_core));
		if (args == NULL) {
			return -1;
		}
	}

	/* set no pci  if enabled */
	if (opts->no_pci) {
		args = spdk_push_arg(args, &argcount, _sprintf_alloc("--no-pci"));
		if (args == NULL) {
			return -1;
		}
	}

	/* create just one hugetlbfs file */
	if (opts->hugepage_single_segments) {
		args = spdk_push_arg(args, &argcount, _sprintf_alloc("--single-file-segments"));
		if (args == NULL) {
			return -1;
		}
	}

#ifdef __linux__
	if (opts->shm_id < 0) {
		args = spdk_push_arg(args, &argcount, _sprintf_alloc("--file-prefix=spdk_pid%d",
				     getpid()));
		if (args == NULL) {
			return -1;
		}
	} else {
		args = spdk_push_arg(args, &argcount, _sprintf_alloc("--file-prefix=spdk%d",
				     opts->shm_id));
		if (args == NULL) {
			return -1;
		}

		/* set the base virtual address */
		args = spdk_push_arg(args, &argcount, _sprintf_alloc("--base-virtaddr=0x1000000000"));
		if (args == NULL) {
			return -1;
		}

		/* set the process type */
		args = spdk_push_arg(args, &argcount, _sprintf_alloc("--proc-type=auto"));
		if (args == NULL) {
			return -1;
		}
	}
#endif

	eal_cmdline = args;
	eal_cmdline_argcount = argcount;
	if (atexit(spdk_destruct_eal_cmdline) != 0) {
		fprintf(stderr, "Failed to register cleanup handler\n");
	}

	return argcount;
}

typedef struct __bar_rw_req {
    uint16_t    funcid;
    uint8_t     bir;
    uint8_t     rw;
    uint32_t    offset;
    uint16_t    size;
    uint8_t     data[0];
} bar_rw_req_t;

enum {
    BAR_RW_REQ_READ     = 0,
    BAR_RW_REQ_WRITE    = 1,
};

static struct flock wr_lock = {
    .l_type     = F_WRLCK,
    .l_whence   = SEEK_SET,
    .l_start    = 0,
    .l_len      = 4,
};

static int __setup_ipc(void)
{
    int fd;
    int res;
    bar_rw_req_t hdr;
    size_t length;
    struct stat sb;
    struct passwd *pw;
    char name[64];

    pw = getpwuid(geteuid());

    sprintf(name, "%s_%s", MMIO_REQ_FIFO, pw->pw_name);
    mmio_req_fd = open(name, O_RDWR);
    if (mmio_req_fd < 0) {
        fprintf(stderr, "Failed to open %s\n", name);
        return -1;
    }

    sprintf(name, "%s_%s", MMIO_CPL_FIFO, pw->pw_name);
    mmio_cpl_fd = open(name, O_RDWR);
    if (mmio_cpl_fd < 0) {
        fprintf(stderr, "Failed to open %s\n", name);
        return -1;
    }

    sprintf(name, "%s_%s", SHM_OBJECT, pw->pw_name);
    fd = shm_open(name, O_RDWR, 0644);
    if (fd < 0) {
        fprintf(stderr, "Failed to open shared memory object %s\n", name);
        return -1;
    }

    fstat(fd, &sb);
    length = sb.st_size;
    shm_base = mmap((void *)SHM_BASE, length, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd ,0);
    if (shm_base == NULL) {
        fprintf(stderr, "Failed to get shared memory address\n");
        return -1;
    }
    /* reserved for memzone */
    shm_used = shm_base + 128;

    sprintf(name, "/tmp/ssdsim_%s", pw->pw_name);
    fd = open(name, O_RDWR);
    if (fd < 0) {
        fd = open(name, O_RDWR | O_CREAT, 0660);
        if (fd < 0) {
            fprintf(stderr, "Failed to open %s\n", name);
            return -1;
        }
    }
    res = fcntl(fd, F_SETLK, &wr_lock);
    if (res == 0) {
        is_primary = 1;
        *shm_used = 128 + sizeof (*shm_used);
    } else {
        is_primary = 0;
    }

    hdr.funcid = 0;
    hdr.bir = 0;
    hdr.rw = 2;
    hdr.offset = 0;
    hdr.size = 0;

    res = write(mmio_req_fd, &hdr, sizeof (hdr));
    if (res != (int)sizeof (hdr)) {
        fprintf(stderr, "write failed\n");
        return -1;
    }
    res = read(mmio_cpl_fd, &phys_base, sizeof (phys_base));
    if (res != 8) {
        fprintf(stderr, "write failed\n");
        return -1;
    }

    return 0;
}

int spdk_env_init(const struct spdk_env_opts *opts)
{
	char **dpdk_args = NULL;
	int rc;
	int orig_optind;
    int res;

	rc = spdk_build_eal_cmdline(opts);
	if (rc < 0) {
		fprintf(stderr, "Invalid arguments to initialize DPDK\n");
		return -1;
	}

	/* DPDK rearranges the array we pass to it, so make a copy
	 * before passing so we can still free the individual strings
	 * correctly.
	 */
	dpdk_args = calloc(eal_cmdline_argcount, sizeof(char *));
	if (dpdk_args == NULL) {
		fprintf(stderr, "Failed to allocate dpdk_args\n");
		return -1;
	}
	memcpy(dpdk_args, eal_cmdline, sizeof(char *) * eal_cmdline_argcount);

	fflush(stdout);
	orig_optind = optind;
	optind = 1;
	rc = 0;
	optind = orig_optind;

	free(dpdk_args);

	if (rc < 0) {
		fprintf(stderr, "Failed to initialize DPDK\n");
		return -1;
	}

	if (spdk_mem_map_init() < 0) {
		fprintf(stderr, "Failed to allocate mem_map\n");
		return -1;
	}
	if (spdk_vtophys_init() < 0) {
		fprintf(stderr, "Failed to initialize vtophys\n");
		return -1;
	}

    res = __setup_ipc();
    if (res) {
        return res;
    }

//    __setup_extent_hook();

	return 0;
}
