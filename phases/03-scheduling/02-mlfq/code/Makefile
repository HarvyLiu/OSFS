CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/mlfq
$(BUILD)/mlfq: main.c mlfq_engine.c mlfq.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) main.c mlfq_engine.c -o $(BUILD)/mlfq
run: $(BUILD)/mlfq
	./$(BUILD)/mlfq
test: all
	./$(BUILD)/mlfq | grep -q "NOBOOST comp: A=7 B=16 C=12" && ./$(BUILD)/mlfq | grep -q "BOOST8 comp: A=7 B=16 C=11" && echo "PASS mlfq"
	$(CC) $(CFLAGS) tests/test_main.c mlfq_engine.c -o $(BUILD)/test_mlfq && ./$(BUILD)/test_mlfq
clean:
	rm -rf $(BUILD)
