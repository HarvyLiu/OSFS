CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/split
$(BUILD)/split: main.c vec.c vec.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) main.c vec.c -o $(BUILD)/split
run: $(BUILD)/split
	./$(BUILD)/split
test: all
	./$(BUILD)/split | grep -q "len=6 cap=8 first=0 last=50" && echo "PASS headers-make"
	$(CC) $(CFLAGS) tests/test_main.c vec.c -o $(BUILD)/test_split && ./$(BUILD)/test_split
clean:
	rm -rf $(BUILD)
