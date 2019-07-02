SPDK_ENABLE	:= 1

CFLAGS	= -Wall -I$(shell pwd)
-include .config.mk

ifeq ($(SPDK_ENABLE),1)
FQA_ROOT_DIR	:= $(abspath $(CURDIR)/../..)
include $(FQA_ROOT_DIR)/.config.mk

CFLAGS	+= $(SPDK_INCS)
LIBS	:= $(SPDK_LIBS)
SRCS	:= io/spdkio.c
endif
LIBS	+= -lpthread -lm -laio -lrt
BIN		:= nbio
RM		:= rm -rf
CP		:= cp
SRCS	+= io/null.c			\
		   io/libaio.c			\
		   ioengine.c			\
		   disp/standalone.c	\
		   disp/testcase.c		\
		   display.c			\
		   iogen.c				\
		   iover.c				\
		   datagen.c			\
		   parser.c				\
		   rng.c				\
		   jesd219.c			\
		   log.c				\
		   dump.c				\
		   crc_t10dif.c			\
		   powerctl.c			\
		   main.c

OBJS	:= $(subst .c,.o,$(SRCS))
DEPS	:= $(OBJS:.o=.d)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN) $(LIBS)

all: $(BIN)

debug:
	$(CP) debug.mk .config.mk
	$(MAKE)

release:
	$(CP) release.mk .config.mk
	$(MAKE)

profile:
	$(CP) profile.mk .config.mk
	$(MAKE)

-include $(DEPS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@ -MMD

tags:
	find $(PWD) -name '*.[ch]' -exec echo {} \; | grep -v "$(PWD)/unit/" | sort -u > cscope.files
	ctags -R -L cscope.files
	cscope -bvq

clean:
	$(RM) $(OBJS)
	$(RM) $(DEPS)
	$(RM) $(BIN)
	$(RM) tags cscope.files cscope.out cscope.in.out cscope.po.out

.PHONY: all clean tags debug release profile $(BIN)

