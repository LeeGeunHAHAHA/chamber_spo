/**
 * @file sec_list_parse.c
 * @date 2018. 10. 03.
 * @author kmsun
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "nvme_ds.h"

nvme_id_sec_ctrl_list_t sec_ctrl_list;

int main(int argc, char *argv[])
{
    nvme_sec_ctrl_entry_t *entry;
    size_t res;
    u8 i;
    u8 ctrlid = 0;

    if (argc > 1) {
        ctrlid = (u8)atoi(argv[1]);
    }

    res = read(0, &sec_ctrl_list, 4096);
    if (ctrlid == 0) {
        printf("nr      : %u\n", sec_ctrl_list.nr);
    }
    for (i = 0; i < sec_ctrl_list.nr; i++) {
        if ((ctrlid != 0) && (ctrlid - 1 != i)) {
            continue;
        }
        entry = &sec_ctrl_list.entries[i];
        printf(".................\n");
        printf(" Entry[%u]\n", i);
        printf(".................\n");
        printf("scid    : %u\n", entry->scid);
        printf("pcid    : %u\n", entry->pcid);
        printf("scs     : 0x%x\n", entry->scs);
        printf("  [0:0] : %u   %s\n", entry->scs & 1, ((entry->scs & 1) == 0) ? "Offline" : "Online");
        printf("vfn     : %u\n", entry->vfn);
        printf("nvq     : %u\n", entry->nvq);
        printf("nvi     : %u\n", entry->nvi);
    }

    return 0;
}
