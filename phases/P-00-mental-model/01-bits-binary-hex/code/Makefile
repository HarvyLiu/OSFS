CC=cc
CFLAGS=-Wall -Werror -std=c11
BUILD=build
all: $(BUILD)/bits
$(BUILD)/bits: main.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) main.c -o $(BUILD)/bits
run: $(BUILD)/bits
	./$(BUILD)/bits
test: $(BUILD)/bits
	./$(BUILD)/bits | grep -q "1<<4 = 16" && echo "PASS bits"
clean:
	rm -rf $(BUILD)
