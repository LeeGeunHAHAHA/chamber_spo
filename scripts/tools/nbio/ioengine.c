/**
 * @file ioengine.c
 * @date 2016. 05. 16.
 * @author kmsun
 */

#include <string.h>
#include "ioengine.h"

static LIST_HEAD(engines);

int ioengine_register(struct ioengine_ops *engine_ops)
{
    INIT_LIST_HEAD(&engine_ops->list);
    list_add(&engine_ops->list, &engines);
    return 0;
}

int ioengine_unregister(struct ioengine_ops *engine_ops)
{
    list_del(&engine_ops->list);
    return 0;
}

struct ioengine_ops *ioengine_find(char *name)
{
    struct ioengine_ops *engine_ops;

    list_for_each_entry(engine_ops, &engines, list) {
        if (strcmp(name, engine_ops->name) == 0) {
            return engine_ops;
        }
    }

    return NULL;
}

