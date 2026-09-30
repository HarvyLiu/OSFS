CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
UNAME := $(shell uname)
all: $(BUILD)/layout
ifeq ($(UNAME),Linux)
all: $(BUILD)/map
endif
$(BUILD)/layout: layout.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) layout.c -o $(BUILD)/layout
$(BUILD)/map: map.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) map.c -o $(BUILD)/map
run: all
	./$(BUILD)/layout
ifeq ($(UNAME),Linux)
	./$(BUILD)/map
endif
test: $(BUILD)/layout
	./$(BUILD)/layout | grep -q "heap-below-stack=" && echo "PASS address-spaces"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_as && ./$(BUILD)/test_as
clean:
	rm -rf $(BUILD)
