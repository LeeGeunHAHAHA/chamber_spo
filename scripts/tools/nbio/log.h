/**
 * @file log.h
 * @date 2016. 05. 24.
 * @author kmsun
 */
#ifndef __NBIO_LOG_H__
#define __NBIO_LOG_H__

#include "iogen.h"
#include "acct.h"

struct iolog_writer {
    int njobs;
    u64 timestamp;
    FILE *file;
};

struct smartlog_writer {
    u64 timestamp;
    FILE *file;
};

typedef struct iotrace_writer {
    FILE *file;
} iotrace_writer_t;

int iolog_create(struct iolog_writer *writer, const char *file, struct iogen *iogen);
int iolog_append(struct iolog_writer *writer, const struct ioacct *accts);
int iolog_append_line(struct iolog_writer *writer, const char *line);
void iolog_close(struct iolog_writer *writer);

int smartlog_create(struct smartlog_writer *writer, const char *file);
int smartlog_append(struct smartlog_writer *writer, const nvme_smart_log_t *log);
void smartlog_close(struct smartlog_writer *writer);

int iotrace_create(struct iotrace_writer *writer, const char *file);
int iotrace_append(const struct bio *bio);
void iotrace_close(struct iotrace_writer *writer);


#endif /* __NBIO_LOG_H__ */

