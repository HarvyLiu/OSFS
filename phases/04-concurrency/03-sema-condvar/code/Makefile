CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11 -pthread
BUILD=build
all: $(BUILD)/pc $(BUILD)/order
$(BUILD)/pc: pc.c sem.c sem.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) pc.c sem.c -o $(BUILD)/pc -pthread
$(BUILD)/order: order.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) order.c -o $(BUILD)/order -pthread
run: $(BUILD)/pc $(BUILD)/order
	./$(BUILD)/pc
	./$(BUILD)/order
test: all
	./$(BUILD)/pc | grep -q "consumed sum=19900 want=19900" && ./$(BUILD)/order | grep -q "consistent path ok" && echo "PASS sema-condvar"
	$(CC) $(CFLAGS) tests/test_main.c sem.c -o $(BUILD)/test_sync -pthread && ./$(BUILD)/test_sync
clean:
	rm -rf $(BUILD)
