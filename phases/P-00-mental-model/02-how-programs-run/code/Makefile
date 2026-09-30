CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/hello $(BUILD)/args
$(BUILD)/hello: hello.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) hello.c -o $(BUILD)/hello
$(BUILD)/args: args.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) args.c -o $(BUILD)/args
run: all
	./$(BUILD)/hello
	./$(BUILD)/args alpha beta; echo "args-exit=$$?"
stages: hello.c
	@mkdir -p $(BUILD)
	$(CC) -E hello.c -o $(BUILD)/hello.i
	$(CC) -S hello.c -o $(BUILD)/hello.s
	$(CC) -c hello.c -o $(BUILD)/hello.o
test: all
	./$(BUILD)/hello | grep -q "hello stages" && ./$(BUILD)/args a b | grep -q "argc=3" && echo "PASS how-programs-run"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_stages && ./$(BUILD)/test_stages
clean:
	rm -rf $(BUILD)
