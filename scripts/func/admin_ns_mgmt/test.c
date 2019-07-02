/**
 * @file test_ns_mgmt.c
 * @date 2018. 05. 24.
 * @author kmsun
 */

#include "types.h"
#include "testsuite.h"

#include "spdk/stdinc.h"
#include "spdk/nvme.h"
#include "spdk/env.h"
#include "spdk/log.h"
#include "nvme_internal.h"

#define NS_ALLOC_UNIT       (1UL << 30)

struct ctrlr_entry {
    struct spdk_nvme_ctrlr  *ctrlr;
    struct ctrlr_entry      *next;
    char                    name[1024];
};

static struct ctrlr_entry *g_controllers = NULL;
static struct spdk_nvme_ctrlr* ctrlr;

static u8 *payload;
static const struct spdk_nvme_ctrlr_data *cdata;
static struct nvme_completion_poll_status status;
static struct spdk_nvme_qpair *ioqpair;

static void identify_ctrlr(void)
{
    TEST_ASSERT_EQ(nvme_ctrlr_cmd_identify(ctrlr, SPDK_NVME_IDENTIFY_CTRLR, 0, 0, payload, 4096, nvme_completion_poll_cb, &status), 0);
    TEST_ASSERT_EQ(spdk_nvme_wait_for_completion_robust_lock(ctrlr->adminq, &status, &ctrlr->ctrlr_lock), 0);
    TEST_ASSERT_EQ(spdk_nvme_cpl_is_error(&status.cpl), 0);
    cdata = (struct spdk_nvme_ctrlr_data *)payload;
}

static bool probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid, struct spdk_nvme_ctrlr_opts *opts)
{
    return true;
}

static void attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid, struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts)
{
    struct ctrlr_entry *entry;

    entry = malloc(sizeof(struct ctrlr_entry));
    if (entry == NULL) {
        perror("ctrlr_entry malloc");
        exit(1);
    }

    cdata = spdk_nvme_ctrlr_get_data(ctrlr);
    snprintf(entry->name, sizeof(entry->name), "%-20.20s (%-20.20s)", cdata->mn, cdata->sn);

    entry->ctrlr = ctrlr;
    entry->next = g_controllers;
    g_controllers = entry;
}

static void setup()
{
    int rc;
    struct spdk_env_opts opts;

    g_spdk_log_print_level = SPDK_LOG_ERROR;

    spdk_env_opts_init(&opts);
    opts.name = "test_ns_mgmt";
    opts.shm_id = 0;
    if (spdk_env_init(&opts) < 0) {
        fprintf(stderr, "Unable to initialize SPDK env\n");
        return;
    }

    rc = spdk_nvme_probe(NULL, NULL, probe_cb, attach_cb, NULL);
    if (rc != 0) {
        fprintf(stderr, "spdk_nvme_probe() failed\n");
        return;
    }

    if (g_controllers == NULL) {
        fprintf(stderr, "no NVMe controllers found\n");
        return;
    }

    ctrlr = g_controllers->ctrlr;

    payload = spdk_dma_malloc(4096, 0, NULL);
}

static void teardown()
{
    spdk_nvme_detach(ctrlr);
}

static void test_create_ns()
{
    struct spdk_nvme_ns_data *ndata;
    u64 unvmcap;
  
    TEST_INVOKE(identify_ctrlr());
    unvmcap = cdata->unvmcap[0];

    ndata = (struct spdk_nvme_ns_data *)payload;
    memset(ndata, 0, 4096);
    ndata->nsze = 0x1000;
    ndata->ncap = 0x1000;
    ndata->flbas.format = 0;
    ndata->dps.pit = 0;
    ndata->dps.md_start = 0;
    ndata->nmic.can_share = 1;

    TEST_ASSERT_EQ(spdk_nvme_ctrlr_create_ns(ctrlr, ndata), 1);

    unvmcap -= ALIGN(0x1000ULL << 12, NS_ALLOC_UNIT);
    TEST_INVOKE(identify_ctrlr());
    TEST_ASSERT_EQ(cdata->unvmcap[0], unvmcap);
}

static void test_delete_invalid_ns()
{
    TEST_ASSERT_EQ(nvme_ctrlr_cmd_delete_ns(ctrlr, 0, nvme_completion_poll_cb, &status), 0);
    TEST_ASSERT_NE(spdk_nvme_wait_for_completion_robust_lock(ctrlr->adminq, &status, &ctrlr->ctrlr_lock), 0);
    TEST_ASSERT_EQ(status.cpl.status.sct, SPDK_NVME_SCT_GENERIC);
    TEST_ASSERT_EQ(status.cpl.status.sc, SPDK_NVME_SC_INVALID_NAMESPACE_OR_FORMAT);

    TEST_ASSERT_EQ(nvme_ctrlr_cmd_delete_ns(ctrlr, 2, nvme_completion_poll_cb, &status), 0);
    TEST_ASSERT_NE(spdk_nvme_wait_for_completion_robust_lock(ctrlr->adminq, &status, &ctrlr->ctrlr_lock), 0);
    TEST_ASSERT_EQ(status.cpl.status.sct, SPDK_NVME_SCT_GENERIC);
    TEST_ASSERT_EQ(status.cpl.status.sc, SPDK_NVME_SC_INVALID_NAMESPACE_OR_FORMAT);
}

static void test_delete_valid_ns()
{
    TEST_ASSERT_EQ(spdk_nvme_ctrlr_delete_ns(ctrlr, 1), 0);
}

static void test_create_ns_thin_provision()
{
    struct spdk_nvme_ns_data *ndata;

    ndata = (struct spdk_nvme_ns_data *)payload;
    memset(ndata, 0, 4096);
    ndata->nsze = 0x10000;
    ndata->ncap = 0x8000;
    ndata->flbas.format     = 0;
    ndata->dps.pit          = 0;
    ndata->dps.md_start     = 0;
    ndata->nmic.can_share   = 1;

    TEST_ASSERT_EQ(nvme_ctrlr_cmd_create_ns(ctrlr, ndata, nvme_completion_poll_cb, &status), 0);
    TEST_ASSERT_NE(spdk_nvme_wait_for_completion_robust_lock(ctrlr->adminq, &status, &ctrlr->ctrlr_lock), 0);
    TEST_ASSERT_EQ(status.cpl.status.sct, SPDK_NVME_SCT_COMMAND_SPECIFIC);
    TEST_ASSERT_EQ(status.cpl.status.sc, SPDK_NVME_SC_THINPROVISIONING_NOT_SUPPORTED);
}

static void test_create_ns_invalid_lbaf()
{
    struct spdk_nvme_ns_data *ndata;

    ndata = (struct spdk_nvme_ns_data *)payload;
    memset(ndata, 0, 4096);
    ndata->nsze = 0x1000;
    ndata->ncap = 0x1000;
    ndata->flbas.format     = 8;        /* invalid lba format */
    ndata->dps.pit          = 0;
    ndata->dps.md_start     = 0;
    ndata->nmic.can_share   = 1;

    TEST_ASSERT_EQ(nvme_ctrlr_cmd_create_ns(ctrlr, ndata, nvme_completion_poll_cb, &status), 0);
    TEST_ASSERT_NE(spdk_nvme_wait_for_completion_robust_lock(ctrlr->adminq, &status, &ctrlr->ctrlr_lock), 0);
    TEST_ASSERT_EQ(status.cpl.status.sct, SPDK_NVME_SCT_GENERIC);
    TEST_ASSERT_EQ(status.cpl.status.sc, SPDK_NVME_SC_INVALID_FIELD);
}

static void test_create_ns_invalid_select()
{
    struct spdk_nvme_cmd cmd;

    memset(&cmd, 0, sizeof (cmd));
    cmd.opc = SPDK_NVME_OPC_NS_MANAGEMENT;
    cmd.nsid = 1;
    cmd.cdw10 = 3;                      /* invalid select value */

    status.done = false;
    TEST_ASSERT_EQ(spdk_nvme_ctrlr_cmd_admin_raw(ctrlr, &cmd, NULL, 0, nvme_completion_poll_cb, &status), 0);
    while (status.done == false) {
        spdk_nvme_ctrlr_process_admin_completions(ctrlr);
    }
    TEST_ASSERT_EQ(status.cpl.status.sct, SPDK_NVME_SCT_GENERIC);
    TEST_ASSERT_EQ(status.cpl.status.sc, SPDK_NVME_SC_INVALID_FIELD);
}

static void test_create_ns_entire_space()
{
    struct spdk_nvme_ns_data *ndata;
    struct spdk_nvme_ctrlr_list *ctrl_list;
    u32 i;
    int res;

    for (i = 0; i < 32; i++) {
        ndata = (struct spdk_nvme_ns_data *)payload;

        memset(ndata, 0, sizeof (*ndata));
        ndata->nsze = 0x4000;       /* 1GB */
        ndata->ncap = 0x4000;       /* 1GB */
        ndata->flbas.format     = 0;
        ndata->dps.pit          = 0;
        ndata->dps.md_start     = 0;
        ndata->nmic.can_share   = 0;

        TEST_ASSERT_EQ(spdk_nvme_ctrlr_create_ns(ctrlr, ndata), i + 1);

        ctrl_list = (struct spdk_nvme_ctrlr_list *)payload;
        memset(ctrl_list, 0, 4096);
        ctrl_list->ctrlr_count = 1;       /* number of identifiers */
        ctrl_list->ctrlr_list[0] = 0;     /* identifer 0 */

        TEST_ASSERT_EQ(spdk_nvme_ctrlr_attach_ns(ctrlr, i + 1, ctrl_list), 0);
    }

    TEST_INVOKE(identify_ctrlr());
    TEST_ASSERT_EQ(cdata->unvmcap[0], 0);
    res = system("nbio -q ns_test");
    TEST_ASSERT_EQ(res, 0);
}

static void test_create_ns_should_fail()
{
    struct spdk_nvme_ns_data *ndata;

    ndata = (struct spdk_nvme_ns_data *)payload;
    memset(ndata, 0, sizeof (*ndata));
    ndata->nsze = 0x4000;       /* 1GB */
    ndata->ncap = 0x4000;       /* 1GB */
    ndata->flbas.format     = 0;
    ndata->dps.pit          = 0;
    ndata->dps.md_start     = 0;
    ndata->nmic.can_share   = 1;

    TEST_ASSERT_EQ(nvme_ctrlr_cmd_create_ns(ctrlr, ndata, nvme_completion_poll_cb, &status), 0);
    TEST_ASSERT_NE(spdk_nvme_wait_for_completion_robust_lock(ctrlr->adminq, &status, &ctrlr->ctrlr_lock), 0);
    TEST_ASSERT_EQ(status.cpl.status.sct, SPDK_NVME_SCT_COMMAND_SPECIFIC);
    TEST_ASSERT_EQ(status.cpl.status.sc, SPDK_NVME_SC_NAMESPACE_INSUFFICIENT_CAPACITY);
}

static void test_delete_nses()
{
    u64 unvmcap;

    TEST_INVOKE(identify_ctrlr());
    unvmcap = cdata->unvmcap[0];

    /* delete namespace id 2 */
    TEST_ASSERT_EQ(spdk_nvme_ctrlr_delete_ns(ctrlr, 2), 0);

    unvmcap += ALIGN(0x40000ULL << 12, NS_ALLOC_UNIT);
    TEST_INVOKE(identify_ctrlr());
    TEST_ASSERT_EQ(cdata->unvmcap[0], unvmcap);

    /* delete namespace id 6 */
    TEST_ASSERT_EQ(spdk_nvme_ctrlr_delete_ns(ctrlr, 6), 0);

    unvmcap += ALIGN(0x40000ULL << 12, NS_ALLOC_UNIT);
    TEST_INVOKE(identify_ctrlr());
    TEST_ASSERT_EQ(cdata->unvmcap[0], unvmcap);
}

static void test_compact_create_ns()
{
    struct spdk_nvme_ns_data *ndata;
    u64 unvmcap;

    TEST_INVOKE(identify_ctrlr());
    unvmcap = cdata->unvmcap[0];

    /* create namespace(map segment table compaction should be performed) */
    ndata = (struct spdk_nvme_ns_data *)payload;
    memset(ndata, 0, sizeof (*ndata));
    ndata->nsze = 0x8000;       /* 2GB */
    ndata->ncap = 0x8000;       /* 2GB */
    ndata->flbas.format     = 0;
    ndata->dps.pit          = 0;
    ndata->dps.md_start     = 0;
    ndata->nmic.can_share   = 1;

    TEST_ASSERT_EQ(spdk_nvme_ctrlr_create_ns(ctrlr, ndata), 2);

    unvmcap -= ALIGN(ndata->nsze << 12, NS_ALLOC_UNIT);
    TEST_INVOKE(identify_ctrlr());
    TEST_ASSERT_EQ(cdata->unvmcap[0], unvmcap);
}

static void test_delete_all_ns()
{
    TEST_ASSERT_EQ(spdk_nvme_ctrlr_delete_ns(ctrlr, 0xFFFFFFFF), 0);

    TEST_INVOKE(identify_ctrlr());
    TEST_ASSERT_EQ(cdata->unvmcap[0], cdata->tnvmcap[0]);
}

static void test_read_inactive_ns_should_fail()
{
    struct spdk_nvme_cmd cmd;

    memset(&cmd, 0, sizeof (cmd));
    cmd.opc = SPDK_NVME_OPC_READ;
    cmd.nsid = 1;

    ioqpair = spdk_nvme_ctrlr_alloc_io_qpair(ctrlr, NULL, 0);

    status.done = false;
    TEST_ASSERT_EQ(spdk_nvme_ctrlr_cmd_io_raw(ctrlr, ioqpair, &cmd, NULL, 0, nvme_completion_poll_cb, &status), 0);
    while (status.done == false) {
        spdk_nvme_qpair_process_completions(ioqpair, 0);
    }
    TEST_ASSERT_EQ(status.cpl.status.sct, SPDK_NVME_SCT_COMMAND_SPECIFIC);
    TEST_ASSERT_EQ(status.cpl.status.sc, SPDK_NVME_SC_NAMESPACE_NOT_ATTACHED);

    spdk_nvme_ctrlr_free_io_qpair(ioqpair);
}

static void test_write_inactive_ns_should_fail()
{
    struct spdk_nvme_cmd cmd;

    memset(&cmd, 0, sizeof (cmd));
    cmd.opc = SPDK_NVME_OPC_WRITE;
    cmd.nsid = 1;

    ioqpair = spdk_nvme_ctrlr_alloc_io_qpair(ctrlr, NULL, 0);

    status.done = false;
    TEST_ASSERT_EQ(spdk_nvme_ctrlr_cmd_io_raw(ctrlr, ioqpair, &cmd, NULL, 0, nvme_completion_poll_cb, &status), 0);
    while (status.done == false) {
        spdk_nvme_qpair_process_completions(ioqpair, 0);
    }
    TEST_ASSERT_EQ(status.cpl.status.sct, SPDK_NVME_SCT_COMMAND_SPECIFIC);
    TEST_ASSERT_EQ(status.cpl.status.sc, SPDK_NVME_SC_NAMESPACE_NOT_ATTACHED);

    spdk_nvme_ctrlr_free_io_qpair(ioqpair);
}

static void test_attach_ns()
{
    struct spdk_nvme_ctrlr_list *ctrl_list = (struct spdk_nvme_ctrlr_list *)payload;

    memset(ctrl_list, 0, 4096);
    ctrl_list->ctrlr_count = 1;       /* number of identifiers */
    ctrl_list->ctrlr_list[0] = 0;     /* identifer 0 */

    TEST_ASSERT_EQ(spdk_nvme_ctrlr_attach_ns(ctrlr, 1, ctrl_list), 0);
}

static void test_read_active_ns()
{
    struct spdk_nvme_ns *ns;

    ioqpair = spdk_nvme_ctrlr_alloc_io_qpair(ctrlr, NULL, 0);

    ns = spdk_nvme_ctrlr_get_ns(ctrlr, 1);

    status.done = false;
    TEST_ASSERT_EQ(spdk_nvme_ns_cmd_read(ns, ioqpair, payload, 0, 1, nvme_completion_poll_cb, &status, 0), 0);
    while (status.done == 0) {
        spdk_nvme_qpair_process_completions(ioqpair, 0);
    }
    TEST_ASSERT_EQ(spdk_nvme_cpl_is_error(&status.cpl), 0);

    spdk_nvme_ctrlr_free_io_qpair(ioqpair);
}

static void test_write_active_ns()
{
    struct spdk_nvme_ns *ns;

    ioqpair = spdk_nvme_ctrlr_alloc_io_qpair(ctrlr, NULL, 0);

    ns = spdk_nvme_ctrlr_get_ns(ctrlr, 1);

    status.done = false;
    TEST_ASSERT_EQ(spdk_nvme_ns_cmd_write(ns, ioqpair, payload, 0, 1, nvme_completion_poll_cb, &status, 0), 0);
    while (status.done == false) {
        spdk_nvme_qpair_process_completions(ioqpair, 0);
    }
    TEST_ASSERT_EQ(spdk_nvme_cpl_is_error(&status.cpl), 0);

    spdk_nvme_ctrlr_free_io_qpair(ioqpair);
}

static void test_attach_already_attached_ns()
{
    struct spdk_nvme_ctrlr_list *ctrl_list = (struct spdk_nvme_ctrlr_list *)payload;

    memset(ctrl_list, 0, 4096);
    ctrl_list->ctrlr_count = 1;       /* number of identifiers */
    ctrl_list->ctrlr_list[0] = 0;     /* identifer 0 */

    TEST_ASSERT_EQ(nvme_ctrlr_cmd_attach_ns(ctrlr, 1, ctrl_list, nvme_completion_poll_cb, &status), 0);
    TEST_ASSERT_NE(spdk_nvme_wait_for_completion_robust_lock(ctrlr->adminq, &status, &ctrlr->ctrlr_lock), 0);
    TEST_ASSERT_EQ(status.cpl.status.sct, SPDK_NVME_SCT_COMMAND_SPECIFIC);
    TEST_ASSERT_EQ(status.cpl.status.sc, SPDK_NVME_SC_NAMESPACE_ALREADY_ATTACHED);
}

static void test_detach_ns()
{
    struct spdk_nvme_ctrlr_list *ctrl_list = (struct spdk_nvme_ctrlr_list *)payload;

    memset(ctrl_list, 0, 4096);
    ctrl_list->ctrlr_count = 1;       /* number of identifiers */
    ctrl_list->ctrlr_list[0] = 0;     /* identifer 0 */

    TEST_ASSERT_EQ(spdk_nvme_ctrlr_detach_ns(ctrlr, 1, ctrl_list), 0);
}

testcase_t test_ns_mgmt[] = {
    TESTCASE("delete all namespaces",                   test_delete_all_ns),
    TESTCASE("create namespace",                        test_create_ns),
    TESTCASE("delete not existing namespace",           test_delete_invalid_ns),
    TESTCASE("delete existing namespaces",              test_delete_valid_ns),
    TESTCASE("create namespace with thin-provision",    test_create_ns_thin_provision),
    TESTCASE("create namespace with invalid lba format",test_create_ns_invalid_lbaf),
    TESTCASE("create namespace with invalid select",    test_create_ns_invalid_select),
    TESTCASE("create namespaces in the entire space",   test_create_ns_entire_space),
    TESTCASE("create namespace should fail",            test_create_ns_should_fail),
    TESTCASE("delete some namespaces",                  test_delete_nses),
    TESTCASE("compact and create a namespace",          test_compact_create_ns),
    TESTCASE("delete all namespaces",                   test_delete_all_ns),
    TESTCASE("create single namespace",                 test_create_ns),
    TESTCASE("read from inactive namespace should fail",test_read_inactive_ns_should_fail),
    TESTCASE("write to inactive namespace should fail", test_write_inactive_ns_should_fail),
    TESTCASE("attach namespace",                        test_attach_ns),
    TESTCASE("read from active namespace",              test_read_active_ns),
    TESTCASE("write to active namespace",               test_write_active_ns),
    TESTCASE("attach already attached namespace",       test_attach_already_attached_ns),
    TESTCASE("detach namespace",                        test_detach_ns),
    TESTCASE("read from inactive namespace should fail",test_read_inactive_ns_should_fail),
    TESTCASE("write to inactive namespace should fail", test_write_inactive_ns_should_fail),
    END_OF_TEST,
};

testsuite_t testsuite = {
    .name = "namespace management",
    .testcases = test_ns_mgmt,
    .setup = setup,
    .teardown = teardown,
};

int main(int argc, char *argv[])
{
    return testsuite_run(&testsuite);
}

