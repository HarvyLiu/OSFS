CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/ptdemo
$(BUILD)/ptdemo: main.c pt.c pt.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) main.c pt.c -o $(BUILD)/ptdemo
run: $(BUILD)/ptdemo
	./$(BUILD)/ptdemo
test: all
	./$(BUILD)/ptdemo | grep -q "0x7abc" && ./$(BUILD)/ptdemo | grep -q "0x100abc" && echo "PASS page-tables"
	$(CC) $(CFLAGS) tests/test_main.c pt.c -o $(BUILD)/test_pt && ./$(BUILD)/test_pt
clean:
	rm -rf $(BUILD)
