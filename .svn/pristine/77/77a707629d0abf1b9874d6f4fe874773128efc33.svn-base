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

#include "spdk/env.h"
#include "spdk/bit_array.h"

#include <unistd.h>
#include <time.h>

extern uint64_t phys_base;
extern void *shm_base;
extern uint32_t *shm_used;
extern bool is_primary;

static uint64_t
virt_to_phys(void *vaddr)
{
    return phys_base + (vaddr - shm_base);
}

void *
spdk_malloc(size_t size, size_t align, uint64_t *phys_addr, int socket_id, uint32_t flags)
{
    void *buf;

    if (align < sizeof (void *)) {
        align = sizeof (void *);
    }

    if ((ulong)*shm_used % (ulong)align) {
        *shm_used += align - (ulong)*shm_used % align;
    }
    buf = shm_base + *shm_used;
    if ((buf != 0) && (phys_addr)) {
        *phys_addr = virt_to_phys(buf);
    }
    *shm_used += size;

    return buf;
}

void *
spdk_zmalloc(size_t size, size_t align, uint64_t *phys_addr, int socket_id, uint32_t flags)
{
	void *buf;
    
    buf = spdk_malloc(size, align, phys_addr, socket_id, flags);
	if (buf) {
		memset(buf, 0, size);
	}
	return buf;
}

void
spdk_free(void *buf)
{
}

void *
spdk_dma_malloc_socket(size_t size, size_t align, uint64_t *phys_addr, int socket_id)
{
	return spdk_malloc(size, align, phys_addr, socket_id, (SPDK_MALLOC_DMA | SPDK_MALLOC_SHARE));
}

void *
spdk_dma_zmalloc_socket(size_t size, size_t align, uint64_t *phys_addr, int socket_id)
{
	return spdk_zmalloc(size, align, phys_addr, socket_id, (SPDK_MALLOC_DMA | SPDK_MALLOC_SHARE));
}

void *
spdk_dma_malloc(size_t size, size_t align, uint64_t *phys_addr)
{
	return spdk_dma_malloc_socket(size, align, phys_addr, SPDK_ENV_SOCKET_ID_ANY);
}

void *
spdk_dma_zmalloc(size_t size, size_t align, uint64_t *phys_addr)
{
	return spdk_dma_zmalloc_socket(size, align, phys_addr, SPDK_ENV_SOCKET_ID_ANY);
}

void *
spdk_dma_realloc(void *buf, size_t size, size_t align, uint64_t *phys_addr)
{
    return spdk_dma_malloc(size, align, phys_addr);
}

void
spdk_dma_free(void *buf)
{
	spdk_free(buf);
}

void *
spdk_memzone_reserve(const char *name, size_t len, int socket_id, unsigned flags)
{
    return shm_base;
}

void *
spdk_memzone_lookup(const char *name)
{
    if (is_primary)
        return NULL;
    return shm_base;
}

int
spdk_memzone_free(const char *name)
{
    return 0;
}

void
spdk_memzone_dump(FILE *f)
{
}

struct spdk_mempool {
    char name[128];
    struct spdk_bit_array *ba;
    void *elements;
    size_t ele_size;
    size_t count;
};

struct spdk_mempool *
spdk_mempool_create(const char *name, size_t count,
		    size_t ele_size, size_t cache_size, int socket_id)
{
    struct spdk_mempool *mp;
    mp = malloc(sizeof (struct spdk_mempool));
    if (mp == NULL) {
        return NULL;
    }

    strncpy(mp->name, name, sizeof (mp->name) - 1);
    mp->name[sizeof (mp->name) - 1] = 0;
    mp->ba = spdk_bit_array_create(count);
    if (mp->ba == NULL) {
        free(mp);
        return NULL;
    }

    mp->elements = malloc(ele_size * count);
    if (mp->elements == NULL) {
        spdk_bit_array_free(&mp->ba);
        free(mp);
        return NULL;
    }
    memset(mp->elements, 0, ele_size * count);

    mp->ele_size = ele_size;
    mp->count = count;

    return mp;
}

char *
spdk_mempool_get_name(struct spdk_mempool *mp)
{
	return mp->name;
}

void
spdk_mempool_free(struct spdk_mempool *mp)
{
    free(mp->elements);
    spdk_bit_array_free(&mp->ba);
    free(mp);
}

void *
spdk_mempool_get(struct spdk_mempool *mp)
{
    uint32_t index;

    if (mp->count == 0) {
        return NULL;
    }
    index = spdk_bit_array_find_first_clear(mp->ba, 0);
    if (index == UINT_MAX) {
        return NULL;
    }
    spdk_bit_array_set(mp->ba, index);
    mp->count--;
    return mp->elements + (mp->ele_size * index);
}

int
spdk_mempool_get_bulk(struct spdk_mempool *mp, void **ele_arr, size_t count)
{
    uint32_t i;

    if (mp->count < count) {
        return -1;
    }

    for (i = 0; i < count; i++) {
        ele_arr[i] = spdk_mempool_get(mp);
    }
	return 0;
}

void
spdk_mempool_put(struct spdk_mempool *mp, void *ele)
{
    uint32_t index;

    index = (ele - mp->elements) / mp->ele_size;
    spdk_bit_array_clear(mp->ba, index);
    mp->count++;
}

void
spdk_mempool_put_bulk(struct spdk_mempool *mp, void *const *ele_arr, size_t count)
{
    uint32_t i;

    for (i = 0; i < count; i++) {
        spdk_mempool_put(mp, ele_arr[i]);
    }
}

size_t
spdk_mempool_count(const struct spdk_mempool *pool)
{
    return pool->count;
}

bool
spdk_process_is_primary(void)
{
	return is_primary;
}

uint64_t spdk_get_ticks(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000000000ULL) + ts.tv_nsec;
}

uint64_t spdk_get_ticks_hz(void)
{
    return 1000000000ULL;
}

void spdk_delay_us(unsigned int us)
{
	usleep(us);
}

void
spdk_unaffinitize_thread(void)
{
}

void *
spdk_call_unaffinitized(void *cb(void *arg), void *arg)
{
	return NULL;
}

struct spdk_ring *
spdk_ring_create(enum spdk_ring_type type, size_t count, int socket_id)
{
    return NULL;
}

void
spdk_ring_free(struct spdk_ring *ring)
{
}

size_t
spdk_ring_count(struct spdk_ring *ring)
{
    return 0;
}

size_t
spdk_ring_enqueue(struct spdk_ring *ring, void **objs, size_t count)
{
    return 0;
}

size_t
spdk_ring_dequeue(struct spdk_ring *ring, void **objs, size_t count)
{
    return 0;
}
