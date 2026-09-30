CC=cc
CFLAGS=-Wall -Werror -std=c11
BUILD=build
DEMO=$(BUILD)/forkdemo

all: $(DEMO)

$(DEMO): main.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) main.c -o $(DEMO)

run: $(DEMO)
	./$(DEMO)

test: $(DEMO)
	./$(DEMO) | grep -q "exited=0" && echo "PASS fork/exec/wait"

clean:
	rm -rf $(BUILD)
