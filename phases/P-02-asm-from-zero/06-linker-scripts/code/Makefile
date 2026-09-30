CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/sections
$(BUILD)/sections: sections.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) sections.c -o $(BUILD)/sections
run: $(BUILD)/sections
	./$(BUILD)/sections
map: $(BUILD)/sections
	objdump -h $(BUILD)/sections | grep -E "text|data|bss|rodata|mysec"
	nm $(BUILD)/sections | grep -E " (main|init_global|zero_global|mysec_var)$$"
test: all
	./$(BUILD)/sections | grep -q "values: 41 0 7 99 5 1" && nm $(BUILD)/sections | grep -q "mysec_var" && echo "PASS linker-scripts"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_link && ./$(BUILD)/test_link
clean:
	rm -rf $(BUILD)
