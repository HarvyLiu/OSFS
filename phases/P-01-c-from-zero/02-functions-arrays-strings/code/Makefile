CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/fn $(BUILD)/libc
$(BUILD)/fn: main.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) main.c -o $(BUILD)/fn
$(BUILD)/libc: libc.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) libc.c -o $(BUILD)/libc
run: $(BUILD)/fn
	./$(BUILD)/fn
	./$(BUILD)/libc
test: all
	./$(BUILD)/fn | grep -q "name=osfs len=4" && ./$(BUILD)/libc | grep -q "libc: osfs 4 0" && echo "PASS fn-arrays-strings"
clean:
	rm -rf $(BUILD)
