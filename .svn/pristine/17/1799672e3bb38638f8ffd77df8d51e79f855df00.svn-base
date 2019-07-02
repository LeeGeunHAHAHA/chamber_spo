#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "vu_common.h"
#include "statistics_struct.h"

void main (int argc, char* argv[])
{
    char cmd_str[500];
    char* fqa_device;

    if (argc < 1) {
        printf("argc:%d\n", argc);
        return;
    }

    fqa_device = argv[1];

    // reset defense code stat
    printf("%sReset defense code statistics\n", KMAG);
    vu_no_data(fqa_device, SUB_OPCODE_RESET_STAT_DEFENSE_CODE, 0, 0, 0, 0);
    printf("%s\n", KNRM);
}
