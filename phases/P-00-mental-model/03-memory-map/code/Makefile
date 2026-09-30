CC=cc
# NOTE: no -Werror here: make_mistake's return-local-addr warning is the lesson.
CFLAGS=-Wall -Wextra -std=c11
BUILD=build
all: $(BUILD)/regions
$(BUILD)/regions: regions.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) regions.c -o $(BUILD)/regions
run: $(BUILD)/regions
	./$(BUILD)/regions
test: all
	./$(BUILD)/regions | grep -q "heap-val=7 s_persistent=2" && ./$(BUILD)/regions | grep -q "dangling: NEVER dereference" && echo "PASS memory-map"
	$(CC) -Wall -Werror -Wextra -std=c11 tests/test_main.c -o $(BUILD)/test_regions && ./$(BUILD)/test_regions
clean:
	rm -rf $(BUILD)
