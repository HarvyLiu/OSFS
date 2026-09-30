CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11 -pthread
BUILD=build
all: $(BUILD)/race
$(BUILD)/race: race.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) race.c -o $(BUILD)/race -pthread
run: $(BUILD)/race
	./$(BUILD)/race
test: all
	./$(BUILD)/race | grep -q "mutex: got=400000 want=400000" && echo "PASS threads-races"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_race -pthread && ./$(BUILD)/test_race
clean:
	rm -rf $(BUILD)
