CC=cc
CFLAGS=-Wall -Werror -std=c11
BUILD=build
all: $(BUILD)/addr
$(BUILD)/addr: main.c addr.s
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) main.c addr.s -o $(BUILD)/addr
run: $(BUILD)/addr
	./$(BUILD)/addr
trace: $(BUILD)/addr
	gdb -batch -ex 'break scaled_get' -ex run -ex si -ex 'info registers eax' -ex continue ./$(BUILD)/addr
test: all
	./$(BUILD)/addr | grep -q "scaled=30 mul3=21 state=2" && echo "PASS lea-addressing"
	$(CC) $(CFLAGS) tests/test_main.c addr.s -o $(BUILD)/test_addr && ./$(BUILD)/test_addr
clean:
	rm -rf $(BUILD)
