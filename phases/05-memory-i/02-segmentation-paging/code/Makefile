CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/trans
$(BUILD)/trans: trans.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) trans.c -o $(BUILD)/trans
run: $(BUILD)/trans
	./$(BUILD)/trans
test: all
	./$(BUILD)/trans | grep -q "0x20100" && ./$(BUILD)/trans | grep -q "0x7abc" && ./$(BUILD)/trans | grep -q "0x9123" && echo "PASS seg-paging"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_trans && ./$(BUILD)/test_trans
clean:
	rm -rf $(BUILD)
