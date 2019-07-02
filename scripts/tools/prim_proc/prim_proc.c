/**
 * @file prim_proc.c
 * @date 2018. 06. 20.
 * @author kmsun
 *
 * primary process
 */

#include <stdio.h>
#include <stdlib.h>
#include "spdk/stdinc.h"
#include "spdk/nvme.h"
#include "spdk/env.h"
#include "spdk/log.h"

struct spdk_nvme_ctrlr *g_ctrlr;
int show;

static bool probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid, struct spdk_nvme_ctrlr_opts *opts) 
{
	return true;
}

static void attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid, struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts)
{
    g_ctrlr = ctrlr;
}

static void aer_cb(void *aer_cb_arg, const struct spdk_nvme_cpl *cpl)
{
    const char *event_type_str[] = { "Error Status", "SMART/Health Status", "Notice", "Undefined", "Undefined", "Undefind", "NVM Command Set Specific Status", "Vendor Specific Status" };
    const char *info_str;
    union spdk_nvme_async_event_completion async_event;

    async_event.raw = cpl->cdw0;
    switch (async_event.bits.async_event_type) {
        case SPDK_NVME_ASYNC_EVENT_TYPE_ERROR:
            switch (async_event.bits.async_event_info) {
                case SPDK_NVME_ASYNC_EVENT_WRITE_INVALID_DB:
                    info_str = "Write to Invalid Doorbell Register";
                    break;
                case SPDK_NVME_ASYNC_EVENT_INVALID_DB_WRITE:
                    info_str = "Invalid Doorbell Register Write Value";
                    break;
                case SPDK_NVME_ASYNC_EVENT_DIAGNOSTIC_FAILURE:
                    info_str = "Diagnostic Failure";
                    break;
                case SPDK_NVME_ASYNC_EVENT_PERSISTENT_INTERNAL:
                    info_str = "Persistent Internal Error";
                    break;
                case SPDK_NVME_ASYNC_EVENT_TRANSIENT_INTERNAL:
                    info_str = "Transient Internal Error";
                    break;
                case SPDK_NVME_ASYNC_EVENT_FW_IMAGE_LOAD:
                    info_str = "Firmware Image Load Error";
                    break;
                default:
                    info_str = "Undefind";
                    break;
            }
            break;
        case SPDK_NVME_ASYNC_EVENT_TYPE_SMART:
            switch (async_event.bits.async_event_info) {
                case SPDK_NVME_ASYNC_EVENT_SUBSYSTEM_RELIABILITY:
                    info_str = "NVM Subsystem Reliability";
                    break;
                case SPDK_NVME_ASYNC_EVENT_TEMPERATURE_THRESHOLD:
                    info_str = "Temperature Threshold";
                    break;
                case SPDK_NVME_ASYNC_EVENT_SPARE_BELOW_THRESHOLD:
                    info_str = "Spare Below Threshold";
                    break;
                default:
                    info_str = "Undefind";
                    break;
            }
            break;
        case SPDK_NVME_ASYNC_EVENT_TYPE_NOTICE:
            switch (async_event.bits.async_event_info) {
                case SPDK_NVME_ASYNC_EVENT_NS_ATTR_CHANGED:
                    info_str = "Namespace Attribute Changed";
                    break;
                case SPDK_NVME_ASYNC_EVENT_FW_ACTIVATION_START:
                    info_str = "Firmware Activation Starting";
                    break;
                case SPDK_NVME_ASYNC_EVENT_TELEMETRY_LOG_CHANGED:
                    info_str = "Telemetry Log Changed";
                    break;
                default:
                    info_str = "Undefind";
                    break;
            }
            break;
        case SPDK_NVME_ASYNC_EVENT_TYPE_IO:
            switch (async_event.bits.async_event_info) {
                case SPDK_NVME_ASYNC_EVENT_RESERVATION_LOG_AVAIL:
                    info_str = "Reservation Log Page Available";
                    break;
                case SPDK_NVME_ASYNC_EVENT_SANITIZE_COMPLETED:
                    info_str = "Sanitize Operation Completed";
                    break;
                default:
                    info_str = "Undefind";
                    break;
            }
            break;
        case SPDK_NVME_ASYNC_EVENT_TYPE_VENDOR:
            switch (async_event.bits.async_event_info) {
                case SPDK_NVME_ASYNC_EVENT_RESERVATION_LOG_AVAIL:
                    info_str = "Reservation Log Page Available";
                    break;
                case SPDK_NVME_ASYNC_EVENT_SANITIZE_COMPLETED:
                    info_str = "Sanitize Operation Completed";
                    break;
                default:
                    info_str = "Undefind";
                    break;
            }
            break;
        default:
            info_str = "Undefind";
            break;
    }

    if (show) {
        printf("Asynchronous Event Type: %s(%u)\n", event_type_str[async_event.bits.async_event_type], async_event.bits.async_event_type);
        printf("Asynchronous Event Info: %s(%u)\n", info_str, async_event.bits.async_event_info);
        fflush(stdout);
    }
}

static void show_aerc(int signo)
{
    show = 1;
    spdk_nvme_ctrlr_process_admin_completions(g_ctrlr);
}

static void discard_aerc(int signo)
{
    show = 0;
    spdk_nvme_ctrlr_process_admin_completions(g_ctrlr);
}

int main(int argc, char *argv[])
{
    struct spdk_env_opts opts;
	int res;

    spdk_env_opts_init(&opts);
	opts.name = "primary process";
	opts.shm_id = 0;
	opts.mem_size = 2048;

    if (getenv("FQA_SPDK_SHM_ID")) {
        opts.shm_id = atoi(getenv("FQA_SPDK_SHM_ID"));
    }

    if (getenv("FQA_SPDK_MEM_SIZE")) {
        opts.mem_size = atoi(getenv("FQA_SPDK_MEM_SIZE"));
    }

    res = spdk_env_init(&opts);
	if (res) {
		return res;
	}

	spdk_log_set_level(SPDK_LOG_ERROR);

	res = spdk_nvme_probe(NULL, NULL, probe_cb, attach_cb, NULL);
	if (res) {
		return res;
	}

    if (signal(SIGUSR1, show_aerc) == SIG_ERR) {
        fprintf(stderr, "signal() returns error\n");
        return 1;
    }
    if (signal(SIGUSR2, discard_aerc) == SIG_ERR) {
        fprintf(stderr, "signal() returns error\n");
        return 1;
    }

    spdk_nvme_ctrlr_register_aer_callback(g_ctrlr, aer_cb, NULL);

	while (1) {
        /*spdk_nvme_ctrlr_process_admin_completions(g_ctrlr);*/
		sleep(100);
	}

    return 0;
}

