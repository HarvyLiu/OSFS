CC=cc
CFLAGS=-Wall -Werror -std=c11
BUILD=build
all: $(BUILD)/ctx $(BUILD)/switch.o
$(BUILD)/ctx: ctx.c switch.s
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) ctx.c switch.s -o $(BUILD)/ctx
$(BUILD)/switch.o: switch.s
	@mkdir -p $(BUILD)
	$(CC) -Wall -c switch.s -o $(BUILD)/switch.o
run: $(BUILD)/ctx
	./$(BUILD)/ctx
test: all
	./$(BUILD)/ctx | grep -q "done steps=3,3" && nm $(BUILD)/switch.o | grep -q "get_rsp" && echo "PASS ctx-switch"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_ctx && ./$(BUILD)/test_ctx
clean:
	rm -rf $(BUILD)
