CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/cfs
$(BUILD)/cfs: cfs.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) cfs.c -o $(BUILD)/cfs
run: $(BUILD)/cfs
	./$(BUILD)/cfs
test: all
	./$(BUILD)/cfs | grep -q "task 3 nice=-5 weight=3121 ticks=41" && ./$(BUILD)/cfs | grep -q "task 2 nice=5 weight=335 ticks=5" && echo "PASS cfs-intuition"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_cfs && ./$(BUILD)/test_cfs
clean:
	rm -rf $(BUILD)
