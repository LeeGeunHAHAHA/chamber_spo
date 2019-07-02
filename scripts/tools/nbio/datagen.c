/**
 * @file datagen.c
 * @date 2016. 06. 04.
 * @author kmsun
 *
 * block data generator
 */

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "datagen.h"
#include "rng.h"
#include "crc_t10dif.h"

#define NUM_PATTERNS            (8)
#define DATAGEN_RNG_SEED        (0x19850830)
#define PI_SIZE                 (8)

static void create_body_data_pattern(struct datagen *datagen, struct rng *rng)
{
    int pattern_idx;
    u32 offset;
    u8 *bytes;
    u16 *words;

    /* synthetic bit patterns */
    memset(datagen->patterns[0]->body, 0x00, datagen->body_size);    /* all zeroes */
    memset(datagen->patterns[1]->body, 0x55, datagen->body_size);    /* zero and one */
    memset(datagen->patterns[2]->body, 0xAA, datagen->body_size);    /* one and zero */
    memset(datagen->patterns[3]->body, 0xFF, datagen->body_size);    /* all ones */

    /* increasing values */
    bytes = (u8 *)datagen->patterns[4]->body;
    for (offset = 0; offset < datagen->body_size; offset++) {
        bytes[offset] = offset;
    }
    words = (u16 *)datagen->patterns[5]->body;
    for (offset = 0; offset < (datagen->body_size >> 1); offset++) {
        words[offset] = offset;
    }

    /* decreasing values */
    bytes = (u8 *)datagen->patterns[6]->body;
    for (offset = 0; offset < datagen->body_size; offset++) {
        bytes[offset] = datagen->body_size - offset - 1;
    }
    words = (u16 *)datagen->patterns[7]->body;
    for (offset = 0; offset < (datagen->body_size >> 1); offset++) {
        words[offset] = datagen->body_size - offset - 1;
    }

    /* random bit patterns */
    for (pattern_idx = 8; pattern_idx < datagen->npatterns; pattern_idx++) {
        rng_fill(rng, datagen->patterns[pattern_idx]->body, datagen->body_size);
    }
}

int datagen_init(struct datagen *datagen, u32 data_size, bool randomize_header)
{
    u8 *patterns;
    int pattern_idx;
    struct rng rng;

    datagen->npatterns = NUM_PATTERNS;
    datagen->patterns = (struct datagen_pattern **)calloc(datagen->npatterns, sizeof (long));
    if (datagen->patterns == NULL) {
        return -1;
    }
    patterns = (u8 *)calloc(datagen->npatterns, data_size);
    if (patterns == NULL) {
        free(datagen->patterns);
        datagen->patterns = NULL;
        return -1;
    }

    rng_init(&rng, DATAGEN_RNG_SEED);
    datagen->data_size = data_size;
    datagen->body_size = data_size - (sizeof (struct block_header) * 2);
    datagen->randomize_header = randomize_header;

    /* create block header masks for randomization */
    for (pattern_idx = 0; pattern_idx < datagen->npatterns; pattern_idx++) {
        datagen->patterns[pattern_idx] = (struct datagen_pattern *)&patterns[pattern_idx * data_size];
        if (randomize_header) {
            rng_fill(&rng, datagen->patterns[pattern_idx]->header_mask, sizeof (struct block_header));
        }
    }

    /* create data pattern for block body */
    create_body_data_pattern(datagen, &rng);

    return 0;
}

void datagen_exit(struct datagen *datagen)
{
    if (datagen->patterns) {
        if (datagen->patterns[0]) {
            free(datagen->patterns[0]);
        }
        free(datagen->patterns);
        datagen->patterns = NULL;
    }
    datagen->npatterns = 0;
}

static void randomize_header(const struct block_header *src, struct block_header *dst, const u8 *mask)
{
    const u8 *byte_src;
    u8 *byte_dst;
    int offset;

    byte_src = (const u8 *)src;
    byte_dst = (u8 *)dst;

    *byte_dst++ = *byte_src++;  /* pattern field should not be randomized */
    mask++;
    for (offset = 1; offset < sizeof (struct block_header); offset++) {
        *byte_dst++ = *byte_src++ ^ *mask++;
    }
}

static void derandomize_header(const struct block_header *src, struct block_header *dst, const u8 *mask)
{
    randomize_header(src, dst, mask);
}

int datagen_get_header(struct datagen *datagen, const void *data, u64 *lba, u64 *timestamp, u8 *pattern)
{
    struct block_header *randomized = (struct block_header *)data;
    struct block_header header;
    struct block_header footer;
    int res;

    if (randomized->pattern >= datagen->npatterns) {
        fprintf(stderr, "pattern range error <%u>\n", randomized->pattern);
        return -1;
    }

    if (datagen->randomize_header) {
        derandomize_header(randomized,
                           &header,
                           datagen->patterns[randomized->pattern]->header_mask);
        derandomize_header((struct block_header *)(data + datagen->data_size - sizeof (struct block_header)),
                           &footer,
                           datagen->patterns[randomized->pattern]->header_mask);
    } else {
        memcpy(&header, randomized, sizeof (struct block_header));
        memcpy(&footer, data + datagen->data_size - sizeof (struct block_header), sizeof (struct block_header));
    }

    if ((header.start_marker != BLOCK_HEADER_START_MARKER) ||
        (header.finish_marker != BLOCK_HEADER_FINISH_MARKER)) {
        fprintf(stderr, "marker error! start marker <%02x>, finish marker <%02x>\n", header.start_marker,header.finish_marker);
        return -1;
    }

    res = memcmp(&header, &footer, sizeof (struct block_header));
    if (res) {
        fprintf(stderr, "header and footer mismatch\n");
        return -1;
    }

    *lba = header.lba;
    *timestamp = header.timestamp;
    *pattern = header.pattern;

    return 0;
}

void datagen_set_header(struct datagen *datagen, void *data, u64 lba, u64 timestamp, u8 pattern)
{
    struct block_header header;

    header.pattern = pattern;
    header.start_marker = BLOCK_HEADER_START_MARKER;
    header.lba = lba;
    header.timestamp = timestamp;
    header.finish_marker = BLOCK_HEADER_FINISH_MARKER;
    header.rsvd = 0;

    if (datagen->randomize_header) {
        randomize_header(&header,
                         (struct block_header *)data,
                         datagen->patterns[pattern]->header_mask);
        randomize_header(&header,
                         (struct block_header *)(data + datagen->data_size - sizeof (struct block_header)),
                         datagen->patterns[pattern]->header_mask);
    } else {
        memcpy(data, &header, sizeof (struct block_header));
        memcpy(data + datagen->data_size - sizeof (struct block_header), &header, sizeof (struct block_header));
    }
}

void datagen_set_body(struct datagen *datagen, void *data, u8 pattern)
{
    memcpy(data + sizeof (struct block_header),
           datagen->patterns[pattern]->body,
           datagen->body_size);
}

void datagen_set_data(struct datagen *datagen, void *data, u64 lba, u64 timestamp, u8 pattern)
{
    datagen_set_header(datagen, data, lba, timestamp, pattern);
    datagen_set_body(datagen, data, pattern);
}

int datagen_set_pi(struct datagen *datagen, struct pr_info *pr_info, u64 lba)
{
    switch (datagen->pi_type) {
        case 1:
            /* Type 1: Reference tag is LBA */
            pr_info->ref = (u32)htonl(lba);
            pr_info->app = 0xFAD0;
            break;
        case 2:
            /* Type 2: Any value can be initial reference tag */
            pr_info->ref = (u32)htonl(lba << 1);
            pr_info->app = 0xFAD0;
            break;
        case 3:
            /* Type 3: Reference tag is used as part of the application tag */
            pr_info->ref = (u32)lba;
            pr_info->app = (u16)lba;
            break;
        default:
            return -1;
    }
    return 0;
}

int datagen_set_meta(struct datagen *datagen, void *metadata, const void *data, u64 lba)
{
    struct pr_info *prinfo;

    prinfo = (struct pr_info *)(metadata + datagen->pi_offset);

    datagen_set_pi(datagen, prinfo, lba);
    prinfo->guard = htons(crc_t10dif_generic(0, data, datagen->data_size));

    if (datagen->meta_size > PI_SIZE) {
        /* metadata size > 8 */
        if (datagen->pi_offset) {
            /* protection information transferred as the last 8bytes of metadata */
            memcpy(metadata, data, datagen->pi_offset);
            prinfo->guard = htons(crc_t10dif_generic(ntohs(prinfo->guard), metadata, datagen->pi_offset));
        } else {
            /* protection information transferred as the first 8bytes of metadata */
            memcpy(metadata + PI_SIZE, data, datagen->meta_size - PI_SIZE);
        }
    }

    return 0;
}

int datagen_compare_meta(struct datagen *datagen, const void *metadata, const void *data, u64 lba)
{
    union {
        struct pr_info pi;
        u8 raw_data[64];
    } expect;
    struct pr_info *actual;
    int res;

    actual = (struct pr_info *)(metadata + datagen->pi_offset);

    switch (datagen->pi_type) {
        case 1:
        case 2:
            if (actual->app == 0xFFFF) {
                /* Do not check */
                return 0;
            }
            break;
        case 3:
            if ((actual->app == 0xFFFF) && (actual->ref == 0xFFFFFFFF)) {
                /* Do not check */
                return 0;
            }
            break;
        default:
            return -1;
    }
    datagen_set_meta(datagen, &expect, data, lba);
    res = memcmp(&expect, metadata, datagen->meta_size);
    return res;
}

