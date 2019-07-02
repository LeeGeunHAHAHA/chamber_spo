/**
 * @file prim_cap_parse.c
 * @date 2018. 10. 03.
 * @author kmsun
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "nvme_ds.h"

nvme_id_prim_ctrl_cap_t cap;

int main(void)
{
    size_t res;
    res = read(0, &cap, 4096);

    printf("cntlid  : %u\n", cap.cntlid);
    printf("portid  : %u\n", cap.portid);
    printf("crt     : 0x%x\n", cap.crt);
    printf("  [1:1] : %u   %s\n", (cap.crt >> 1) & 1, ((((cap.crt >> 1) & 1) == 0) ? "VI Resources are not supported" : "VI Resources are supported" ));
    printf("  [0:0] : %u   %s\n", cap.crt & 1, (((cap.crt & 1) == 0) ? "VQ Resources are not supported" : "VQ Resources are supported" ));
    printf("vqfrt   : %u\n", cap.vqfrt);
    printf("vqrfa   : %u\n", cap.vqrfa);
    printf("vqrfap  : %u\n", cap.vqrfap);
    printf("vqprt   : %u\n", cap.vqprt);
    printf("vqfrsm  : %u\n", cap.vqfrsm);
    printf("vqgran  : %u\n", cap.vqgran);
    printf("vifrt   : %u\n", cap.vifrt);
    printf("virfa   : %u\n", cap.virfa);
    printf("virfap  : %u\n", cap.virfap);
    printf("viprt   : %u\n", cap.viprt);
    printf("vifrsm  : %u\n", cap.vifrsm);
    printf("vigran  : %u\n", cap.vigran);

    return 0;
}
