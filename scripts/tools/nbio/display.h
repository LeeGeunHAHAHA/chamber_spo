/**
 * @file display.h
 * @date 2016. 05. 31.
 * @author kmsun
 *
 * display engine interface definition
 */
#ifndef __NBIO_DISPLAY_H__
#define __NBIO_DISPLAY_H__

#include "list.h"
#include "iogen.h"

struct display {
    void *ctx;
    struct display_ops *ops;
    struct iogen *iogen;
};

struct display_ops {
    struct list_head list;
    char *name;
    int (*init)(struct display *);
    void (*update)(struct display *, const struct ioacct *);
    void (*report)(struct display *, const struct ioacct *);
    void (*exit)(struct display *);
};

int display_register(struct display_ops *disp_ops);
void display_unregister(struct display_ops *disp_ops);
struct display_ops *display_find(const char *name);

#endif /* __NBIO_DISPLAY_H__ */

