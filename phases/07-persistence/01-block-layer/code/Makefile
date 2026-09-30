CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/blkdemo
$(BUILD)/blkdemo: demo.c blk.c blk.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) demo.c blk.c -o $(BUILD)/blkdemo
run: $(BUILD)/blkdemo
	./$(BUILD)/blkdemo
test: all
	./$(BUILD)/blkdemo | grep -q "after-crash-1st-byte=0" && ./$(BUILD)/blkdemo | grep -q "after-flush-1st-byte=66" && echo "PASS block-layer"
	$(CC) $(CFLAGS) tests/test_main.c blk.c -o $(BUILD)/test_blk && ./$(BUILD)/test_blk
clean:
	rm -rf $(BUILD)
