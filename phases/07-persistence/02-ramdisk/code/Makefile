CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/rddemo
$(BUILD)/rddemo: demo.c ramdisk.c ramdisk.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) demo.c ramdisk.c -o $(BUILD)/rddemo
run: $(BUILD)/rddemo
	./$(BUILD)/rddemo
test: all
	./$(BUILD)/rddemo | grep -q "pre-crash=82" && ./$(BUILD)/rddemo | grep -q "post-crash=0" && echo "PASS ramdisk"
	$(CC) $(CFLAGS) tests/test_main.c ramdisk.c -o $(BUILD)/test_rd && ./$(BUILD)/test_rd
clean:
	rm -rf $(BUILD)
