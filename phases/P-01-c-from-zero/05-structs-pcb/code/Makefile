CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/pcb $(BUILD)/offsets
$(BUILD)/pcb: pcb.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) pcb.c -o $(BUILD)/pcb
$(BUILD)/offsets: offsets.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) offsets.c -o $(BUILD)/offsets
run: $(BUILD)/pcb $(BUILD)/offsets
	./$(BUILD)/pcb
	./$(BUILD)/offsets
test: all
	./$(BUILD)/pcb | grep -q "reaped pid 2" && ./$(BUILD)/pcb | grep -q "slots used=1" && echo "PASS structs-pcb"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_pcb && ./$(BUILD)/test_pcb
clean:
	rm -rf $(BUILD)
