CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/jdemo
$(BUILD)/jdemo: demo.c journal.c journal.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) demo.c journal.c -o $(BUILD)/jdemo
run: $(BUILD)/jdemo
	./$(BUILD)/jdemo
test: all
	./$(BUILD)/jdemo | grep -q "crash@0: blk3=- blk5=- CONSISTENT" && ./$(BUILD)/jdemo | grep -q "crash@1: blk3=A blk5=B CONSISTENT" && ./$(BUILD)/jdemo | grep -q "crash@2: blk3=A blk5=B CONSISTENT" && echo "PASS journaling"
	$(CC) $(CFLAGS) tests/test_main.c journal.c -o $(BUILD)/test_journal && ./$(BUILD)/test_journal
clean:
	rm -rf $(BUILD)
