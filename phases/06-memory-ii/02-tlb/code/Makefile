CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/tlb
$(BUILD)/tlb: tlb.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) tlb.c -o $(BUILD)/tlb
run: $(BUILD)/tlb
	./$(BUILD)/tlb
test: all
	./$(BUILD)/tlb | grep -q "hits=2 misses=6 eat=176" && echo "PASS tlb"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_tlb && ./$(BUILD)/test_tlb
clean:
	rm -rf $(BUILD)
