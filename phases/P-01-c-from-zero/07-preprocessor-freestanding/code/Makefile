CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/macros
$(BUILD)/macros: macros.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) macros.c -o $(BUILD)/macros
run: $(BUILD)/macros
	./$(BUILD)/macros
release: macros.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -DNDEBUG -O2 macros.c -o $(BUILD)/macros-release
	./$(BUILD)/macros-release
freestanding: bare.c
	@mkdir -p $(BUILD)
	$(CC) -ffreestanding -nostdlib -c bare.c -o $(BUILD)/bare.o
	nm $(BUILD)/bare.o | grep -q " add" && echo "PASS freestanding (add survives, no libc)"
test: all
	./$(BUILD)/macros | grep -q "min=4" && echo "PASS preprocessor-freestanding"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_pp && ./$(BUILD)/test_pp
clean:
	rm -rf $(BUILD)
