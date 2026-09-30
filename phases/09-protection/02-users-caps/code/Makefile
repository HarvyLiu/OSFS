CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/permsdemo
$(BUILD)/permsdemo: perms.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) perms.c -o $(BUILD)/permsdemo
run: $(BUILD)/permsdemo
	./$(BUILD)/permsdemo
test: all
	./$(BUILD)/permsdemo | grep -q "alice-w=ok bob-w=DENY bob-r=ok eve-r=DENY root-w=ok op-raw=ok" && ./$(BUILD)/permsdemo | grep -q "eve-raw=DENY" && echo "PASS users-caps"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_perms && ./$(BUILD)/test_perms
clean:
	rm -rf $(BUILD)
