/*
 * @file iover.c
 * @date 2016. 05. 16.
 * @author kmsun
 *
 * I/O verifier implementation
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <errno.h>
#include <memory.h>
#include <assert.h>
#include <arpa/inet.h>
#include "iover.h"
#include "iogen.h"
#include "assert.h"
#include "bit.h"
#include "ansi_escape.h"
#include "rng.h"
#include "dump.h"
#include "por.h"

#define JOURNAL_HEADER_MAGIC        (0x19850830)
#define JOURNAL_FOOTER_MAGIC        (0x20160528)

#define PATTERN_ZEROES              (30)
#define PATTERN_ONES                (31)

#define ERROR_LOG(...)                                              \
    do {                                                            \
        time_t now = time(0);                                       \
        strftime(time_buf, 32, "%b %d %H:%M:%S ", localtime(&now)); \
        fprintf(stderr, "%s", time_buf);                            \
        fprintf(stderr, __VA_ARGS__);                               \
    } while(0)

enum block_state {
    block_state_unwritten = 0,
    block_state_dealloc,
    block_state_uncor,
    block_state_zeroes,
    block_state_data,
};

static const char *block_state_strings[] = {
    "UNWRITTEN", "DEALLOCATED", "UNCORRECTABLE", "ZEROES", "DATA"
};

static const char *bio_opcode_strings[] = {
    "READ", "WRITE", "FLUSH", "DEALLOCATE", "WRITE ZEROES", "WRITE UNCORRECTABLE"
};

static char time_buf[32];                                          \

static void __check_size(void)
{
    BUILD_ASSERT(sizeof (struct journal_header) == 52);
    BUILD_ASSERT(sizeof (struct journal_footer) == 32);
    BUILD_ASSERT(sizeof (struct block_desc) == 4);
    BUILD_ASSERT(sizeof (struct block_header) == 20);
    BUILD_ASSERT(sizeof (struct hist_desc) == 16);
}

static int __create_journal_file(struct iover *iover)
{
    struct journal_header header;
    struct journal_footer footer;
    char *buf;
    u64 file_byte;
    u64 remained_byte;
    int buf_byte;
    int write_byte;
    int res;
    int percentage;
    int old_percentage;
    int fd;

    buf = NULL;

    fd = open(iover->ns_info->journal_file, O_CREAT | O_RDWR, 0777);
    if (fd < 0) {
        ERROR_LOG("cannot create file '%s'\n", iover->ns_info->journal_file);
        res = fd;
        goto error;
    }

    buf_byte = 1 << 20;
    file_byte = iover->ns_info->nlbs * sizeof (struct block_desc);
    file_byte += iover->ns_info->nhist * sizeof (struct hist_desc);
    remained_byte = file_byte;

    buf = (char *)malloc(buf_byte);
    if (buf == NULL) {
        res = -1;
        goto error;
    }

    old_percentage = 0;
    printf(AE_SAVE_CURSOR);
    printf("creating journal file...   0%%");
    fflush(stdout);
    memset(&header, 0, sizeof (struct journal_header));
    header.magic = JOURNAL_HEADER_MAGIC;
    header.data_size_shift = iover->ns_info->lba_shift;
    header.meta_size = 0;
    header.nlbs = iover->ns_info->nlbs;
    header.nhist = iover->ns_info->nhist;
    header.hist_idx = 0;

    memset(&footer, 0, sizeof (struct journal_footer));
    footer.timestamp = 0;
    footer.magic = JOURNAL_FOOTER_MAGIC;

    res = write(fd, &header, sizeof (struct journal_header));
    if (res != sizeof (struct journal_header)) {
        ERROR_LOG("file write error(%d, %s)\n", res, strerror(res));
        goto error;
    }

    while (remained_byte) {
        write_byte = (int)MIN(remained_byte, buf_byte);
        res = write(fd, buf, write_byte);
        if (res != write_byte) {
            ERROR_LOG("file write error(%d, %s)\n", res, strerror(res));
            goto error;
        }
        remained_byte -= write_byte;
        percentage = 100 - (remained_byte * 100 / file_byte);
        if (percentage != old_percentage) {
            printf("\b\b\b\b%3d%%", percentage);
            fflush(stdout);
        }
    }
    res = write(fd, &footer, sizeof (struct journal_footer));
    if (res != sizeof (struct journal_footer)) {
        ERROR_LOG("file write error(%d, %s)\n", res, strerror(res));
        goto error;
    }

    printf(AE_RESTORE_CURSOR AE_CLEAR_EOL);

    if (buf) {
        free(buf);
    }
    return fd;

error:
    if (buf) {
        free(buf);
    }

    if (fd >= 0) {
        close(fd);
    }
    return res;
}

static int __open_journal_file(struct iover *iover)
{
    int fd;

    /* try to open existing file */
    fd = open(iover->ns_info->journal_file, O_RDWR);
    if (fd < 0) {
        /* file not exist, create new file */
        fd = __create_journal_file(iover);
    }

    return fd;
}

static int __create_journal_shm(struct iover *iover)
{
    struct journal_header *header;
    struct journal_footer *footer;
    u64 offset = 0;

    header = (struct journal_header *)iover->journal.base;
    header->magic = JOURNAL_HEADER_MAGIC;
    header->data_size_shift = iover->ns_info->lba_shift;
    header->meta_size = 0;
    header->nlbs = iover->ns_info->nlbs;
    header->nhist = iover->ns_info->nhist;
    header->hist_idx = 0;
    offset += sizeof (struct journal_header);

    memset(iover->journal.base + offset, 0, iover->ns_info->nlbs * sizeof (struct block_desc));
    offset += iover->ns_info->nlbs * sizeof (struct block_desc);

    memset(iover->journal.base + offset, 0xFF, iover->ns_info->nhist * sizeof (struct hist_desc));
    offset += iover->ns_info->nhist * sizeof (struct hist_desc);

    footer = (struct journal_footer *)(iover->journal.base + offset);
    memset(footer, 0, sizeof (struct journal_footer));
    footer->timestamp = 0;
    footer->magic = JOURNAL_FOOTER_MAGIC;

    return 0;
}

static int __validate_journal(struct iover *iover)
{
    struct journal_header *header;
    struct journal_footer *footer;

    header = (struct journal_header *)iover->journal.base;
    if (header->magic != JOURNAL_HEADER_MAGIC) {
        ERROR_LOG("invalid journal file format, header not found\n");
        return -1;
    }
    if (header->data_size_shift != iover->ns_info->lba_shift) {
        ERROR_LOG("expected logical block size is <%u>, but was <%u>\n",
                  (1 << header->data_size_shift), 
                  (1 << iover->ns_info->lba_shift));
        return -1;
    }
    if (header->nlbs != iover->ns_info->nlbs) {
        ERROR_LOG("expected number of lbas is <%llu>, but was <%llu>\n",
                  header->nlbs, 
                  iover->ns_info->nlbs);
        return -1;
    }
    if (header->nhist != iover->ns_info->nhist) {
        ERROR_LOG("expected number of history is <%u>, but was <%u>\n",
                  header->nhist, 
                  iover->ns_info->nhist);
        return -1;
    }

    footer = (struct journal_footer *)(iover->journal.base +
                sizeof (struct journal_header) +
                sizeof (struct block_desc) * iover->ns_info->nlbs +
                sizeof (struct hist_desc) * iover->ns_info->nhist);

    if (footer->magic != JOURNAL_FOOTER_MAGIC) {
        ERROR_LOG("invalid journal file format, footer not found\n");
        return -1;
    }
    iover->timestamp = footer->timestamp;
    iover->journal.header = header;
    iover->journal.footer = footer;

#if 0
    struct hist_desc *hist_desc = (struct hist_desc *)(iover->journal.base + sizeof (struct journal_header) + iover->ns_info->nlbs * sizeof (struct block_desc));
    u32 loop = header->nhist;
    u32 idx = header->hist_idx;

    printf("nhist %d hist_idx %d\n", loop, idx);
    
    while (loop > 0) {
        struct hist_desc *hd = hist_desc + idx;
        if (hd->lba == 0xFFFFFFFFFFFFFFFF)
            break;

        printf("%-16s: [%4d] lba=%16llu %12s\n", __func__, idx, hd->lba, block_state_strings[hd->bd.state]);

        if (idx == 0)
            idx = header->nhist - 1;
        else
            idx--;

        loop--;
    }

    // header->hist_idx = 0;
    // memset(hist_desc, 0xFF, sizeof (struct hist_desc) * iover->ns_info->nhist);
#endif

    return 0;
}

void clear_hist(struct iover *iover)
{
    struct journal_header *header;

    header = (struct journal_header *)iover->journal.base;
    header->hist_idx = 0;

    struct hist_desc *hist_desc = (struct hist_desc *)(iover->journal.base + sizeof (struct journal_header) + iover->ns_info->nlbs * sizeof (struct block_desc));
    memset(hist_desc, 0xFF, sizeof (struct hist_desc) * iover->ns_info->nhist);
}

static u64 __get_journal_size(struct iover *iover) {
    return sizeof (struct journal_header) +
        (sizeof (struct block_desc) * iover->ns_info->nlbs) + 
        (sizeof (struct hist_desc) * iover->ns_info->nhist) + 
        sizeof (struct journal_footer);
}

int iover_init(struct iover *iover, struct namespace_info *ns_info)
{
    int res;
    int seg_id;
    int created = 0;
    u64 read_size;
    u64 offset = 0;

    iover->ns_info = ns_info;
    iover->pagesize = getpagesize();

    iover->lba_lock = (ulong *)calloc(BITS_TO_LONGS(iover->ns_info->nlbs), sizeof (ulong));
    if (iover->lba_lock == NULL) {
        res = -1;
        goto error;
    }

    if (ns_info->journal_file || ns_info->journal_shm) {
        iover->journal_enabled = true;
        iover->journal.size = __get_journal_size(iover);
        if (ns_info->journal_file) {
            /* file based */
            res = __open_journal_file(iover);
            if (res < 0) {
                goto error;
            }
            iover->journal.fd = res;
            if (ns_info->journal_file_mmap) {
                iover->journal.base = mmap(NULL,
                                          iover->journal.size,
                                          PROT_READ | PROT_WRITE,
                                          MAP_SHARED,
                                          iover->journal.fd,
                                          0);
                if (iover->journal.base == MAP_FAILED) {
                    ERROR_LOG("mmap() failed(%d, %s)\n", errno, strerror(errno));
                    res = errno;
                    goto error;
                }
            } else {
                iover->journal.base = malloc(iover->journal.size);
                if (iover->journal.base == NULL) {
                    ERROR_LOG("malloc(%llu) failed(%d, %s)\n", iover->journal.size, errno, strerror(errno));
                    res = errno;
                    goto error;
                }
                while (offset < iover->journal.size) {
                    read_size = pread(iover->journal.fd, iover->journal.base + offset, iover->journal.size - offset, offset);
                    if (read_size < 0) {
                        ERROR_LOG("pread() failed(%d, %s)\n", errno, strerror(errno));
                        res = errno;
                        goto error;
                    }
                    offset += read_size;
                }
            }
        } else {
            /* shared memory based */
            u64 size = ALIGN(iover->journal.size, iover->pagesize);
            seg_id = shmget(ns_info->journal_shm, size, 0);
            if (seg_id < 0) {
                /* create a new shared memory */
                seg_id = shmget(ns_info->journal_shm, ALIGN(iover->journal.size, iover->pagesize), IPC_CREAT);
                if (seg_id < 0) {
                    ERROR_LOG("shmget() shm 0x%x size 0x%llx failed(%d, %s)\n", ns_info->journal_shm, size, errno, strerror(errno));
                    res = errno;
                    goto error;
                }
                created = 1;
            }
            iover->journal.base = shmat(seg_id, 0, 0);
            if (iover->journal.base == (void *)-1) {
                ERROR_LOG("shmat() failed(%d, %s)\n", errno, strerror(errno));
                res = errno;
                goto error;
            }
            if (created) {
                __create_journal_shm(iover);
            }
        }
        res = __validate_journal(iover);
        if (res) {
            goto error;
        }
        iover->block_desc = (struct block_desc *)(iover->journal.base + sizeof (struct journal_header));
        iover->hist_desc = (struct hist_desc *)(iover->journal.base + sizeof (struct journal_header) + iover->ns_info->nlbs * sizeof (struct block_desc));
    } else {
        iover->journal_enabled = false;
        iover->block_desc = (struct block_desc *)malloc(sizeof (struct block_desc) * iover->ns_info->nlbs);
        if (iover->block_desc == NULL) {
            res = -1;
            goto error;
        }
        memset(iover->block_desc, 0, sizeof (struct block_desc) * iover->ns_info->nlbs);

        iover->hist_desc = (struct hist_desc *)malloc(sizeof (struct hist_desc) * iover->ns_info->nhist);
        if (iover->hist_desc == NULL) {
            res = -1;
            goto error;
        }
        memset(iover->hist_desc, 0xFF, sizeof (struct hist_desc) * iover->ns_info->nhist);
    }

    res = datagen_init(&iover->datagen, iover->ns_info->data_size, ns_info->randomize_header);
    iover->datagen.meta_size = iover->ns_info->meta_size;
    if (iover->datagen.meta_size) {
        if (iover->ns_info->pi_loc == 1) {
            iover->datagen.pi_offset = 0;
        } else {
            iover->datagen.pi_offset = iover->datagen.meta_size - 8;
        }
        iover->datagen.pi_type = 1;
    }
    return res;

error:
    if (ns_info->journal_file) {
        if (iover->journal.base) {
            if (ns_info->journal_file_mmap) {
                munmap(iover->journal.base, iover->journal.size);
            } else {
                free(iover->journal.base);
            }
        }
        if (iover->journal.fd >= 0) {
            close(iover->journal.fd);
        }
    } else if (ns_info->journal_shm) {
        if (iover->journal.base) {
            shmdt(iover->journal.base);
        }
    }

    if (iover->lba_lock) {
        free(iover->lba_lock);
        iover->lba_lock = NULL;
    }

    return res;
}

void iover_set_host_writes(struct iover *iover, const u8 *host_writes)
{
    memcpy(&iover->journal.footer->host_writes, host_writes, 16);
}

void iover_get_host_writes(struct iover *iover, u8 *host_writes)
{
    memcpy(host_writes, iover->journal.footer->host_writes, 16);
}

void iover_exit(struct iover *iover)
{
    u64 write_size;
    u64 offset = 0;

    if (iover->block_desc) {
        if (iover->journal_enabled) {
            if (iover->ns_info->journal_file) {
                iover->journal.footer->timestamp = iover->timestamp;
                if (iover->ns_info->journal_file_mmap) {
                    munmap(iover->journal.base, iover->journal.size);
                } else {
                    while (offset < iover->journal.size) {
                        write_size = pwrite(iover->journal.fd, iover->journal.base + offset, iover->journal.size - offset, offset);
                        if (write_size < 0) {
                            ERROR_LOG("pwrite() failed(%d, %s)\n", errno, strerror(errno));
                        }
                        offset += write_size;
                    }
                    free(iover->journal.base);
                }
                close(iover->journal.fd);
            } else {
                shmdt(iover->journal.base);
            }
        } else {
            free(iover->block_desc);
        }
        iover->block_desc = NULL;
    }

    if (iover->lba_lock) {
        free(iover->lba_lock);
        iover->lba_lock = NULL;
    }

    datagen_exit(&iover->datagen);

    __check_size();
}

static void print_data_layout(struct iover *iover)
{
    u32 offset;

    ERROR_LOG(" byte 0h       : pattern id\n");
    ERROR_LOG(" byte 1h       : header start marker(%02xh)\n", BLOCK_HEADER_START_MARKER);
    ERROR_LOG(" byte 2h-9h    : timestamp(8bytes)\n");
    ERROR_LOG(" byte Ah-11h   : logical block address(8bytes)\n");
    ERROR_LOG(" byte 12h      : reserved\n");
    ERROR_LOG(" byte 13h      : header finish marker(%02xh)\n", BLOCK_HEADER_FINISH_MARKER);
    ERROR_LOG(" byte 14h-%3lxh : %'lu bytes of data\n",
              iover->datagen.body_size + sizeof (struct block_header) - 1,
              iover->datagen.body_size - sizeof (struct block_header));
    offset = iover->datagen.body_size + sizeof (struct block_header);
    ERROR_LOG(" byte %3xh     : pattern id\n", offset++);
    ERROR_LOG(" byte %3xh     : footer start marker(%02xh)\n", offset++, BLOCK_HEADER_START_MARKER);
    ERROR_LOG(" byte %3xh-%3xh: timestamp(8bytes)\n", offset, offset + 8);
    offset += 8;
    ERROR_LOG(" byte %3xh-%3xh: logical block address(8bytes)\n", offset, offset + 8);
    offset += 8;
    ERROR_LOG(" byte %3xh     : reserved\n", offset++);
    ERROR_LOG(" byte %3xh     : footer finish marker(%02xh)\n", offset, BLOCK_HEADER_FINISH_MARKER);
}

static void miscompare_message(struct iover *iover, struct bio *bio, u32 lba_offset, const struct block_desc *block_desc)
{
    ERROR_LOG("data miscompare(lba=%llu(%s), slba=%llu, iosize=%llu)\n",
              bio->slba + lba_offset,
              block_state_strings[block_desc->state],
              bio->slba,
              bio->iosize);
}

static u32 get_block_size(struct iover *iover, struct bio *bio)
{
    if ((bio->pract) && (iover->ns_info->meta_size == 8))  {
        /* protection information insert/strip */
        return iover->ns_info->data_size;
    } else {
        /* protection information pass */
        return iover->ns_info->block_size;
    }
}

static void miscompare_dump(struct iover *iover, struct bio *bio, u32 lba_offset, u64 timestamp, u8 pattern)
{
    u8 *expected;

    expected = malloc(iover->ns_info->data_size);
    if (expected == NULL) {
        ERROR_LOG("failed to allocate memory for data dump\n");
        return;
    }
    if (pattern == PATTERN_ZEROES) {
        memset(expected, 0x00, iover->ns_info->data_size);
    } else if (pattern == PATTERN_ONES) {
        memset(expected, 0xFF, iover->ns_info->data_size);
    } else {
        datagen_set_header(&iover->datagen, expected, bio->slba + lba_offset, timestamp, pattern);
        memcpy(expected + sizeof (struct block_header),
               iover->datagen.patterns[pattern]->body,
               iover->datagen.body_size);
    }
    ERROR_LOG(" bio->data=%p\n", bio->data);
    print_data_layout(iover);
    ERROR_LOG(" on the left : the data that was expected\n");
    ERROR_LOG(" on the right: the data that was found\n");
    ERROR_LOG(" mismatched line will be marked with '*'\n");
    dump_miscompare(stderr,
                    time_buf,
                    expected,
                    bio->data + get_block_size(iover, bio) * lba_offset,
                    iover->ns_info->data_size,
                    16);
    free(expected);
}

static void miscompare_dump_meta(struct iover *iover, const void *meta, const void *data, u64 lba)
{
    u8 *expected;

    expected = malloc(iover->ns_info->meta_size);
    if (expected == NULL) {
        ERROR_LOG("failed to allocate memory for data dump\n");
        return;
    }
    datagen_set_meta(&iover->datagen, expected, data, lba);

    ERROR_LOG(" on the left : the data that was expected\n");
    ERROR_LOG(" on the right: the data that was found\n");
    ERROR_LOG(" mismatched line will be marked with '*'\n");
    dump_miscompare(stderr,
                    time_buf,
                    expected,
                    meta,
                    iover->ns_info->meta_size,
                    8);
    free(expected);
}

static void miscompare_dump_meta_unwritten(struct iover *iover, int meta_pattern, const void *meta)
{
    u8 *expected;
    u8 pattern;

    expected = malloc(iover->ns_info->meta_size);
    if (expected == NULL) {
        ERROR_LOG("failed to allocate memory for data dump\n");
        return;
    }

    if (meta_pattern == unwr_read_value_type_zeroes) {
        pattern = 0x00;
    } else {
        pattern = 0xFF;
    }
    memset(expected, pattern, iover->ns_info->meta_size);
    if ((iover->ns_info->meta_size > 8) && (iover->ns_info->pi_type)) {
        if (iover->ns_info->pi_loc == 0) {
            /* the last 8 bytes */
            memset(expected + iover->ns_info->meta_size - 8, 0xFF, 8);
        } else {
            /* the first 8 bytes */
            memset(expected, 0xFF, 8);
        }
    }

    ERROR_LOG(" on the left : the data that was expected\n");
    ERROR_LOG(" on the right: the data that was found\n");
    ERROR_LOG(" mismatched line will be marked with '*'\n");
    dump_miscompare(stderr,
                    time_buf,
                    expected,
                    meta,
                    iover->ns_info->meta_size,
                    8);
    free(expected);
}

static int verify_block_header(struct iover *iover, struct bio *bio, u32 lba_offset, const struct block_desc *cur_block_desc, u8 *pattern)
{
    int res;
    u64 lba;
    u64 read_lba;
    u64 read_timestamp;
    const u8 *data;
    const struct block_desc *issue_block_desc;

    lba = bio->slba + lba_offset;
    issue_block_desc = &bio->block_desc[lba_offset];
    data = bio->data + get_block_size(iover, bio) * lba_offset;

    res = datagen_get_header(&iover->datagen, data, &read_lba, &read_timestamp, pattern);
    if (res == 0) {
        if (read_lba != lba) {
            /* lba mismatch */
            miscompare_message(iover, bio, lba_offset, cur_block_desc);
            ERROR_LOG(" lba mismatch detected\n");
            ERROR_LOG(" > expected <lba=%llu>, but was <lba=%llu>\n", lba, read_lba);
            miscompare_dump(iover, bio, lba_offset, cur_block_desc->timestamp, cur_block_desc->pattern);
            return -1;
        }
        if ((u16)read_timestamp != cur_block_desc->timestamp) {
            if (bio->job->params->lba_overlap) {
                if (issue_block_desc->timestamp <= cur_block_desc->timestamp) {
                    if (((u16)read_timestamp >= issue_block_desc->timestamp) &&
                        ((u16)read_timestamp <= cur_block_desc->timestamp)) {
                        return 0;
                    }
                } else {
                    /* timestamp wrapped around */
                    if (!(((u16)read_timestamp < issue_block_desc->timestamp) &&
                          ((u16)read_timestamp > cur_block_desc->timestamp + U16_MAX))) {
                        return 0;
                    }
                }
            }

            miscompare_message(iover, bio, lba_offset, cur_block_desc);
            ERROR_LOG(" timestamp mismatch detected\n");
            if (bio->job->params->lba_overlap) {
                ERROR_LOG(" > expected <timestamp=%'u-%'u>, ", issue_block_desc->timestamp, cur_block_desc->timestamp);
            } else {
                ERROR_LOG(" > expected <timestamp=%'u>, ", cur_block_desc->timestamp);
            }
            fprintf(stderr, "but was <timestamp=%'u(%'llu)>\n", (u16)read_timestamp, read_timestamp);
            miscompare_dump(iover, bio, lba_offset, cur_block_desc->timestamp, cur_block_desc->pattern);
            return -1;
        }
    } else {
        /* header not exist */
        miscompare_message(iover, bio, lba_offset, cur_block_desc);
        ERROR_LOG(" header not found, offset 0x%llx\n", (u64)(ulong)data);
        miscompare_dump(iover, bio, lba_offset, cur_block_desc->timestamp, cur_block_desc->pattern);
        return -1;
    }

    return 0;
}

static int verify_block_body(struct iover *iover, struct bio *bio, u32 lba_offset, const struct block_desc *cur_block_desc, u8 pattern)
{
    int res;
    u64 read_lba;
    u64 read_timestamp;
    const u8 *data;

    data = bio->data + get_block_size(iover, bio) * lba_offset;
    res = memcmp(data + sizeof (struct block_header),
                 iover->datagen.patterns[pattern]->body,
                 iover->datagen.body_size);
    if (res) {
        datagen_get_header(&iover->datagen, data, &read_lba, &read_timestamp, &pattern);
        miscompare_message(iover, bio, lba_offset, cur_block_desc);
        ERROR_LOG(" data pattern mismatch detected\n");
        miscompare_dump(iover, bio, lba_offset, read_timestamp, pattern);
        res = -1;
    }

    return res;
}

static int verify_block_meta(struct iover *iover, struct bio *bio, u32 lba_offset)
{
    int res;
    u64 read_lba;
    u64 read_timestamp;
    u8 pattern;
    const u8 *data;
    const u8 *meta;
    u8 expected[64];

    data = bio->data + get_block_size(iover, bio) * lba_offset;
    if (iover->ns_info->extended_lba) {
        meta = data + iover->ns_info->data_size;
    } else {
        meta = bio->meta + iover->ns_info->meta_size * lba_offset;
    }
    res = datagen_get_header(&iover->datagen, data, &read_lba, &read_timestamp, &pattern);
    if (res) {
        ERROR_LOG("datagen_get_header() failed(%d)\n", res);
        ERROR_LOG("controller %s, nsid %u, bio opcode %d, lba %llu\n", bio->job->params->controller, bio->job->params->nsid, bio->opcode, bio->slba + lba_offset);
    }

    res = datagen_compare_meta(&iover->datagen, meta, data, bio->slba + lba_offset);
    if (res) {
        ERROR_LOG("datagen_compare_meta() failed(%d)\n", res);
        ERROR_LOG("controller %s, nsid %u, bio opcode %d, lba %llu, bio->data 0x%p, data size %llu, bio->meta 0x%p, meta size %llu, actual data buffer 0x%p\n",
                bio->job->params->controller, bio->job->params->nsid, bio->opcode, bio->slba + lba_offset, bio->data, bio->data_size, bio->meta, bio->meta_size, data);
        datagen_set_meta(&iover->datagen, &expected, data, bio->slba + lba_offset);
        miscompare_dump_meta(iover, expected, meta, bio->slba + lba_offset);
    }

    return res;
}

static int verify_block_data(struct iover *iover, struct bio* bio, u32 lba_offset, const struct block_desc *cur_block_desc)
{
    int res;
    u8 pattern;

    res = verify_block_header(iover, bio, lba_offset, cur_block_desc, &pattern);
    if (res == 0) {
        if (!bio->job->params->compare_header_only) {
            res = verify_block_body(iover, bio, lba_offset, cur_block_desc, pattern);
        }
    }

    return res;
}

static bool is_all(const u8 *data, u8 val, u32 size)
{
    u32 offset;
    for (offset = 0; offset < size; offset++) {
        if (*data != val) {
            return false;
        }
    }
    return true;
}

static bool is_all_ones(const u8 *data, u32 size)
{
    return is_all(data, 0xFF, size);
}

static bool is_all_zeroes(const u8 *data, u32 size)
{
    return is_all(data, 0x00, size);
}

static int identify_dealloc_block(struct iover *iover, struct bio *bio, u32 lba_offset, struct block_desc *block_desc)
{
    u32 data_size;
    const u8 *data;
    int res;
    u8 pattern;
    u64 lba;
    u64 read_lba;
    u64 read_timestamp;

    data_size = iover->ns_info->data_size;
    data = bio->data + get_block_size(iover, bio) * lba_offset;

    if (bio->job->params->dealloc_read_value_type & dealloc_read_value_type_zeroes) {
        res = is_all_zeroes(data, data_size);
        if (res == true) {
            return dealloc_read_value_type_zeroes;
        }
    }
    if (bio->job->params->dealloc_read_value_type & dealloc_read_value_type_ones) {
        res = is_all_ones(data, data_size);
        if (res == true) {
            return dealloc_read_value_type_ones;
        }
    }

    if (bio->job->params->dealloc_read_value_type & dealloc_read_value_type_last) {
        lba = bio->slba + lba_offset;
        res = datagen_get_header(&iover->datagen, data, &read_lba, &read_timestamp, &pattern);
        if (res == 0) {
            if ((read_lba == lba) && (read_timestamp == block_desc->timestamp)) {
                res = verify_block_body(iover, bio, lba_offset, block_desc, pattern);
                if (res == 0) {
                    return dealloc_read_value_type_last;
                }
            }
            return -1;
        }
    }

    miscompare_message(iover, bio, lba_offset, block_desc);
    ERROR_LOG(" unidentified data from deallocated lba(nsid=%u, lba=%llu, slba=%llu, iosize=%u)\n", bio->job->params->nsid, bio->slba + lba_offset, bio->slba, data_size * bio->nlbs);
    ERROR_LOG(" > the values read from a deallocated lba shall be");
    if (bio->job->params->dealloc_read_value_type == dealloc_read_value_type_zeroes) {
        fprintf(stderr, " all zeroes\n");
    }
    if (bio->job->params->dealloc_read_value_type == dealloc_read_value_type_ones) {
        fprintf(stderr, " all ones\n");
    }
    if (bio->job->params->dealloc_read_value_type == dealloc_read_value_type_last) {
        fprintf(stderr, " the last data written to the assocated lba\n");
    }
    dump_data(stderr, time_buf, data, data_size);

    return -1;
}

static int verify_write_zeroes_block(struct iover *iover, struct bio* bio, u32 lba_offset, struct block_desc *cur_block_desc)
{
    u32 data_size;
    const u8 *data;
    int res;

    data_size = iover->ns_info->data_size;
    data = bio->data + get_block_size(iover, bio) * lba_offset;

    res = is_all_zeroes(data, data_size);
    if (!res) {
        miscompare_message(iover, bio, lba_offset, cur_block_desc);
        ERROR_LOG(" > expected all zeroes\n");
        miscompare_dump(iover, bio, lba_offset, 0, PATTERN_ZEROES);
        return -1;
    }

    return 0;
}

static int verify_dealloc_block(struct iover *iover, struct bio* bio, u32 lba_offset, struct block_desc *cur_block_desc)
{
    u32 data_size;
    const u8 *data;
    int res;

    data_size = iover->ns_info->data_size;
    data = bio->data + get_block_size(iover, bio) * lba_offset;

    switch (bio->job->params->dealloc_read_value_type) {
        case dealloc_read_value_type_last:
            res = verify_block_data(iover, bio, lba_offset, cur_block_desc);
            break;
        case dealloc_read_value_type_zeroes:
            res = is_all_zeroes(data, data_size);
            if (!res) {
                miscompare_message(iover, bio, lba_offset, cur_block_desc);
                ERROR_LOG(" > expected all zeroes\n");
                miscompare_dump(iover, bio, lba_offset, 0, PATTERN_ZEROES);
                return -1;
            }
            break;
        case dealloc_read_value_type_ones:
            res = is_all_ones(data, data_size);
            if (!res) {
                miscompare_message(iover, bio, lba_offset, cur_block_desc);
                ERROR_LOG(" > expected all ones\n");
                miscompare_dump(iover, bio, lba_offset, 0, PATTERN_ONES);
                return -1;
            }
            break;
        default:
            /* it's first read of deallocated block, ns_info read data */
            res = identify_dealloc_block(iover, bio, lba_offset, cur_block_desc);
            if (res > 0) {
                cur_block_desc->pattern = (u8)res;
            }
            break;
    }

    return 0;
}

const u8 *get_metadata(struct iover *iover, struct bio *bio, u32 lba_offset)
{
    if (iover->ns_info->meta_size == 0) {
        return NULL;
    }
    if ((bio->pract) && (iover->ns_info->meta_size == 8)) {
        return NULL;
    }

    if (iover->ns_info->extended_lba) {
        return bio->data + get_block_size(iover, bio) * lba_offset + iover->ns_info->data_size;
    } else {
        return bio->meta + iover->ns_info->meta_size * lba_offset;
    }
}

static int verify_unwritten_block(struct iover *iover, struct bio *bio, u32 lba_offset, struct block_desc *cur_block_desc)
{
    u32 data_size;
    const u8 *data;
    const u8 *meta;
    const u8 *pi;
    int res;

    data_size = iover->ns_info->data_size;
    data = bio->data + get_block_size(iover, bio) * lba_offset;

    if (bio->job->params->unwr_read_result_type == unwr_read_result_type_value) {
        switch (bio->job->params->unwr_read_value_type) {
            case unwr_read_value_type_zeroes:
                res = is_all_zeroes(data, data_size);
                if (!res) {
                    miscompare_message(iover, bio, lba_offset, cur_block_desc);
                    ERROR_LOG(" > expected all zeroes\n");
                    miscompare_dump(iover, bio, lba_offset, 0, PATTERN_ZEROES);
                    return -1;
                }
                meta = get_metadata(iover, bio, lba_offset);
                if (meta) {
                    if (iover->ns_info->pi_type) {
                        if (iover->ns_info->pi_loc == 0) {
                            pi = meta + iover->ns_info->meta_size - 8;
                        } else {
                            pi = meta;
                        }
                        res = is_all_ones(pi, 8);
                        if (!res) {
                            miscompare_message(iover, bio, lba_offset, cur_block_desc);
                            ERROR_LOG(" > protection information shall be all bytes set to FFh\n");
                            miscompare_dump_meta_unwritten(iover, unwr_read_value_type_zeroes, meta);
                            return -1;
                        }
                        if (iover->ns_info->meta_size > 8) {
                            if (iover->ns_info->pi_loc == 0) {
                                res = is_all_zeroes(meta, iover->ns_info->meta_size - 8);
                            } else {
                                res = is_all_zeroes(meta + 8, iover->ns_info->meta_size - 8);
                            }
                            if (!res) {
                                miscompare_message(iover, bio, lba_offset, cur_block_desc);
                                ERROR_LOG(" > metadata(excluding protection information) shall be all bytes set to 00h\n");
                                miscompare_dump_meta_unwritten(iover, unwr_read_value_type_zeroes, meta);
                                return -1;
                            }
                        }
                    } else {
                        res = is_all_zeroes(data, data_size);
                        if (!res) {
                            miscompare_message(iover, bio, lba_offset, cur_block_desc);
                            ERROR_LOG(" > metadata shall be all bytes set to 00h\n");
                            miscompare_dump_meta_unwritten(iover, unwr_read_value_type_zeroes, meta);
                            return -1;
                        }
                    }
                }
                break;
            case unwr_read_value_type_ones:
                res = is_all_ones(data, data_size);
                if (!res) {
                    miscompare_message(iover, bio, lba_offset, cur_block_desc);
                    ERROR_LOG(" > expected all ones\n");
                    miscompare_dump(iover, bio, lba_offset, 0, PATTERN_ONES);
                    return -1;
                }
                break;
            default:
                /* it's first read of unwritten block, ns_info read data */
                if (bio->job->params->unwr_read_value_type == unwr_read_value_type_zeroes) {
                    res = is_all_zeroes(data, data_size);
                    if (res) {
                        cur_block_desc->pattern = unwr_read_value_type_zeroes;
                        break;
                    }
                }
                if (bio->job->params->unwr_read_value_type == unwr_read_value_type_ones) {
                    res = is_all_ones(data, data_size);
                    if (res) {
                        cur_block_desc->pattern = unwr_read_value_type_ones;
                        break;
                    }
                }
                ERROR_LOG(" unidentified data from unwritten lba(nsid=%u, lba=%llu, slba=%llu, iosize=%u)\n", bio->job->params->nsid, bio->slba + lba_offset, bio->slba, get_block_size(iover, bio) * bio->nlbs);
                ERROR_LOG(" > the values read from a unwritten lba shall be");
                if (bio->job->params->unwr_read_value_type == unwr_read_value_type_zeroes) {
                    fprintf(stderr, " all zeroes\n");
                }
                if (bio->job->params->unwr_read_value_type == unwr_read_value_type_ones) {
                    fprintf(stderr, " all ones\n");
                }
                dump_data(stderr, time_buf, data, data_size);
                return -1;
        }
    }

    return 0;
}

/* check if the lba is written by write uncorrectable */
static int check_read_error(struct iover *iover, struct bio *bio)
{
    u64 lba_offset;
    u64 lba;
    struct block_desc *issue_block_desc;
    struct block_desc cur_block_desc;

    issue_block_desc = bio->block_desc;

    for (lba_offset = 0; lba_offset < bio->nlbs; lba_offset++) {
        if (bio->job->params->lba_overlap) {
            if (issue_block_desc[lba_offset].state == block_state_unwritten) {
                /* don't care */
                continue;
            }
        }

        lba = bio->slba + lba_offset;
        cur_block_desc = iover->block_desc[lba];

        if (cur_block_desc.state == block_state_uncor) {
            return -1;
        }
    }

    return 0;

}

static int verify_bio_read(struct iover *iover, struct bio *bio)
{
    u64 lba_offset;
    u64 lba;
    u32 data_size;
    int final_res = 0;
    int res;
    struct block_desc *issue_block_desc;
    struct block_desc cur_block_desc;

    data_size = iover->ns_info->data_size;
    issue_block_desc = bio->block_desc;

    for (lba_offset = 0; lba_offset < bio->nlbs; lba_offset++) {
        if (bio->job->params->lba_overlap) {
            if (issue_block_desc[lba_offset].state == block_state_unwritten) {
                /* don't care */
                continue;
            }
        }

        lba = bio->slba + lba_offset;
        cur_block_desc = iover->block_desc[lba];

        switch (cur_block_desc.state) {
            case block_state_unwritten:
                res = verify_unwritten_block(iover, bio, lba_offset, &cur_block_desc);
                if (res && !final_res) {
                    final_res = res;
                    fprintf(stderr, "unwritten lba(nsid=%u, lba=%llu, slba=%llu, nlbs=%u)\n", bio->job->params->nsid, lba, bio->slba, bio->nlbs);
                }
                break;
            case block_state_dealloc:
                res = verify_dealloc_block(iover, bio, lba_offset, &cur_block_desc);
                if (res && !final_res) {
                    final_res = res;
                    fprintf(stderr, "dealloc lba(nsid=%u, lba=%llu, slba=%llu, nlbs=%u)\n", bio->job->params->nsid, lba, bio->slba, bio->nlbs);
                }
                break;
            case block_state_uncor:
                if (bio->result == 0) {
                    ERROR_LOG("read success on the lba that was marked as %s(nsid=%u, lba=%llu, slba=%llu, nlbs=%u)\n",
                              block_state_strings[cur_block_desc.state],
                              bio->job->params->nsid, 
                              lba,
                              bio->slba,
                              bio->nlbs);
                    print_data_layout(iover);
                    dump_data(stderr, time_buf, bio->data + get_block_size(iover, bio) * lba_offset, data_size);
                }
                break;
            case block_state_data:
                res = verify_block_data(iover, bio, lba_offset, &cur_block_desc);
                if ((res == 0) && (iover->ns_info->meta_size > 0)) {
                    if (!((bio->pract) && (iover->ns_info->meta_size == 8))) {
                        res = verify_block_meta(iover, bio, lba_offset);
                    }
                }
                if (res && !final_res) {
                    final_res = res;
                    fprintf(stderr, "data mismatch(nsid=%u, lba=%llu, slba=%llu, nlbs=%u)\n", bio->job->params->nsid, lba, bio->slba, bio->nlbs);
                }
                break;
            case block_state_zeroes:
                res = verify_write_zeroes_block(iover, bio, lba_offset, &cur_block_desc);
                break;
        }
    }

    return final_res;
}

#define BD_START        BD_NEXT

static int verify_hist_read(struct iover *iover, struct bio *bio)
{
    u64 lba_offset;
    u64 lba;
    u32 data_size;
    int final_res = 0;
    int res[2] = { 0, };
    int bd_idx;

    struct hist_desc *hd;
    struct block_desc *bd;

    if (verify_bio_read(iover, bio) == 0)
        return 0;

    data_size = iover->ns_info->data_size;

    for (lba_offset = 0; lba_offset < bio->nlbs; lba_offset++) {
        lba = bio->slba + lba_offset;

        hd = iover->hist_desc + bio->hist_idx;

        // for (bd_idx = 0; bd_idx < NUM_BD_HISTORY; bd_idx++) {
        for (bd_idx = BD_START; bd_idx < NUM_BD_HISTORY; bd_idx++) {
            bd = &hd->bd[bd_idx];

            switch (bd->state) {
                case block_state_unwritten:
                    res[bd_idx] = verify_unwritten_block(iover, bio, lba_offset, bd);
                    break;
                case block_state_dealloc:
                    res[bd_idx] = verify_dealloc_block(iover, bio, lba_offset, bd);
                    break;
                case block_state_uncor:
                    ERROR_LOG("read success on the lba that was marked as %s(nsid=%u, lba=%llu, slba=%llu, nlbs=%u)\n",
                            block_state_strings[bd->state],
                            bio->job->params->nsid, 
                            lba,
                            bio->slba,
                            bio->nlbs);
                    print_data_layout(iover);
                    dump_data(stderr, time_buf, bio->data + get_block_size(iover, bio) * lba_offset, data_size);
                    break;
                case block_state_data:
                    res[bd_idx] = verify_block_data(iover, bio, lba_offset, bd);
                    if ((res == 0) && (iover->ns_info->meta_size > 0)) {
                        if (!((bio->pract) && (iover->ns_info->meta_size == 8))) {
                            res[bd_idx] = verify_block_meta(iover, bio, lba_offset);
                        }
                    }
                    break;
                case block_state_zeroes:
                    res[bd_idx] = verify_write_zeroes_block(iover, bio, lba_offset, bd);
                    break;
            }

            // forward fixing
            if (res[bd_idx] == 0)
                memcpy(&iover->block_desc[lba], &hd->bd[bd_idx], sizeof (struct block_desc));
        }

        u8 fail_cnt = BD_START;
        for (bd_idx = BD_START; bd_idx < NUM_BD_HISTORY; bd_idx++) {
            if (res[bd_idx] != 0)
                fail_cnt++;
        }

        if (fail_cnt == NUM_BD_HISTORY) {
            final_res = -1;
            ERROR_LOG("*** REPLAY HISTORY VERIFY ERROR *** lba %llu\n", lba);
        }
    }

    if (final_res != -1)
        ERROR_LOG("*** REPLAY HISTORY VERIFY OK *** lba %llu\n", lba);
        
    return final_res;
}

static int enqueue(struct iover *iover, struct bio *bio)
{
    return 0;
}

/* bitmap-based lock */
static int try_lock_lbas(struct iover *iover, struct bio *bio)
{
    ulong unlock;
    ulong lock;
    ulong lock_mask;
    u32 lba_offset = 0;
    u32 ulong_offset;
    u32 bit_offset;
    u32 bit_length;
    int res;

    ulong_offset = bio->slba / BITS_PER_LONG;
    while (lba_offset < bio->nlbs) {
        bit_offset = (bio->slba + lba_offset) % BITS_PER_LONG;
        bit_length = MIN(BITS_PER_LONG - bit_offset, bio->nlbs - lba_offset);
        lock_mask = GET_MASK64(bit_offset, bit_length);

        do {
            unlock = iover->lba_lock[ulong_offset];
            if ((unlock & lock_mask) != 0) {
                /* failed to lock */
                goto rollback;
            }
            lock = unlock | lock_mask;

            res = __sync_bool_compare_and_swap(&iover->lba_lock[ulong_offset], unlock, lock);
        } while (!res);

        lba_offset += bit_length;
        ulong_offset++;
    }
    return 0;
rollback:
    while (lba_offset > 0) {
        ulong_offset--;
        bit_length = MIN(BITS_PER_LONG, lba_offset);
        bit_offset = BITS_PER_LONG - bit_length;
        lock_mask = GET_MASK64(bit_offset, bit_length);

        do {
            lock = iover->lba_lock[ulong_offset];
            unlock = lock & ~lock_mask;
            res = __sync_bool_compare_and_swap(&iover->lba_lock[ulong_offset], lock, unlock);
        } while (!res);

        lba_offset -= bit_length;
    }
    return -1;
}

static void unlock_lbas(struct iover *iover, struct bio *bio)
{
    ulong unlock;
    ulong lock;
    ulong lock_mask;
    u32 lba_offset = 0;
    u32 ulong_offset;
    u32 bit_offset;
    u32 bit_length;
    int res;

    ulong_offset = bio->slba / BITS_PER_LONG;
    while (lba_offset < bio->nlbs) {
        bit_offset = (bio->slba + lba_offset) % BITS_PER_LONG;
        bit_length = MIN(BITS_PER_LONG - bit_offset, bio->nlbs - lba_offset);
        lock_mask = GET_MASK64(bit_offset, bit_length);

        do {
            lock = iover->lba_lock[ulong_offset];
            unlock = lock & ~lock_mask;

            res = __sync_bool_compare_and_swap(&iover->lba_lock[ulong_offset], lock, unlock);
        } while (!res);

        lba_offset += bit_length;
        ulong_offset++;
    }
}

int iover_generate_data(struct iover *iover, struct bio *bio)
{
    u64 lba_offset;
    u64 lba;
    u8 *data;
    u8 *meta;
    u8 pattern;
    int res;
    struct pr_info *pr_info;
    u8 pr_info_buf[8];
    u32 block_size;

    if ((!bio->job->params->lba_overlap) && (bio->opcode < bio_identify)) {
        res = try_lock_lbas(iover, bio);
        if (res) {
            return res;
        }
    }
    if ((iover->ns_info->pi_type) && (iover->ns_info->meta_size)) {
        pr_info = (struct pr_info *)pr_info_buf;
        datagen_set_pi(&iover->datagen, pr_info, bio->slba);
        bio->apptag = (u16)ntohs(pr_info->app);
        bio->reftag = (u32)ntohl(pr_info->ref);
    }

    if (bio->opcode == bio_read) {
        if ((bio->job->params->compare) && (bio->job->params->lba_overlap)) {
            /* store command issue-time lba descriptors */
            memcpy(bio->block_desc, &iover->block_desc[bio->slba], sizeof (struct block_desc) * bio->nlbs);
        }
    } else if (bio->opcode == bio_write) {
        if (bio->job->params->compare) {
            iover->timestamp++;
            block_size = get_block_size(iover, bio);
            for (lba_offset = 0; lba_offset < bio->nlbs; lba_offset++) {
                lba = bio->slba + lba_offset;
                data = bio->data + block_size * lba_offset;
                pattern = (iover->timestamp + lba) % iover->datagen.npatterns;
                datagen_set_data(&iover->datagen, data, lba, iover->timestamp, pattern);
                if (iover->ns_info->meta_size) {
                    if (!((bio->pract) && (iover->ns_info->meta_size == 8))) {
                        if (iover->ns_info->extended_lba) {
                            meta = data + iover->ns_info->data_size;
                        } else {
                            meta = bio->meta + iover->ns_info->meta_size * lba_offset;
                        }
                        datagen_set_meta(&iover->datagen, meta, data, lba);
                        if ((rand() % 10) == 0) {
                            if (iover->ns_info->pi_loc) {
                                pr_info = (struct pr_info *)meta;
                            } else {
                                pr_info = (struct pr_info *)(meta + iover->ns_info->meta_size - 8);
                            }
                            pr_info->app = 0xFFFF;
                            if (iover->ns_info->pi_type == 3) {
                                pr_info->ref = 0xFFFFFFFF;
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}

static void commit_block_state(struct iover *iover, struct bio *bio)
{
    u8 pattern;
    u32 lba_offset;
    u64 lba;
    u64 read_lba;
    u64 read_timestamp;
    char *data;
    int res;
    u32 block_size;

    switch (bio->opcode) {
        case bio_write:
            if (bio->job->params->compare) {
                block_size = get_block_size(iover, bio);
                for (lba_offset = 0; lba_offset < bio->nlbs; lba_offset++) {
                    lba = bio->slba + lba_offset;
                    data = bio->data + block_size * lba_offset;
                    res = datagen_get_header(&iover->datagen, data, &read_lba, &read_timestamp, &pattern);
                    if (res < 0) {
                        ERROR_LOG("datagen_get_header() failed(%d)\n", res);
                        ERROR_LOG("controller %s, nsid %u, bio opcode %d, lba %llu, bio->data 0x%p, data 0x%p, block size %u, pract %u\n", bio->job->params->controller, bio->job->params->nsid, bio->opcode, lba, bio->data, data, block_size, bio->pract);
                        dump_data(stderr, time_buf, bio->data, bio->data_size);
                        exit(0);
                    } else {
                        iover->block_desc[lba].state = block_state_data;
                        iover->block_desc[lba].pattern = pattern;
                        iover->block_desc[lba].timestamp = read_timestamp;
                    }
                }
            }
            break;
        case bio_deallocate:
            if (bio->job->params->compare) {
                block_size = get_block_size(iover, bio);
                for (lba_offset = 0; lba_offset < bio->nlbs; lba_offset++) {
                    lba = bio->slba + lba_offset;
                    iover->block_desc[lba].state = block_state_dealloc;
                }
            }
            break;
        case bio_write_zeroes:
            if (bio->job->params->compare) {
                block_size = get_block_size(iover, bio);
                for (lba_offset = 0; lba_offset < bio->nlbs; lba_offset++) {
                    lba = bio->slba + lba_offset;
                    iover->block_desc[lba].state = block_state_zeroes;
                }
            }
            break;
        case bio_write_uncorrect:
            if (bio->job->params->compare) {
                block_size = get_block_size(iover, bio);
                for (lba_offset = 0; lba_offset < bio->nlbs; lba_offset++) {
                    lba = bio->slba + lba_offset;
                    iover->block_desc[lba].state = block_state_uncor;
                }
            }
            break;
        default:
            break;
    }
}

#if (SUPPORT_SPOR_LEVEL_3 == 1)
void commit_block_state_unwritten(struct iover *iover, struct bio *bio)
{
    u8 pattern;
    u32 lba_offset;
    u64 lba;
    u64 read_lba;
    u64 read_timestamp;
    char *data;
    int res;
    u32 block_size;

    if (bio->job->params->compare) {
        block_size = get_block_size(iover, bio);
        for (lba_offset = 0; lba_offset < bio->nlbs; lba_offset++) {
            lba = bio->slba + lba_offset;
            data = bio->data + block_size * lba_offset;
            res = datagen_get_header(&iover->datagen, data, &read_lba, &read_timestamp, &pattern);
            if (res < 0) {
                ERROR_LOG("datagen_get_header() failed(%d)\n", res);
                ERROR_LOG("controller %s, nsid %u, bio opcode %d, lba %llu, bio->data 0x%p, data 0x%p, block size %u, pract %u\n", bio->job->params->controller, bio->job->params->nsid, bio->opcode, lba, bio->data, data, block_size, bio->pract);
                dump_data(stderr, time_buf, bio->data, bio->data_size);
                exit(0);
            } else {
//                iover->block_desc[lba].state = block_state_data;
                iover->block_desc[lba].state = block_state_unwritten;
                iover->block_desc[lba].pattern = pattern;
                iover->block_desc[lba].timestamp = read_timestamp;
            }
        }
    }
}
#endif

void backup_hist_state(struct iover *iover, struct bio *bio)
{
    u8 pattern;
    u32 lba_offset;
    u64 lba;
    u64 read_lba;
    u64 read_timestamp;
    char *data;
    int res;
    u32 block_size;

    struct journal_header *jh = iover->journal.header;
    struct hist_desc *hd;
    struct block_desc *bd;

    if (bio->job->params->history == 0 || bio->job->params->compare == 0)
        return;

    switch (bio->opcode) {
        case bio_write:
            block_size = get_block_size(iover, bio);
            for (lba_offset = 0; lba_offset < bio->nlbs; lba_offset++) {
                lba = bio->slba + lba_offset;
                data = bio->data + block_size * lba_offset;
                res = datagen_get_header(&iover->datagen, data, &read_lba, &read_timestamp, &pattern);
                if (res < 0) {
                    ERROR_LOG("datagen_get_header() failed(%d)\n", res);
                    ERROR_LOG("controller %s, nsid %u, bio opcode %d, lba %llu, bio->data 0x%p, data 0x%p, block size %u, pract %u\n", bio->job->params->controller, bio->job->params->nsid, bio->opcode, lba, bio->data, data, block_size, bio->pract);
                    dump_data(stderr, time_buf, bio->data, bio->data_size);
                    exit(0);
                }

                hd = iover->hist_desc + jh->hist_idx;
                hd->lba = lba;

                bd = iover->block_desc;
                memcpy(&hd->bd[BD_CURR], bd, sizeof (struct block_desc));

                bd = &hd->bd[BD_NEXT];
                bd->state = block_state_data;
                bd->pattern = pattern;
                bd->timestamp = read_timestamp;

                jh->hist_idx++;
                jh->hist_idx %= jh->nhist;
            }
            break;
        case bio_deallocate:
            block_size = get_block_size(iover, bio);
            for (lba_offset = 0; lba_offset < bio->nlbs; lba_offset++) {
                lba = bio->slba + lba_offset;

                hd = iover->hist_desc + jh->hist_idx;
                hd->lba = lba;

                bd = iover->block_desc;
                memcpy(&hd->bd[BD_CURR], bd, sizeof (struct block_desc));

                bd = &hd->bd[BD_NEXT];
                bd->state = block_state_dealloc;

                jh->hist_idx++;
                jh->hist_idx %= jh->nhist;
            }
            break;
        case bio_write_zeroes:
            block_size = get_block_size(iover, bio);
            for (lba_offset = 0; lba_offset < bio->nlbs; lba_offset++) {
                lba = bio->slba + lba_offset;

                hd = iover->hist_desc + jh->hist_idx;
                hd->lba = lba;

                bd = iover->block_desc;
                memcpy(&hd->bd[BD_CURR], bd, sizeof (struct block_desc));

                bd = &hd->bd[BD_NEXT];
                bd->state = block_state_zeroes;

                jh->hist_idx++;
                jh->hist_idx %= jh->nhist;
            }
            break;
        case bio_write_uncorrect:
            block_size = get_block_size(iover, bio);
            for (lba_offset = 0; lba_offset < bio->nlbs; lba_offset++) {
                lba = bio->slba + lba_offset;

                hd = iover->hist_desc + jh->hist_idx;
                hd->lba = lba;

                bd = iover->block_desc;
                memcpy(&hd->bd[BD_CURR], bd, sizeof (struct block_desc));

                bd = &hd->bd[BD_NEXT];
                bd->state = bio_write_uncorrect;

                jh->hist_idx++;
                jh->hist_idx %= jh->nhist;
            }
            break;
        default:
            return;
    }
}

int iover_check_written(struct iover *iover, struct bio *bio)
{
    u64 lba_offset;
    u64 lba;
    struct block_desc cur_block_desc;

    for (lba_offset = 0; lba_offset < bio->nlbs; lba_offset++) {
        lba = bio->slba + lba_offset;
        cur_block_desc = iover->block_desc[lba];

        switch (cur_block_desc.state) {
            case block_state_uncor:
            case block_state_data:
            case block_state_zeroes:
                return 1;
        }
    }
    return 0;
}

int iover_submit(struct iover *iover, struct bio *bio)
{
    int res = 0;

    if (!bio->job->params->lba_overlap) {
        unlock_lbas(iover, bio);
    }

    if ((bio->result != bio->expected_result) && (check_read_error(iover, bio) == 0) && iover_check_written(iover, bio)) {
        ERROR_LOG("[%s] %s(nsid=%u, slba=%llu, iosize=%u) unexpected return value(%d, %s, expected %d)\n",
                  bio->job->display_name,
                  bio_opcode_strings[bio->opcode],
                  bio->job->params->nsid, 
                  bio->slba,
                  iover->ns_info->data_size * bio->nlbs,
                  bio->result,
                  strerror(-bio->result),
                  bio->expected_result);

        if (bio->job->params->stop_on_error) {
            return -1;
        } else {
            res = 0;
        }
    }

    if ((bio->result != 0) || bio->job->params->compare == 0) {
        return 0;
    }

    if (bio->opcode == bio_identify) {
        if (bio->job->identify_valid) {
            res = memcmp(bio->job->identify, bio->data_buf, 4096);
            if (res) {
                bio->result = res;
                fprintf(stderr, "expected:\n");
                dump_data(stderr, "  ", bio->job->identify, 4096);
                fprintf(stderr, "but was:\n");
                dump_data(stderr, "  ", bio->data_buf, 4096);
            }
        } else {
            memcpy(bio->job->identify, bio->data_buf, 4096);
            bio->job->identify_valid = 1;
        }
        return res;
    }
    if (bio->opcode != bio_read) {
        commit_block_state(iover, bio);
    }

    /* verify read data */
    if (bio->opcode == bio_read) {
        if (iover->nthreads > 0) {
            return enqueue(iover, bio);
        }

        if (bio->hist_idx != 0xFFFFFFFF)
            res = verify_hist_read(iover, bio);
        else
            res = verify_bio_read(iover, bio);
        if (res) {
            if (!bio->job->params->stop_on_miscompare) {
                res = 0;
                bio->job->iogen->result = 1;
            }
        }
    }

    return res;
}

