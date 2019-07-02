CFLAGS	= -Wall -I$(shell pwd)
-include .config.mk

FQA_ROOT_DIR	:= $(abspath $(CURDIR)/../..)
include $(FQA_ROOT_DIR)/.config.mk

CFLAGS	+= $(SPDK_INCS)
LIBS	:= $(SPDK_LIBS)
BIN		:= prim_proc
RM		:= rm -rf
SRCS	+= prim_proc.c

OBJS	:= $(subst .c,.o,$(SRCS))
DEPS	:= $(OBJS:.o=.d)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN) $(LIBS)

all: $(BIN)

-include $(DEPS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@ -MMD

clean:
	$(RM) $(OBJS)
	$(RM) $(DEPS)
	$(RM) $(BIN)

.PHONY: all clean $(BIN)

