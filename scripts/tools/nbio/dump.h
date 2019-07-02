/**
 * @file dump.h
 * @date 2016. 06. 12.
 * @author kmsun
 */
#ifndef __NBIO_DUMP_H__
#define __NBIO_DUMP_H__

#include <stdio.h>
#include "types.h"

void dump_data(FILE *file, const char *prefix, const u8 *data, u32 bytes);
void dump_miscompare(FILE *file, const char *prefix, const u8 *expected, const u8 *actual, u32 bytes, u32 width);

#endif /* __NBIO_DUMP_H__ */

