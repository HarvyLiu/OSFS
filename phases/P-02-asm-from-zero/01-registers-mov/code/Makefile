CC=cc
CFLAGS=-Wall -Werror -std=c11
BUILD=build
all: $(BUILD)/regs
$(BUILD)/regs: main.c add.s
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) main.c add.s -o $(BUILD)/regs
run: $(BUILD)/regs
	./$(BUILD)/regs
trace: $(BUILD)/regs
	gdb -batch -ex 'break add2' -ex run -ex 'info registers eax edi esi' -ex continue ./$(BUILD)/regs
test: all
	./$(BUILD)/regs | grep -q "add2(40,2)=42" && echo "PASS regs-mov"
	$(CC) $(CFLAGS) tests/test_main.c add.s -o $(BUILD)/test_regs && ./$(BUILD)/test_regs
clean:
	rm -rf $(BUILD)
