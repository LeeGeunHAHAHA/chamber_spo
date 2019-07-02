/*
 * @file reg_access.c
 * @data 2018. 06. 13.
 * @author mkpark
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

#define REG_FINIT    (0xFC)

static void get_registers(const char *devicename, unsigned int **bar)
{
	int pci_fd;
	char path[512];
	void *membase;

	sprintf(path, "/sys/class/nvme/%s/device/resource0", devicename);
	pci_fd = open(path, O_RDWR);
	if (pci_fd < 0){
        sprintf(path, "/sys/class/misc/%s/device/resource0", devicename);
	    pci_fd = open(path, O_RDWR);
	}
	if (pci_fd < 0) {
	    sprintf(path, "/sys/bus/pci/devices/0000:%s/resource0", devicename);
	    pci_fd = open(path, O_RDWR);
	}
	if (pci_fd < 0) {
	    sprintf(path, "/sys/bus/pci/devices/%s/resource0", devicename);
	    pci_fd = open(path, O_RDWR);
	}
	if (pci_fd < 0) {
	    fprintf(stderr, "%s did not find a pci resource\n", devicename);
	    exit(ENODEV);
	}

	membase = mmap(0, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, pci_fd, 0);
	if (membase == MAP_FAILED) {
	    fprintf(stderr, "%s failed to map\n", devicename);
	    exit(ENODEV);
	}
	*bar = membase;
}

int main(int argc, char *argv[])
{
	unsigned int *bar;
    unsigned int read_index, write_index, write_value;

	if (argc < 3 && 4 < argc ) {
		printf("argument is less or more\n");
		return 0;
	}

	get_registers(argv[1], &bar);

	if (!strcmp(argv[2], "read")) {
		read_index = atoi(argv[3])/4 + atoi(argv[3])%4;
		printf("%08x\n", (*(volatile unsigned int *)&bar[read_index]));
	}

	if (!strcmp(argv[2], "write")) {
		write_index = atoi(argv[3])/4 + atoi(argv[3])%4;
		write_value = atoi(argv[4]);
		bar[write_index] = write_value;
	}
}
