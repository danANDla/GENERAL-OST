CC = gcc

TARGET_EXEC ?= result

BUILD_DIR ?= ./build
DEBUG_DIR ?= ./debug
SRC_DIRS ?= /

SRCS := main.c utils/payload_io.c core/segment.c $(wildcard spwstub/*.c)
TEST_ARG := ""
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

CFLAGS ?=

$(BUILD_DIR)/$(TARGET_EXEC): $(SRCS)
	$(MKDIR_P) $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRCS) -o $@

.PHONY: all clean run mem pack dbg

all: $(BUILD_DIR)/$(TARGET_EXEC)

clean:
	$(RM) -r $(BUILD_DIR) $(DEBUG_DIR)
	$(RM) pipes.log events.log
	$(RM) pa1.tar.gz

run: $(BUILD_DIR)/$(TARGET_EXEC)
	$(BUILD_DIR)/$(TARGET_EXEC)
	
test: $(BUILD_DIR)/$(TARGET_EXEC)
	echo $(TEST_ARG) | $(BUILD_DIR)/$(TARGET_EXEC)

mem: $(BUILD_DIR)/$(TARGET_EXEC)
	valgrind --tool=memcheck --child-silent-after-fork=yes -s --track-origins=yes $(BUILD_DIR)/$(TARGET_EXEC) -p $(ARGS)

mem_child: $(BUILD_DIR)/$(TARGET_EXEC)
	valgrind --tool=memcheck -s --track-origins=yes $(BUILD_DIR)/$(TARGET_EXEC) -p $(ARGS)

dbg: $(SRCS)
	$(MKDIR_P) $(DEBUG_DIR)
	cd $(DEBUG_DIR); $(CC) $(CFLAGS) -g $(addprefix ../,$(SRCS))

pack: clean
	mkdir ~/pa1
	cp -r * ~/pa1
	# tar czf ~/pa1.tar.gz ~/pa1

MKDIR_P ?= mkdir -p
