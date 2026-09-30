CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/ringsdemo
$(BUILD)/ringsdemo: rings.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) rings.c -o $(BUILD)/ringsdemo
run: $(BUILD)/ringsdemo
	./$(BUILD)/ringsdemo
test: all
	./$(BUILD)/ringsdemo | grep -q "syscall-gate=ok kdebug-gate=FAULT kdata=FAULT k-reads-u=ok u-reads-u=ok" && echo "PASS rings-gates"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_rings && ./$(BUILD)/test_rings
clean:
	rm -rf $(BUILD)
