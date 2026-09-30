CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/inline
$(BUILD)/inline: main.c asmops.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -O2 main.c -o $(BUILD)/inline
run: $(BUILD)/inline
	./$(BUILD)/inline
test: all
	./$(BUILD)/inline | grep -q "add=42" && ./$(BUILD)/inline | grep -Eq "vendor=.+" && echo "PASS inline-asm"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_inline && ./$(BUILD)/test_inline
clean:
	rm -rf $(BUILD)
