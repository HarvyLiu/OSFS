CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/abstractions
$(BUILD)/abstractions: demo.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) demo.c -o $(BUILD)/abstractions
run: $(BUILD)/abstractions
	./$(BUILD)/abstractions
test: all
	./$(BUILD)/abstractions | grep -q "read back: hello from an abstraction" && test -s $(BUILD)/hello-osfs.txt && echo "PASS abstractions"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_abs && ./$(BUILD)/test_abs
clean:
	rm -rf $(BUILD)
