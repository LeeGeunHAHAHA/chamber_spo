#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "types.h"
#include "testsuite.h"

#define TEST_CONFIG_NAME    "Configuration"
#define LOG_FILENAME        "test.log"
#define MAX_TEST_DEPTH      (8)
#define MAX_NR_SKIP         (32)
#define COLOR_ENABLED       (0)

/* ANSI Escape Codes */
#if (COLOR_ENABLED == 1)
#define AE_FG_RED           "\033[31;1m"
#define AE_FG_GREEN         "\033[32;1m"
#define AE_FG_DEFAULT       "\033[0m"
#else
#define AE_FG_RED
#define AE_FG_GREEN
#define AE_FG_DEFAULT
#endif

#define INDENT_INIT()       do { __test_log_indent = 0; } while(0)
#define INDENT_SET(x)       do { __test_log_indent = (x); } while(0)
#define INDENT_INC()        do { __test_log_indent++; } while(0)

typedef u32 testno_t[MAX_TEST_DEPTH];

static testcase_t *__first_tc;
static testcase_t *__last_tc;
//static testcase_t *__skip_tc[MAX_NR_SKIP];
u32 __nr_skip_tc;

int __test_log_indent;
int __test_res;
static u32 nr_fails;
static u32 nr_passes;
FILE* __test_log_file;

char *__test_gettime(void)
{
    struct timeval tv;
    struct tm *tm;
    static const char mon_name[][4] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    };
    static char result[23];

    gettimeofday(&tv, NULL);
    tm = localtime(&tv.tv_sec);

    /* syslog style time format */
    sprintf(result, "%.3s%3d %.2d:%.2d:%.2d %06d",
            mon_name[tm->tm_mon],
            tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)tv.tv_usec);
    return result;
}

static int __get_test_depth(testno_t testno)
{
    u32 i;
    for (i = 0; i < MAX_TEST_DEPTH; i++) {
        if (testno[i] == 0) {
            if (i > 0) {
                return i - 1;
            }
            return i;
        }
    }
    return MAX_TEST_DEPTH;
}

static void __run_testcase(testno_t testno, const testcase_t *tc)
{
    u32 depth;

    depth = __get_test_depth(testno);
    if (tc->name == TEST_CONFIG) {
        TEST_LOG("Test Case %u. %s:", testno[depth], TEST_CONFIG_NAME);
        printf("%3u. %-*s\t... ", testno[depth], 70 - (depth * 2), TEST_CONFIG_NAME);
    } else {
        TEST_LOG("Test Case %u. %s:", testno[depth], tc->name);
        printf("%3u. %-*s\t... ", testno[depth], 70 - (depth * 2), tc->name);
    }
    INDENT_INC();
    fflush(stdout);
    if (__test_res != __TESTGROUP_FAIL) {
        __test_res = 0;
        tc->func();
    }
    if ((__test_res == __TESTCASE_FAIL) || (__test_res == __TESTGROUP_FAIL)) {
        printf(AE_FG_RED"fail"AE_FG_DEFAULT"\n");
        nr_fails++;
    } else if (__test_res == __TESTCASE_IGNORE) {
        printf("ignore\n");
    } else {
        TEST_LOG("-> success");
        printf(AE_FG_GREEN"pass"AE_FG_DEFAULT"\n");
        nr_passes++;
    }
    TEST_LOG("");
    fflush(__test_log_file);
}

static void __run_config(testno_t testno, const testcase_t *tc)
{
    u32 depth;
    testno_t testno_config;
    const testcase_t *tc_config;

    depth = __get_test_depth(testno);
    if (testno[depth] > 1) {
        tc_config = tc - (testno[depth] - 1);
        if (tc_config->name == TEST_CONFIG) {
            INDENT_SET(depth);
            if (depth > 0) {
                printf("%*s", depth * 2, " ");
            }
            memcpy(testno_config, testno, sizeof (testno_t));
            testno_config[depth] = 1;
            __run_testcase(testno_config, tc_config);
        }
    }
}

static int __run_testcases(testno_t testno, const testcase_t *tc)
{
    u32 depth;
    int res = 0;

    depth = __get_test_depth(testno);
    __test_res = 0;

    __run_config(testno, tc);
    if (__test_res) {
        /* Configuration failed, stop test */
        return -1;
    }

    for (; tc->name != NULL; testno[depth]++, tc++) {
        if (tc == __last_tc) {
            res = 1;
            break;
        }
        INDENT_SET(depth);
        if (depth > 0) {
            printf("%*s", depth * 2, " ");
        }
        if (tc->func) {
            __run_testcase(testno, tc);
            if (__test_res) {
            //if ((tc->name == TEST_CONFIG) && (__test_res)) {
                /* Configuration failed, stop test */
                res = -1;
                break;
            }
        } else if (tc->testcases) {
            TEST_LOG("Test Group %u. %s", testno[depth], tc->name);
            printf("%u. %s\n", testno[depth], tc->name);

            testno[depth + 1] = 1;
            res = __run_testcases(testno, tc->testcases);
            if (res) {
                break;
            }
            INDENT_SET(depth);
        }
    }
    testno[depth] = 0;

    return res;
}

void __show_summary(void)
{
    u32 nr_testcases;

    nr_testcases = nr_passes + nr_fails;
    TEST_LOG("=======================================");
    TEST_LOG(" * test summary:");
    TEST_LOG("   successful           %3u%%(%3u/%3u)", nr_passes * 100 / nr_testcases, nr_passes, nr_testcases);
    TEST_LOG("   failed               %3u%%(%3u/%3u)", nr_fails * 100 / nr_testcases, nr_fails, nr_testcases);
    TEST_LOG("=======================================");

    printf("=======================================\n");
    printf(" * test summary:\n");
    printf("   successful           %3u%%(%3u/%3u)\n", nr_passes * 100 / nr_testcases, nr_passes, nr_testcases);
    printf("   failed               %3u%%(%3u/%3u)\n", nr_fails * 100 / nr_testcases, nr_fails, nr_testcases);
    printf("=======================================\n");
}

char *__to_testno(char *str, testno_t testno)
{
    u32 depth;

    memset(testno, 0, sizeof (testno_t));

    depth = 0;
    while(str) {
         testno[depth] = strtol(str, &str, 10);
         if (testno[depth] == 0) {
             break;
         }
         if (*str != '.') {
             return ++str;
         }
         str++;     /* to skip dot */
         depth++;
    }
    return NULL;
}

testcase_t *__get_subtc(testcase_t *tc, u32 idx)
{
    u32 i;

    if (tc) {
        for (i = 0; i <= idx; i++) {
            if (tc[i].name == NULL) {
                return NULL;
            }
        }
        return &tc[idx];
    }
    return NULL;
}

testcase_t *__get_tc(const testsuite_t *ts, testno_t testno)
{
    testcase_t *tc;
    u32 depth;

    tc = ts->testcases;
    depth = 0;
    while (testno[depth]) {
        if (depth == 0) {
            tc = __get_subtc(ts->testcases, testno[depth] - 1);
        } else {
            tc = __get_subtc(tc->testcases, testno[depth] - 1);
        }
        if (tc == NULL) {
            fprintf(stderr, "testsuite: test case out of range\n");
            return NULL;
        }
        depth++;
    }

    return tc;
}

int __read_env_var(const testsuite_t *ts, testno_t first, testcase_t **tc)
{
    testno_t testno;

    /* Test Numbering Example: 2.10.3 */
    __to_testno(getenv("TEST_FIRST"), testno);
    if (testno[0] == 0) {
        __first_tc = NULL;
        *tc = ts->testcases;
        memset(first, 0, sizeof (testno_t));
        first[0] = 1;
    } else {
        __first_tc = __get_tc(ts, testno);
        memcpy(first, testno, sizeof (testno));
        *tc = __first_tc;
        if (__first_tc == NULL) {
            return -1;
        }
    }

    __to_testno(getenv("TEST_LAST"), testno);
    if (testno[0] == 0) {
        __last_tc = NULL;
    } else {
        __last_tc = __get_tc(ts, testno);
        if (__last_tc == NULL) {
            return -1;
        }
    }

    return 0;
}

int testsuite_run(const testsuite_t *ts)
{
    int res;
    testno_t testno;
    testcase_t *tc;

    res = __read_env_var(ts, testno, &tc);
    if (res) {
        return -1;
    }

    INDENT_INIT();
    __test_log_file = fopen(LOG_FILENAME, "w");

    TEST_LOG("============ \"%s\" Test Begin ============", ts->name);

    if (ts->setup) {
        TEST_LOG("========== Setup Begin ==========");
        ts->setup();
        TEST_LOG("========== Setup End ==========");
    }

    printf("* %s Test\n", ts->name);
    __run_testcases(testno, tc);

    INDENT_INIT();
    if (ts->teardown) {
        TEST_LOG("========== Teardown Begin ==========");
        ts->teardown();
        TEST_LOG("========== Teardown End ==========");
    }
    TEST_LOG("============ \"%s\" Test End ============", ts->name);
    __show_summary();
    fclose(__test_log_file);

    return 0;
}

