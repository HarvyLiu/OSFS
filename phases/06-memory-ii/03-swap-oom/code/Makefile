CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/swapdemo $(BUILD)/oomdemo
$(BUILD)/swapdemo: main.c demand.c demand.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) main.c demand.c -o $(BUILD)/swapdemo
$(BUILD)/oomdemo: oom.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) oom.c -o $(BUILD)/oomdemo
run: all
	./$(BUILD)/swapdemo
	./$(BUILD)/oomdemo 3
test: all
	./$(BUILD)/swapdemo | grep -q "faults=9 evictions=6 writebacks=1" && ./$(BUILD)/oomdemo 3 | grep -q "allocated=3MB requested=3MB" && echo "PASS swap-oom"
	$(CC) $(CFLAGS) tests/test_main.c demand.c -o $(BUILD)/test_swap && ./$(BUILD)/test_swap
clean:
	rm -rf $(BUILD)
