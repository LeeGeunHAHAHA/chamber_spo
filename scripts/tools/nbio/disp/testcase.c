/**
 * @file testcase.c
 * @date 2016. 05. 31.
 * @author kmsun
 *
 * testcase display
 */

#include <stdio.h>
#include "display.h"
#include "ansi_escape.h"

static const char *op_strings[] = { "R", "W", "F", "D", "Z", "U" };

int testcase_disp_init(struct display *disp)
{
    printf(AE_SAVE_CURSOR);
    fflush(stdout);

    return 0;
}

static void size_to_human_readable(unsigned long long size, int kilo_unit, unsigned long long *num, const char **metric)
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

static void time_to_human_readable(unsigned long long us, unsigned long long *num, const char **metric)
{
    const char *metrics[] = {
        "us", "ms", "s"
    };
    int i;
    int threshold = 100000;

    *num = us;
    for (i = 0; i < countof (metrics) - 1; i++) {
        if (*num < threshold) {
            break;
	}
        *num /= 1000;
    }
    *metric = metrics[i];
}

static void print_opacct(const struct opacct *opacct)
{
    unsigned long long num;
    const char *metric;

    if (opacct->ops == 0) {
        printf("-");
    } else {
        /* throughput */
        size_to_human_readable(opacct->bytes, 1024, &num, &metric);
        printf("%'llu%sB", num, metric);
#if 0
        /* iops */
        to_human_readable(opacct->ops, 1000, &num, &metric);
        printf("/%'llu%s", num, metric);
#endif
        /* average ratency */
        num = (opacct->ops > 0 ? opacct->latency.sum / opacct->ops : 0);
	time_to_human_readable(num / 1000, &num, &metric);
        printf("/%'llu%s", num, metric);
    }
}

void testcase_disp_update(struct display *disp, const struct ioacct *ioaccts)
{
    int op_idx;
    u32 eta;

    printf(AE_RESTORE_CURSOR AE_CLEAR_EOL);
    eta = iogen_eta(disp->iogen);
    if (eta != U32_MAX ) {
        printf("(eta %02um:%02us)", eta / 60, eta % 60);
    }

    for (op_idx = 0; op_idx < bio_max; op_idx++) {
        if (ioaccts[0].ops[op_idx].ops > 0) {
            printf("[%s ", op_strings[op_idx]);
            print_opacct(&ioaccts[0].ops[op_idx]);
            printf("]");
        }
    }
    fflush(stdout);
}

void testcase_disp_report(struct display *disp, const struct ioacct *ioaccts)
{
    printf(AE_RESTORE_CURSOR AE_CLEAR_EOL);
    fflush(stdout);
}

void testcase_disp_exit(struct display *disp)
{
}

static struct display_ops testcase_disp = {
    .name = "testcase",
    .init = testcase_disp_init,
    .update = testcase_disp_update,
    .report = testcase_disp_report,
    .exit = testcase_disp_exit,
};

void __attribute__((constructor)) testcase_disp_register(void)
{
    display_register(&testcase_disp);
}

void __attribute__((destructor)) testcase_disp_unregister(void)
{
    display_unregister(&testcase_disp);
}

