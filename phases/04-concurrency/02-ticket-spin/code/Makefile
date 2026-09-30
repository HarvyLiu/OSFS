CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11 -pthread
BUILD=build
all: $(BUILD)/ticketdemo
$(BUILD)/ticketdemo: demo.c ticket.c ticket.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) demo.c ticket.c -o $(BUILD)/ticketdemo -pthread
run: $(BUILD)/ticketdemo
	./$(BUILD)/ticketdemo
test: all
	./$(BUILD)/ticketdemo | grep -q "ticket: got=400000 want=400000" && echo "PASS ticket-spin"
	$(CC) $(CFLAGS) tests/test_main.c ticket.c -o $(BUILD)/test_ticket -pthread && ./$(BUILD)/test_ticket
clean:
	rm -rf $(BUILD)
