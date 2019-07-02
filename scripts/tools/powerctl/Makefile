CFLAGS	= -Wall -I$(shell pwd)

CC	= gcc
SRCS	:= powerctl.c
BIN	:= powerctl
RM	:= rm -rf
CP	:= cp
OBJS	:= $(subst .c,.o,$(SRCS))

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN) $(LIBS)

all: $(BIN)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@ 

clean:
	$(RM) $(OBJS)
	$(RM) $(BIN)

.PHONY: all clean tags debug release profile $(BIN)

