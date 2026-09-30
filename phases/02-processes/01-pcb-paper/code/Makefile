CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/pcbpaper
$(BUILD)/pcbpaper: pcb.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) pcb.c -o $(BUILD)/pcbpaper
run: $(BUILD)/pcbpaper
	./$(BUILD)/pcbpaper
test: all
	./$(BUILD)/pcbpaper | grep -q "pc=0x400100" && ./$(BUILD)/pcbpaper | grep -q "r7=107" && echo "PASS pcb-paper"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_pcbpaper && ./$(BUILD)/test_pcbpaper
clean:
	rm -rf $(BUILD)
