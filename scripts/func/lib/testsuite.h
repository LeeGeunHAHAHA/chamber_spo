#ifndef __TESTSUITE_H__
#define __TESTSUITE_H__

#include <stdio.h>
#include <string.h>
#include "types.h"

extern int __test_res;
extern int __test_log_indent;
extern FILE* __test_log_file;

#define __TESTCASE_FAIL                 (1)
#define __TESTGROUP_FAIL                (2)
#define __TESTCASE_IGNORE               (3)

#define __TEST_STRINGIFY(x)             #x
#define TEST_STRINGIFY(x)               __TEST_STRINGIFY(x)

#define TEST_FAILED()                   (__test_res == __TESTCASE_FAIL)
#define TEST_INVOKE(a)                  do { TEST_LOG("> TEST_INVOKE(%s) at %s:%d", TEST_STRINGIFY(a), __FILE__, __LINE__); a; if (TEST_FAILED()) { return; } } while(0)

#define TEST_LOG(format, ...)           { fprintf(__test_log_file, "%s%*s" format "\n", __test_gettime(), __test_log_indent * 2, " ", ##__VA_ARGS__); fflush(__test_log_file); }

#define TEST_IGNORE()                   { TEST_LOG("-> ignored at %s:%d", __FILE__,  __LINE__); __test_res = __TESTCASE_IGNORE; return; };
#define TEST_FAIL()                     { TEST_LOG("-> failed at %s:%d", __FILE__,  __LINE__); __test_res = __TESTCASE_FAIL; return; };
#define TEST_GROUP_FAIL()               { TEST_LOG("-> test group failed at %s:%d", __FILE__,  __LINE__); __test_res = __TESTGROUP_FAIL; return; };
#define TEST_ASSERT_EQ(a, b)            { u64 __a, __b; TEST_LOG("> TEST_ASSERT_EQ(%s, %s) at %s:%d", TEST_STRINGIFY(a), TEST_STRINGIFY(b), __FILE__, __LINE__); __a = (a); __b = (b); if ((__a) != (__b)) { TEST_LOG(" > Expected <%llx>, but was <%llx>", (__b), (__a)); TEST_FAIL(); } }
#define TEST_ASSERT_NE(a, b)            { u64 __a, __b; TEST_LOG("> TEST_ASSERT_NQ(%s, %s) at %s:%d", TEST_STRINGIFY(a), TEST_STRINGIFY(b), __FILE__, __LINE__); __a = (a); __b = (b); if ((__a) == (__b)) { TEST_LOG(" > Expected not <%llu>, but was <%llu>", (__b), (__a)); TEST_FAIL(); } }
#define TEST_ASSERT_GT(a, b)            { u64 __a, __b; TEST_LOG("> TEST_ASSERT_GT(%s, %s) at %s:%d", TEST_STRINGIFY(a), TEST_STRINGIFY(b), __FILE__, __LINE__); __a = (a); __b = (b); if ((__a) <= (__b)) { TEST_LOG(" > Expected <%llu> to be greater than or equal to <%llu>", (__a), (__b)); TEST_FAIL(); } }
#define TEST_ASSERT_GE(a, b)            { u64 __a, __b; TEST_LOG("> TEST_ASSERT_GE(%s, %s) at %s:%d", TEST_STRINGIFY(a), TEST_STRINGIFY(b), __FILE__, __LINE__); __a = (a); __b = (b); if ((__a) < (__b)) { TEST_LOG(" > Expected <%llu> to be greater than <%llu>", (__a), (__b)); TEST_FAIL(); } }
#define TEST_ASSERT_LT(a, b)            { u64 __a, __b; TEST_LOG("> TEST_ASSERT_LT(%s, %s) at %s:%d", TEST_STRINGIFY(a), TEST_STRINGIFY(b), __FILE__, __LINE__); __a = (a); __b = (b); if ((__a) >= (__b)) { TEST_LOG(" > Expected <%llu> to be less than or equal to <%llu>", (__a), (__b)); TEST_FAIL(); } }
#define TEST_ASSERT_LE(a, b)            { u64 __a, __b; TEST_LOG("> TEST_ASSERT_LE(%s, %s) at %s:%d", TEST_STRINGIFY(a), TEST_STRINGIFY(b), __FILE__, __LINE__); __a = (a); __b = (b); if ((__a) > (__b)) { TEST_LOG(" > Expected <%llu> to be less than <%llu>", (__a), (__b)); TEST_FAIL(); } }
#define TEST_ASSERT_SC(a)               { int __a; TEST_LOG("> TEST_ASSERT_SC(%s) at %s:%d", TEST_STRINGIFY(a), __FILE__, __LINE__); __a = (a); if ((__a) != 0) { TEST_LOG(" > Expected <0>, but was <%d(%s)>", (__a), strerror(__a)); TEST_FAIL(); } }

#define TESTCASE(_name, _func)          { .name = (_name), .func = (_func) }
#define TESTGROUP(_name, _testcases)    { .name = (_name), .func = NULL, .testcases = (_testcases) }
#define END_OF_TEST                     { .name = NULL }
#define TEST_CONFIG                     ((char *)-1)

extern char *__test_gettime(void);

typedef struct __testcase testcase_t;

struct __testcase {
    const char *name;
    void (*func)(void);
    testcase_t *testcases;
};

typedef struct __testsuite {
    const char *name;
    void (*setup)(void);
    void (*teardown)(void);
    testcase_t *testcases;
} testsuite_t;

int testsuite_run(const testsuite_t *ts);

#endif

