LDFLAGS += -g -Wall -O2

SRCS     = $(wildcard *.c)
OBJS     = $(patsubst %.c, %.o, $(SRCS))
TARGETS  = $(SRCS:%.c=%)

$(info $(OBJS))
$(info $(TARGETS))

all : $(TARGETS)

$(TARGETS): %: %.o
	$(CC) $(LDFLAGS) $(LIBRARY) -o $@ $<

$(OBJS) : %.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean :
	@rm -rf $(TARGETS) $(OBJS)

#.SUFFIXES:
.PHONY : all clean
