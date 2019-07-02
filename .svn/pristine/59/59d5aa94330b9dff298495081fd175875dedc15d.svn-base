/**
 * @file standalone.c
 * @date 2016. 05. 31.
 * @author kmsun
 *
 * stand-alone application display
 */

#include <stdio.h>
#include "display.h"
#include "bit.h"
#include "ansi_escape.h"
#include "timespec_sub.h"

static const char *op_strings[] = { "READ", "WRITE", "FLUSH", "DEALLOCATE", "WRITE ZEROES", "WRITE UNCORRECT" };

static void to_human_readable(u64 size, int kilo_unit, u64 *num, const char **metric)
{
    const char *metrics[] = {
        "", "K", "M", "G", "T", "P"
    };
    int i;
    int threshold = kilo_unit * kilo_unit * 10 / 100;   /* k^2 x 10% */

    *num = size;
    for (i = 0; i < 5; i++) {
        if (*num  < threshold) {
            break;
        }
        *num /= kilo_unit;
    }
    *metric = metrics[i];
}

int standalone_disp_init(struct display *disp)
{
    disp->ctx = NULL;
    return 0;
}

static void print_io_acct(const struct opacct *opacct)
{
    u64 num;
    const char *metric;

    if (opacct->ops == 0) {
        printf("-");
    } else {
        /* throughput */
        to_human_readable(opacct->bytes, 1024, &num, &metric);
        printf("%'6llu%sB", num, metric);
        /* iops */
        to_human_readable(opacct->ops, 1000, &num, &metric);
        printf("/%'6llu%s", num, metric);
        /* average ratency */
        num = (opacct->ops > 0 ? opacct->latency.sum / opacct->ops : 0);
        printf("/%'3lluus", num / 1000);
    }
}

static void print_sum_acct(struct iogen *iogen, const struct ioacct *ioacct)
{
    u32 eta;
    int op_idx;
    int num_running_jobs;

    num_running_jobs = popcount64(iogen->running_job_mask);
    printf(AE_CLEAR_LINE);
    printf("\r %u/%u jobs running    ", num_running_jobs, iogen->njobs);
    eta = iogen_eta(iogen);

    if (num_running_jobs > 1) {
        for (op_idx = 0; op_idx < bio_max; op_idx++) {
            if (ioacct->ops[op_idx].ops > 0) {
                printf(" [%s ", op_strings[op_idx]);
                print_io_acct(&ioacct->ops[op_idx]);
                printf( "]");
            }
        }
    }

    if (eta != U32_MAX) {
        printf("(eta %02um:%02us)", eta / 60, eta % 60);
    }

    fflush(stdout);
}

static void print_job_acct(struct iogen_job *job, const struct ioacct *ioacct)
{
    int op_idx;

    printf(AE_CLEAR_LINE);
    if (job->id < job->iogen->njobs) {
        printf("\r ├─ %d.%-15s", job->id, job->display_name);
    } else {
        /* last job */
        printf("\r └─ %d.%-15s", job->id, job->display_name);
    }

    switch (job->state) {
        case iogen_job_state_done:
            printf(" (done)");
            break;
        case iogen_job_state_aborted:
            printf(" (aborted)");
            break;
        case iogen_job_state_waiting:
            printf(" (waiting)");
            break;
        case iogen_job_state_reset:
            printf(" (reset)");
            break;
        default:
            for (op_idx = 0; op_idx < bio_max; op_idx++) {
                if (ioacct->ops[op_idx].ops > 0) {
                    printf(" [%s ", op_strings[op_idx]);
                    print_io_acct(&ioacct->ops[op_idx]);
                    printf("]");
                }
            }
            break;
    }

    fflush(stdout);
}

void standalone_disp_update(struct display *disp, const struct ioacct *ioaccts)
{
    int job_idx;

    if (disp->ctx != NULL) {
        printf(AE_MOVE_UP_CURSOR, disp->iogen->njobs);
    }

    print_sum_acct(disp->iogen, &ioaccts[0]);
    for (job_idx = 0; job_idx < disp->iogen->njobs; job_idx++) {
        printf("\n");
        print_job_acct(&disp->iogen->jobs[job_idx], &ioaccts[job_idx + 1]);
    }
    disp->ctx = (void *)1;
}

static void report_opacct(const struct opacct *opacct, u64 elapsed_time_ns)
{
    u64 num;
    u64 longnum;
    const char *metric;

    to_human_readable(opacct->bytes, 1024, &num, &metric);
    printf("amount=%llu%sB", num, metric);
    to_human_readable(opacct->ops, 1000, &num, &metric);
    printf("(%llu%sio)", num, metric);

    /* throughput */
    longnum = opacct->bytes;
    longnum *= NSEC_PER_SEC;
    longnum /= elapsed_time_ns;
    to_human_readable((u64)longnum, 1024, &num, &metric);
    printf(", throughput=%llu%sB/s", num, metric);

    /* iops */
    longnum = opacct->ops;
    longnum *= NSEC_PER_SEC;
    longnum /= elapsed_time_ns;
    to_human_readable((u64)longnum, 1000, &num, &metric);
    printf(", iops=%llu%s\n", num, metric);
}

static void report_lat_acct_stat(const struct opacct *opacct)
{
    u64 avg;

    avg = (opacct->ops ? opacct->latency.sum / opacct->ops : 0);
    printf("avg=%llu, min=%llu, max=%llu\n", avg / 1000, opacct->latency.min / 1000, opacct->latency.max / 1000);
}

static void report_lat_acct_percentile(const struct opacct *opacct)
{
    int bucket_idx;
    const struct latency_acct_bucket *buckets;
    double percent;
    u64 count = 0;
    int isfirst = 1;

    buckets = opacct->latency.buckets;

    percent = 0.99;
    bucket_idx = 0;
    while (percent < 0.9999999) {
        if (count >= (u64)(opacct->ops * percent)) {
            if (isfirst) {
                isfirst = 0;
            } else {
                printf(", ");
            }
            printf("%lg%%=[%5llu]", percent * 100, buckets[bucket_idx - 1].sum / buckets[bucket_idx - 1].count / 1000);
            percent += 9;
            percent *= 0.1;
            continue;
        }
        count += buckets[bucket_idx].count;
        bucket_idx++;
    }
    printf("\n");
}

static void report_lat_acct_distribution(const struct opacct *opacct)
{
    int bucket_idx;
    const struct latency_acct_bucket *buckets;
    int isfirst = 1;
    float percent;
    u64 avg_ns;

    buckets = opacct->latency.buckets;
    for (bucket_idx = 0; bucket_idx < LATENCY_ACCT_NUM_BUCKETS; bucket_idx++) {
        percent = ((float)buckets[bucket_idx].count * 100 / opacct->ops);
        if (percent) {
            if (isfirst) {
                isfirst = 0;
            } else {
                printf(", ");
            }
            avg_ns = buckets[bucket_idx].sum / buckets[bucket_idx].count;
            printf("%llu=%f%%", avg_ns / 1000, percent);
        }
    }
    printf("\n");
}

static void report_acct_op(int opcode, const char *opname, const char *prefix, u64 elapsed_time_ns, const struct ioacct *ioacct)
{
    const struct opacct *opacct;

    opacct = &ioacct->ops[opcode];
    if (opacct->ops != 0) {
        printf("%s- %-5s: ", prefix, opname);
        report_opacct(opacct, elapsed_time_ns);
        printf("%s   latency stat(us): ", prefix);
        report_lat_acct_stat(opacct);
        printf("%s   percentiles(us) : ", prefix);
        report_lat_acct_percentile(opacct);
        printf("%s   distribution(us): ", prefix);
        report_lat_acct_distribution(opacct);
    }
}

static void report_acct(struct display *disp, const char *prefix, u64 elapsed_time_ns, const struct ioacct *ioacct)
{
    report_acct_op(bio_read,  "read",  prefix, elapsed_time_ns, ioacct);
    report_acct_op(bio_write, "write", prefix, elapsed_time_ns, ioacct);
    report_acct_op(bio_flush, "flush", prefix, elapsed_time_ns, ioacct);
}

void standalone_disp_report(struct display *disp, const struct ioacct *ioaccts)
{
    int job_idx;
    u64 elapsed_time_ns;
    struct iogen_job *job;

    printf("\n");

    elapsed_time_ns = timespec_sub(&disp->iogen->end_ts, &disp->iogen->begin_ts);
    printf("* total summary: runtime=%.02lfs\n", (double)elapsed_time_ns / NSEC_PER_SEC);
    report_acct(disp, " ", elapsed_time_ns, &ioaccts[0]);

    if (disp->iogen->njobs >= 2) {
        printf("* per-job summary:\n");
        for (job_idx = 0; job_idx < disp->iogen->njobs; job_idx++) {
            job = &disp->iogen->jobs[job_idx];
            elapsed_time_ns = timespec_sub(&job->end_ts, &job->begin_ts);
            printf(" > job '%s': runtime=%.02lfs\n", job->params->name, (double)elapsed_time_ns / NSEC_PER_SEC);
            report_acct(disp, "  ", elapsed_time_ns, &ioaccts[job_idx + 1]);
        }
    }
}

void standalone_disp_exit(struct display *disp)
{
}

static struct display_ops standalone_disp = {
    .name = "standalone",
    .init = standalone_disp_init,
    .update = standalone_disp_update,
    .report = standalone_disp_report,
    .exit = standalone_disp_exit,
};

void __attribute__((constructor)) standalone_disp_register(void)
{
    display_register(&standalone_disp);
}

void __attribute__((destructor)) standalone_disp_unregister(void)
{
    display_unregister(&standalone_disp);
}

