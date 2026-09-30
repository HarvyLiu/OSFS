CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/vsfsdemo
$(BUILD)/vsfsdemo: demo.c vsfs.c vsfs.h rambackend.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) demo.c vsfs.c -o $(BUILD)/vsfsdemo
run: $(BUILD)/vsfsdemo
	./$(BUILD)/vsfsdemo
test: all
	./$(BUILD)/vsfsdemo | grep -q "read back: hello vsfs! (size 11)" && ./$(BUILD)/vsfsdemo | grep -q "blocks used: 8/64" && echo "PASS vsfs"
	$(CC) $(CFLAGS) tests/test_main.c vsfs.c -o $(BUILD)/test_vsfs && ./$(BUILD)/test_vsfs
clean:
	rm -rf $(BUILD)
