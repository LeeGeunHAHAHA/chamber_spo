/**
 * @file dump.c
 * @date 2016. 06. 12.
 * @author kmsun
 */

#include <memory.h>
#include "dump.h"

static void dump_line(FILE *file, const char *format, const u8 *data, u32 width)
{
    u32 offset;

    for (offset = 0; offset < width; offset++) {
        fprintf(file, format, data[offset]);
        if ((offset + 1) % 4 == 0) {
            fprintf(file, " ");
        }
    }
}

void dump_data(FILE *file, const char *prefix, const u8 *data, u32 bytes)
{
    u32 offset;
    u32 width = 16;

    if (prefix) {
        fprintf(file, "%s", prefix);
    }
    fprintf(file, "offset 00 01 02 03  04 05 06 07  08 09 0A 0B  0C 0D 0E 0F\n");

    for (offset = 0; offset < bytes; offset += width) {
        if (prefix) {
            fprintf(file, "%s", prefix);
        }
        fprintf(file, " %04X:", offset);
        dump_line(file, " %02x", &data[offset], width);
        fprintf(file, "\n");
    }
}

void dump_miscompare(FILE *file, const char *prefix, const u8 *expected, const u8 *actual, u32 bytes, u32 width)
{
    u32 offset;
    u32 i;
    int res;

    if (prefix) {
        fprintf(file, "%s", prefix);
    }
    fprintf(file, "offset  ");
    for (i = 0; i < width; i++) {
        fprintf(file, "%-2X", i);
        if ((i + 1) % 4 == 0) {
            fprintf(file, " ");
        }
    }
    fprintf(file, " ");
    for (i = 0; i < width; i++) {
        fprintf(file, "%-2X", i);
        if ((i + 1) % 4 == 0) {
            fprintf(file, " ");
        }
    }
    fprintf(file, "\n");

    for (offset = 0; offset < bytes; offset += width) {
        if (prefix) {
            fprintf(file, "%s", prefix);
        }
        res = memcmp(&expected[offset], &actual[offset], width);
        if (res != 0) {
            fprintf(file, "*%04X: ", offset);
        } else {
            fprintf(file, " %04X: ", offset);
        }
        dump_line(file, "%02x", &expected[offset], width);
        fprintf(file, " ");
        dump_line(file, "%02x", &actual[offset], width);
        fprintf(file, "\n");
    }
}

