CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/heap
$(BUILD)/heap: heap.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) heap.c -o $(BUILD)/heap
$(BUILD)/heap-asan: heap.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -fsanitize=address,undefined heap.c -o $(BUILD)/heap-asan
run: $(BUILD)/heap
	./$(BUILD)/heap
asan: $(BUILD)/heap-asan
	./$(BUILD)/heap-asan
test: all
	./$(BUILD)/heap | grep -q "len=6 cap=8" && ./$(BUILD)/heap-asan | grep -q "len=6 cap=8" && echo "PASS malloc-heap"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_heap && ./$(BUILD)/test_heap
clean:
	rm -rf $(BUILD)
