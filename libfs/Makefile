libs := libfs.a
objs    := disk.o fs.o

CC      := gcc
CFLAGS  := -Wall -MMD #-Werror -Wextra
CFLAGS  += -g

ifneq ($(V),1)
Q = @
endif

all: $(libs)

# Dep tracking *must* be below the 'all' rule
deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)

## TODO: Phase 1
libfs.a: $(objs)
	@echo "CC $@"
	$(Q) ar rcs $@ $^

%.o: %.c
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "clean"
	$(Q)rm -f $(libs) $(objs) $(deps)
