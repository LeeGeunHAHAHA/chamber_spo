/**
 * @file log.c
 * @date 2016. 05. 24.
 * @author kmsun
 */

#include <stdio.h>
#include "nvme.h"
#include "log.h"

extern iotrace_writer_t g_iotrace_writer;

int iolog_create(struct iolog_writer *writer, const char *file, struct iogen *iogen)
{
    int job_idx;
    char *name;

    writer->timestamp = 0;
    writer->njobs = iogen->njobs;
    writer->file = fopen(file, "w+");
    if (writer->file == NULL) {
        return -1;
    }

    fprintf(writer->file, "timestamp");
    for (job_idx = 0; job_idx < writer->njobs; job_idx++) {
        name = iogen->jobs[job_idx].params->name;
        fprintf(writer->file, ", %s read throughput", name);
        fprintf(writer->file, ", %s read iops", name);
        fprintf(writer->file, ", %s read latency", name);

        fprintf(writer->file, ", %s write throughput", name);
        fprintf(writer->file, ", %s write iops", name);
        fprintf(writer->file, ", %s write latency", name);
    }
    fprintf(writer->file, "\n");

    return 0;
}

int iolog_append(struct iolog_writer *writer, const struct ioacct *accts)
{
    const struct ioacct *acct;
    int job_idx;

    fprintf(writer->file, "%llu", ++writer->timestamp);
    for (job_idx = 0; job_idx < writer->njobs; job_idx++) {
        acct = &accts[job_idx + 1];
        fprintf(writer->file, ", %llu", acct->ops[bio_read].bytes);
        fprintf(writer->file, ", %llu", acct->ops[bio_read].ops);
        if (acct->ops[bio_read].ops) {
            fprintf(writer->file, ", %llu", acct->ops[bio_read].latency.sum / acct->ops[bio_read].ops / 1000);
        } else {
            fprintf(writer->file, ", 0");
        }

        fprintf(writer->file, ", %llu", acct->ops[bio_write].bytes);
        fprintf(writer->file, ", %llu", acct->ops[bio_write].ops);
        if (acct->ops[bio_write].ops) {
            fprintf(writer->file, ", %llu", acct->ops[bio_write].latency.sum / acct->ops[bio_write].ops / 1000);
        } else {
            fprintf(writer->file, ", 0");
        }
    }
    fprintf(writer->file, "\n");

    return 0;
}

int iolog_append_line(struct iolog_writer *writer, const char *line)
{
    fputs(line, writer->file);
    fprintf(writer->file, "\n");

    return 0;
}

void iolog_close(struct iolog_writer *writer)
{
    fclose(writer->file);
}

int smartlog_create(struct smartlog_writer *writer, const char *file)
{
    writer->timestamp = 0;
    writer->file = fopen(file, "w+");
    if (writer->file == NULL) {
        return -1;
    }

    fprintf(writer->file,
            "timestamp, critical warning, composite temperature, "
            "available spare, available spare threshold, percentage used, "
            "data units read, data units written, host read commands, "
            "host write commands, controller busy time, power cycles, "
            "power on hours, media and data integrity erros, "
            "number of error information log entires, "
            "warning composite temperature time, "
            "critical composite temperature time, "
            "temperature sensor 1, temperature sensor 2, "
            "temperature sensor 3, temperature sensor 4, "
            "temperature sensor 5, temperature sensor 6, "
            "temperature sensor 7, temperature sensor 8\n");

    return 0;
}

static long double int128_to_double(const u8 *data)
{
	int i;
	long double result = 0;

	for (i = 0; i < 16; i++) {
		result *= 256;
		result += data[15 - i];
	}
	return result;
}

int smartlog_append(struct smartlog_writer *writer, const nvme_smart_log_t *log)
{
    fprintf(writer->file, "%llu", ++writer->timestamp);
    fprintf(writer->file, ", %u", log->critical_warning);
    fprintf(writer->file, ", %u", log->composite_temperature);
    fprintf(writer->file, ", %u", log->avail_spare);
    fprintf(writer->file, ", %u", log->spare_thresh);
    fprintf(writer->file, ", %u", log->percent_used);
    fprintf(writer->file, ", %.0Lf", int128_to_double(log->data_units_read));
    fprintf(writer->file, ", %.0Lf", int128_to_double(log->data_units_written));
    fprintf(writer->file, ", %.0Lf", int128_to_double(log->host_reads));
    fprintf(writer->file, ", %.0Lf", int128_to_double(log->host_writes));
    fprintf(writer->file, ", %.0Lf", int128_to_double(log->ctrl_busy_time));
    fprintf(writer->file, ", %.0Lf", int128_to_double(log->power_cycles));
    fprintf(writer->file, ", %.0Lf", int128_to_double(log->power_on_hours));
    fprintf(writer->file, ", %.0Lf", int128_to_double(log->unsafe_shutdowns));
    fprintf(writer->file, ", %.0Lf", int128_to_double(log->media_errors));
    fprintf(writer->file, ", %.0Lf", int128_to_double(log->num_err_log_entries));
    fprintf(writer->file, ", %u", log->wc_temp_time);
    fprintf(writer->file, ", %u", log->cc_temp_time);
    fprintf(writer->file, ", %u", log->temperatures[0]);
    fprintf(writer->file, ", %u", log->temperatures[1]);
    fprintf(writer->file, ", %u", log->temperatures[2]);
    fprintf(writer->file, ", %u", log->temperatures[3]);
    fprintf(writer->file, ", %u", log->temperatures[4]);
    fprintf(writer->file, ", %u", log->temperatures[5]);
    fprintf(writer->file, ", %u", log->temperatures[6]);
    fprintf(writer->file, ", %u", log->temperatures[7]);
    fprintf(writer->file, "\n");

    return 0;
}

void smartlog_close(struct smartlog_writer *writer)
{
    fclose(writer->file);
}

int iotrace_create(struct iotrace_writer *writer, const char *file)
{
    writer->file = fopen(file, "w+");
    if (writer->file == NULL) {
        return -1;
    }

    fprintf(writer->file, "cmd slba nlbs pattern\n");

    return 0;
}

int iotrace_append(const struct bio *bio)
{
    u8 *data = (u8 *)bio->data;
    fprintf(g_iotrace_writer.file, "%c %llu %u %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x", \
        (bio->opcode == bio_read) ? 'r':'w', bio->slba, bio->nlbs, \
            data[0], data[1], data[2], data[3], \
            data[4], data[5], data[6], data[7], \
            data[8], data[9], data[10], data[11]);
    
    fprintf(g_iotrace_writer.file, "\n");

    return 0;
}

void iotrace_close(struct iotrace_writer *writer)
{
    fclose(writer->file);
}



