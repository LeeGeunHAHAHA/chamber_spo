CFLAGS  += -Wall -fdiagnostics-color=auto -I$(shell pwd) -g
LIBS    += -lpthread -lm -laio -lrt
BIN     := reg_access
RM      := rm -rf
CP      := cp
SRCS    := reg_access.c
OBJS    := $(subst .c,.o,$(SRCS))
DEPS    := $(OBJS:.o=.d)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN) $(LIBS)

all: $(BIN)

-include $(DEPS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@ -MMD


tags:
	find $(PWD) -name '*.[ch]' -exec echo {} \; | grep -v "$(PWD)/unit/" | sort -u > cscope,files
	ctags -R -L cscope.files
	cscope -bvq

clean:
	$(RM) $(OBJS)
	$(RM) $(DEPS)
	$(RM) $(BIN)
	$(RM) tags cscope.files cscope.out cscope.in.out cscope.po.out

.PHONY: all clean tags
	
