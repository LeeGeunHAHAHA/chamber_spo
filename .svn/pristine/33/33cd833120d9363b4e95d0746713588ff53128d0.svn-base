/**
 * @file display.c
 * @date 2016. 05. 31.
 * @author kmsun
 */

#include <string.h>
#include "display.h"

static LIST_HEAD(disp_ops_list);

int display_register(struct display_ops *disp_ops)
{
    INIT_LIST_HEAD(&disp_ops->list);
    list_add(&disp_ops->list, &disp_ops_list);

    return 0;
}

void display_unregister(struct display_ops *disp_ops)
{
    list_del(&disp_ops->list);
}

struct display_ops *display_find(const char *name)
{
    struct display_ops *disp_ops;

    list_for_each_entry(disp_ops, &disp_ops_list, list) {
        if (strcmp(name, disp_ops->name) == 0) {
            return disp_ops;
        }
    }

    return NULL;
}

