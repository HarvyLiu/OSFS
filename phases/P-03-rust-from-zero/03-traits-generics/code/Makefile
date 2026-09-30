RUSTC=rustc
EDITION=--edition 2021
BUILD=build
all: $(BUILD)/traits-rs
$(BUILD)/traits-rs: main.rs
	@mkdir -p $(BUILD)
	$(RUSTC) $(EDITION) -D warnings main.rs -o $(BUILD)/traits-rs
run: $(BUILD)/traits-rs
	./$(BUILD)/traits-rs
test: all
	./$(BUILD)/traits-rs | grep -q "announce: pcb 1 (init)" && ./$(BUILD)/traits-rs | grep -q "display: Job#7(burst 3)" && echo "PASS traits-generics"
	$(RUSTC) $(EDITION) --test main.rs -o $(BUILD)/traits-test && ./$(BUILD)/traits-test
clean:
	rm -rf $(BUILD)
