/**
 * @file datagen.h
 * @date 2016. 06. 04.
 * @author kmsun
 *
 * block data generator
 */
#ifndef __NBIO_DATAGEN_H__
#define __NBIO_DATAGEN_H__

#include "block.h"

struct PACKED datagen_pattern {
    u8 header_mask[sizeof (struct block_header)];
    u8 *body[0];
};

struct datagen {
    int npatterns;
    struct datagen_pattern **patterns;
    u32 data_size;
    u32 body_size;
    u32 meta_size;
    u32 pi_offset;
    int pi_type;
    bool randomize_header;
};

int datagen_init(struct datagen *datagen, u32 data_size, bool randomize_header);
void datagen_exit(struct datagen *datagen);
int datagen_store(struct datagen *datagen, int fd);
int datagen_load(struct datagen *datagen, int fd);
int datagen_get_header(struct datagen *datagen, const void *data, u64 *lba, u64 *timestamp, u8 *pattern);
void datagen_set_header(struct datagen *datagen, void *data, u64 lba, u64 timestamp, u8 pattern);
void datagen_set_body(struct datagen *datagen, void *data, u8 pattern);
void datagen_set_data(struct datagen *datagen, void *data, u64 lba, u64 timestamp, u8 pattern);
int datagen_set_pi(struct datagen *datagen, struct pr_info *pr_info, u64 lba);
int datagen_set_meta(struct datagen *datagen, void *metadata, const void *data, u64 lba);
int datagen_compare_meta(struct datagen *datagen, const void *metadata, const void *data, u64 lba);

#endif /* __NBIO_DATAGEN_H__ */

