FQA_ROOT_DIR:= $(abspath $(CURDIR)/../..)
include	$(FQA_ROOT_DIR)/.config.mk
INCS 		:= -I../../func/lib -I$(SPDK_ROOT_DIR)/lib/nvme $(SPDK_INCS)
LIBS		:= $(SPDK_LIBS)
LIBS		+= -lm -lpthread -lm -laio -lrt 
OBJS 		:= identify.o
#OBJS 		:= outstanding_cmds.o
BIN			:= identify
#BIN			:= outstanding_cmds

CFLAGS 		:= -g $(INCS) -fPIC -lm -MMD -Wall -Werror
#CFLAGS 		:= -O3 $(INCS) -fPIC -lm -MMD -Wall -Werror
DEPS    	:= $(OBJS:%.o=%.d)

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN) $(LIBS)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(OBJS) $(DEPS) $(BIN) test.log

-include $(DEPS)

.PHONY: all clean $(BIN)

