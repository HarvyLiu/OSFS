CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/irqdemo $(BUILD)/irq.o
$(BUILD)/irqdemo: demo.c idt.c idt.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) demo.c idt.c -o $(BUILD)/irqdemo
$(BUILD)/irq.o: irq.s
	@mkdir -p $(BUILD)
	$(CC) -Wall -c irq.s -o $(BUILD)/irq.o
run: $(BUILD)/irqdemo
	./$(BUILD)/irqdemo
test: all
	./$(BUILD)/irqdemo | grep -q "ticks=3 after unmask" && nm $(BUILD)/irq.o | grep -q "irq_shape_note" && echo "PASS interrupts-idt"
	$(CC) $(CFLAGS) tests/test_main.c idt.c -o $(BUILD)/test_idt && ./$(BUILD)/test_idt
clean:
	rm -rf $(BUILD)
