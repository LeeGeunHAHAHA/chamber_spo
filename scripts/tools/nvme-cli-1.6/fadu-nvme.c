#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/fs.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdbool.h>

#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/ioctl.h> 


#include "linux/nvme_ioctl.h"
#include "nvme.h"
#include "nvme-print.h"
#include "nvme-ioctl.h"
#include "plugin.h"
#include "argconfig.h"
#include "suffix.h"

#define CREATE_CMD
#include "fadu-nvme.h"

enum {
    NVME_SC_SHELL_INPROGRESS = NVME_SC_SUCCESS + 1,
    NVME_SC_SHELL_INVALID_OPCODE,
};

#define NVME_DEVICE_TERMINATED -1

#define NVME_CMD_VS_SHELL 0xC1

// VU Command Type
#define ADMIN_CMD               0
#define IO_CMD                  1

// VU layer code
#define VU_LAYER_RSVD           0

// VU modle code
#define VU_MODULE_FTL           0
#define VU_MODULE_NIL           1
#define VU_MODULE_STAT          2
#define VU_MODULE_LIT           3
#define VU_MODULE_SHELL         4

// Data direction
#define VU_NO_DATA              0
#define VU_DMA_TO_HOST          1
#define VU_DMA_FROM_HOST        3
#define VU_DATA_DIRECTION_MASK  0xFF

// module shell

#define VU_SUB_OPCODE_SHELL_EXEC                ((IO_CMD << 31) | (VU_LAYER_RSVD << 24) | (VU_MODULE_SHELL << 16) | (0x00 << 8) | VU_DMA_FROM_HOST) // 0x80040003
#define VU_SUB_OPCODE_SHELL_KEEP_ALIVE          ((IO_CMD << 31) | (VU_LAYER_RSVD << 24) | (VU_MODULE_SHELL << 16) | (0x01 << 8) | VU_NO_DATA)       // 0x80040100
#define VU_SUB_OPCODE_SHELL_GET_RESULT          ((IO_CMD << 31) | (VU_LAYER_RSVD << 24) | (VU_MODULE_SHELL << 16) | (0x02 << 8) | VU_DMA_TO_HOST)   // 0x80040201

#define min(x, y) ((x) > (y) ? (y) : (x))

static int nvme_submit_io_passthru(int fd, struct nvme_passthru_cmd *cmd)
{
	return ioctl(fd, NVME_IOCTL_IO_CMD, cmd);
}

static int send_nvme_cmd(int fd, __u32 sub_opcode, __u32 data_len, void *data)
{
	struct nvme_passthru_cmd cmd = {
		.opcode		= NVME_CMD_VS_SHELL,
		.addr		= (__u64)(uintptr_t) data,
		.nsid		= 1,
		.data_len	= data_len,
		.cdw2		= sub_opcode,
		.cdw10		= (data_len >> 2),
		.cdw11		= 0,
		.cdw12		= 0,
		.cdw13		= 0,
		.cdw14		= 0,
		.cdw15	    = 0,
	};

	return nvme_submit_io_passthru(fd, &cmd);
}

static struct stat nvme_stat;
static int open_dev(const char *dev)
{
	int err, fd;

	devicename = basename(dev);
	err = open(dev, O_RDONLY);
	if (err < 0)
		goto perror;
	fd = err;

	err = fstat(fd, &nvme_stat);
	if (err < 0)
		goto perror;
	if (!S_ISCHR(nvme_stat.st_mode) && !S_ISBLK(nvme_stat.st_mode)) {
		fprintf(stderr, "%s is not a block or character device\n", dev);
		return -ENODEV;
	}
	return fd;
 perror:
	perror(dev);
	return err;
}

static int shell_exec(int argc, char **argv, struct command *cmd, struct plugin *plugin)
{
    int idx;
    int fd;
    int strlen;
    int status;
    int data_len;
    void *shell_cmd;
    char *exec_data;
    const int optind_dev = 1;

    int xfer      = 4096;
    int data_size = (16* 1024*1024); /* 16MB */

    data_len = 0;
    if (argc < 2)
    {
        printf("usage : nvme shell fadu <dev name> <shell cmd> \n");
        return EXIT_FAILURE;
    }

    fd = open_dev((const char *)argv[optind_dev]);
    if (fd < 0)
    {
        printf("usage : nvme shell fadu <dev name> <shell cmd>\n");
	return fd;
     }

	/*
     * argv[1] device name
     * argv[2 ~ [argc -1] ] shell command
     */
     if (posix_memalign(&shell_cmd, getpagesize(), xfer))
     {
         fprintf(stderr, "No memory for f/w size:%d\n", xfer);
         return ENOMEM;
     }

     if (posix_memalign((void *)&exec_data, getpagesize(), data_size))
     {
         fprintf(stderr, "No memory for f/w size:%d\n", data_size);
         return ENOMEM;
     }

     memset(exec_data, 0 ,data_size);

     strlen = 0;
     for (idx = 2; idx < argc; idx++)
     {
         strlen += sprintf(shell_cmd + strlen, "%s ", argv[idx]);
     }

     printf("shell_cmd:%s \n", (char *)shell_cmd);
     /*
      * shell command exectuion
      */
     status = send_nvme_cmd(fd, VU_SUB_OPCODE_SHELL_EXEC, strlen*2, shell_cmd);
     if (status == NVME_DEVICE_TERMINATED)
     {
        return NVME_DEVICE_TERMINATED;
     }
     
     if (status != NVME_SC_SUCCESS)
     {
        printf("There is a currently working shell command exist");
 	    return EAGAIN;
     }

     /*
      * shell command exectuion
      */
    do {
        status = send_nvme_cmd(fd, VU_SUB_OPCODE_SHELL_KEEP_ALIVE, 0, NULL);
	    printf("in progress(run) :%d\n", status);
        if (status == NVME_DEVICE_TERMINATED)
        {
            return NVME_DEVICE_TERMINATED;
        }
    } while (status != NVME_SC_SUCCESS);

    /*
     * read shell output
     */
    do
    {
        status = send_nvme_cmd(fd, VU_SUB_OPCODE_SHELL_GET_RESULT, xfer, exec_data + data_len);
        data_len += xfer;
        printf("In progress(data) :%d\n", status);
        if (status == NVME_SC_SHELL_INVALID_OPCODE)
        {
            printf("shell_cmd:%s is invalid command\n", (char *)shell_cmd);
            break;
        }
        if (status == NVME_DEVICE_TERMINATED)
        {
            return NVME_DEVICE_TERMINATED;
        }
    } while (status != NVME_SC_SUCCESS);

    printf("-----------------------------------------------------------\n");
    printf("%s", (char*)exec_data);

    return 0;
}
