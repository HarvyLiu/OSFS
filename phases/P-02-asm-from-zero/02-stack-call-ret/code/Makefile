CC=cc
CFLAGS=-Wall -Werror -std=c11
BUILD=build
all: $(BUILD)/frames $(BUILD)/depth
$(BUILD)/frames: main.c frame.s
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) main.c frame.s -o $(BUILD)/frames
$(BUILD)/depth: depth.c rsp.s
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) depth.c rsp.s -o $(BUILD)/depth
run: $(BUILD)/frames $(BUILD)/depth
	./$(BUILD)/frames
	./$(BUILD)/depth
trace: $(BUILD)/frames
	gdb -batch -ex 'break frame_add' -ex run -ex bt -ex 'x/4xg $$rsp' -ex continue ./$(BUILD)/frames
test: all
	./$(BUILD)/frames | grep -q "frame_add(30,12)=42" && ./$(BUILD)/depth | grep -q "level2 rsp=" && echo "PASS stack-frames"
	$(CC) $(CFLAGS) tests/test_main.c frame.s rsp.s -o $(BUILD)/test_frames && ./$(BUILD)/test_frames
clean:
	rm -rf $(BUILD)
