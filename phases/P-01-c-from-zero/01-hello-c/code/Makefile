CC=cc
CFLAGS=-Wall -Werror -std=c11
BUILD=build

all: $(BUILD)/hello $(BUILD)/types

$(BUILD)/hello: main.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) main.c -o $(BUILD)/hello

$(BUILD)/types: types.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) types.c -o $(BUILD)/types

run: $(BUILD)/hello
	./$(BUILD)/hello

types: $(BUILD)/types
	./$(BUILD)/types

test: all
	./$(BUILD)/hello | grep -q "square=16" && ./$(BUILD)/types | grep -q "c=A" && echo "PASS hello-c"

clean:
	rm -rf $(BUILD)
