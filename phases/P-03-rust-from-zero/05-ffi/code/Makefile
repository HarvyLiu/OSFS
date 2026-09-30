RUSTC=rustc
EDITION=--edition 2021
CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/ffi-demo $(BUILD)/caller.o $(BUILD)/abi-lib
$(BUILD)/ffi-demo: main.rs
	@mkdir -p $(BUILD)
	$(RUSTC) $(EDITION) -D warnings main.rs -o $(BUILD)/ffi-demo
$(BUILD)/caller.o: caller.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c caller.c -o $(BUILD)/caller.o
$(BUILD)/abi-lib: abi.rs
	@mkdir -p $(BUILD)
	$(RUSTC) $(EDITION) --crate-type=staticlib abi.rs -o $(BUILD)/abi-lib.lib
run: $(BUILD)/ffi-demo
	./$(BUILD)/ffi-demo
test: all
	./$(BUILD)/ffi-demo | grep -q "rsp_off=8" && echo "PASS ffi-fence"
	$(RUSTC) $(EDITION) --test main.rs -o $(BUILD)/ffi-test && ./$(BUILD)/ffi-test
clean:
	rm -rf $(BUILD)
