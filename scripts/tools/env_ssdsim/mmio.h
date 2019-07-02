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

/** \file
 * Memory-mapped I/O utility functions
 */

#ifndef SPDK_MMIO_H
#define SPDK_MMIO_H

#include "spdk/stdinc.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "spdk/barrier.h"

#ifdef __x86_64__
#define SPDK_MMIO_64BIT	1 /* Can do atomic 64-bit memory read/write (over PCIe) */
#else
#define SPDK_MMIO_64BIT	0
#endif

#define BAR_BASE_ADDR   0x10000000
extern int mmio_req_fd;
extern int mmio_cpl_fd;

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

static inline uint8_t
spdk_mmio_read_1(const volatile uint8_t *addr)
{
    struct __attribute__((packed)) {
        bar_rw_req_t hdr;
    } req;
    uint8_t data;
    int res;

    req.hdr.funcid = 0;
    req.hdr.bir = 0;
    req.hdr.rw = BAR_RW_REQ_READ;
    req.hdr.offset = (uint32_t)((ulong)addr - BAR_BASE_ADDR);
    req.hdr.size = sizeof (data);

    res = write(mmio_req_fd, &req, sizeof (req));
    if (res != (int)sizeof (req)) {
        fprintf(stderr, "write failed\n");
    }

    res = read(mmio_cpl_fd, &data, sizeof (data));
    if (res != (int)sizeof (data)) {
        fprintf(stderr, "read failed\n");
    }

    return data;
}

static inline void
spdk_mmio_write_1(volatile uint8_t *addr, uint8_t val)
{
    struct __attribute__((packed)) {
        bar_rw_req_t hdr;
        uint8_t data;
    } req;
    int res;

    req.hdr.funcid = 0;
    req.hdr.bir = 0;
    req.hdr.rw = BAR_RW_REQ_WRITE;
    req.hdr.offset = (uint32_t)((ulong)addr - BAR_BASE_ADDR);
    req.hdr.size = sizeof(req.data);
    req.data = val;

    res = write(mmio_req_fd, &req, sizeof (req));
    if (res != (int)sizeof (req)) {
        fprintf(stderr, "write failed\n");
    }
}

static inline uint16_t
spdk_mmio_read_2(const volatile uint16_t *addr)
{
    struct __attribute__((packed)) {
        bar_rw_req_t hdr;
    } req;
    uint16_t data;
    int res;

    req.hdr.funcid = 0;
    req.hdr.bir = 0;
    req.hdr.rw = BAR_RW_REQ_READ;
    req.hdr.offset = (uint32_t)((ulong)addr - BAR_BASE_ADDR);
    req.hdr.size = sizeof (data);

    res = write(mmio_req_fd, &req, sizeof (req));
    if (res != (int)sizeof (req)) {
        fprintf(stderr, "write failed\n");
    }

    res = read(mmio_cpl_fd, &data, sizeof (data));
    if (res != (int)sizeof (data)) {
        fprintf(stderr, "read failed\n");
    }

    return data;
}

static inline void
spdk_mmio_write_2(volatile uint16_t *addr, uint16_t val)
{
    struct __attribute__((packed)) {
        bar_rw_req_t hdr;
        uint16_t data;
    } req;
    int res;

    req.hdr.funcid = 0;
    req.hdr.bir = 0;
    req.hdr.rw = BAR_RW_REQ_WRITE;
    req.hdr.offset = (uint32_t)((ulong)addr - BAR_BASE_ADDR);
    req.hdr.size = sizeof(req.data);
    req.data = val;

    res = write(mmio_req_fd, &req, sizeof (req));
    if (res != (int)sizeof (req)) {
        fprintf(stderr, "write failed\n");
    }
}

static inline uint32_t
spdk_mmio_read_4(const volatile uint32_t *addr)
{
    struct __attribute__((packed)) {
        bar_rw_req_t hdr;
    } req;
    uint32_t data;
    int res;

    req.hdr.funcid = 0;
    req.hdr.bir = 0;
    req.hdr.rw = BAR_RW_REQ_READ;
    req.hdr.offset = (uint32_t)((ulong)addr - BAR_BASE_ADDR);
    req.hdr.size = sizeof (data);

    res = write(mmio_req_fd, &req, sizeof (req));
    if (res != (int)sizeof (req)) {
        fprintf(stderr, "write failed\n");
    }

    res = read(mmio_cpl_fd, &data, sizeof (data));
    if (res != (int)sizeof (data)) {
        fprintf(stderr, "read failed\n");
    }

    return data;
}

static inline void
spdk_mmio_write_4(volatile uint32_t *addr, uint32_t val)
{
    struct __attribute__((packed)) {
        bar_rw_req_t hdr;
        uint32_t data;
    } req;
    int res;

    req.hdr.funcid = 0;
    req.hdr.bir = 0;
    req.hdr.rw = BAR_RW_REQ_WRITE;
    req.hdr.offset = (uint32_t)((ulong)addr - BAR_BASE_ADDR);
    req.hdr.size = sizeof(req.data);
    req.data = val;

    res = write(mmio_req_fd, &req, sizeof (req));
    if (res != (int)sizeof (req)) {
        fprintf(stderr, "write failed\n");
    }
}

static inline uint64_t
spdk_mmio_read_8(volatile uint64_t *addr)
{
    struct __attribute__((packed)) {
        bar_rw_req_t hdr;
    } req;
    uint64_t data;
    int res;

    req.hdr.funcid = 0;
    req.hdr.bir = 0;
    req.hdr.rw = BAR_RW_REQ_READ;
    req.hdr.offset = (uint32_t)((ulong)addr - BAR_BASE_ADDR);
    req.hdr.size = 8;

    res = write(mmio_req_fd, &req, sizeof (req));
    if (res < (int)sizeof (req)) {
        fprintf(stderr, "write failed\n");
    }

    res = read(mmio_cpl_fd, &data, sizeof (data));
    if (res != (int)sizeof (data)) {
        fprintf(stderr, "read failed\n");
    }

    return data;
}

static inline void
spdk_mmio_write_8(volatile uint64_t *addr, uint64_t val)
{
    struct __attribute__((packed)) {
        bar_rw_req_t hdr;
        uint64_t data;
    } req;
    int res;

    req.hdr.funcid = 0;
    req.hdr.bir = 0;
    req.hdr.rw = BAR_RW_REQ_WRITE;
    req.hdr.offset = (uint32_t)((ulong)addr - BAR_BASE_ADDR);
    req.hdr.size = sizeof(req.data);
    req.data = val;

    res = write(mmio_req_fd, &req, sizeof (req));
    if (res != (int)sizeof (req)) {
        fprintf(stderr, "write failed\n");
    }
}

#ifdef __cplusplus
}
#endif

#endif
