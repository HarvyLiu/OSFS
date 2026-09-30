CC=cc
CFLAGS=-Wall -Werror -std=c11
BUILD=build
SIM=$(BUILD)/sim

all: $(SIM)

$(SIM): main.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) main.c -o $(SIM)

run: $(SIM)
	./$(SIM)

test: $(SIM)
	./$(SIM) | grep -q "heap int=123" && echo "PASS pointers sim"

clean:
	rm -rf $(BUILD)
