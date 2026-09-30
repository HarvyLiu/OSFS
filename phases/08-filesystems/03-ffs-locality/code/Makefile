CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/ffsdemo
$(BUILD)/ffsdemo: main.c ffs.c ffs.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) main.c ffs.c -o $(BUILD)/ffsdemo
run: $(BUILD)/ffsdemo
	./$(BUILD)/ffsdemo
test: all
	./$(BUILD)/ffsdemo | grep -q "firstfit: span=14 gaps=14" && ./$(BUILD)/ffsdemo | grep -q "clustered: span=7 gaps=7" && echo "PASS ffs-locality"
	$(CC) $(CFLAGS) tests/test_main.c ffs.c -o $(BUILD)/test_ffs && ./$(BUILD)/test_ffs
clean:
	rm -rf $(BUILD)
