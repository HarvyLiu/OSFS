CC=cc
CFLAGS=-Wall -Werror -std=c11
BUILD=build
all: $(BUILD)/sched
$(BUILD)/sched: sched.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) sched.c -o $(BUILD)/sched
run: $(BUILD)/sched
	./$(BUILD)/sched
test: all
	./$(BUILD)/sched | grep -q "FIFO order: P1 P2 P3" && ./$(BUILD)/sched | grep -q "SJF order: P1 P3 P2" && ./$(BUILD)/sched | grep -q "RRq2" && echo "PASS sched-policies"
	$(CC) $(CFLAGS) tests/test_main.c -o $(BUILD)/test_sched -lm && ./$(BUILD)/test_sched
clean:
	rm -rf $(BUILD)
