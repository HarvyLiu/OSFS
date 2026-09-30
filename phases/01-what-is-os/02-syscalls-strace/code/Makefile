CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/trapdemo
$(BUILD)/trapdemo: pid.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) pid.c -o $(BUILD)/trapdemo
run: $(BUILD)/trapdemo
	./$(BUILD)/trapdemo
test: all
	./$(BUILD)/trapdemo | grep -q "back=\[knock knock (via write)\]" && test -s $(BUILD)/trap-demo.txt && echo "PASS syscalls-strace"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_trap && ./$(BUILD)/test_trap
clean:
	rm -rf $(BUILD)
