RUSTC=rustc
EDITION=--edition 2021
BUILD=build
all: $(BUILD)/pcb-rs
$(BUILD)/pcb-rs: main.rs
	@mkdir -p $(BUILD)
	$(RUSTC) $(EDITION) -D warnings main.rs -o $(BUILD)/pcb-rs
run: $(BUILD)/pcb-rs
	./$(BUILD)/pcb-rs
test: all
	./$(BUILD)/pcb-rs | grep -q "is on-cpu," && ./$(BUILD)/pcb-rs | grep -q "table full" && echo "PASS rust-pcb"
	$(RUSTC) $(EDITION) --test main.rs -o $(BUILD)/pcb-test && ./$(BUILD)/pcb-test
clean:
	rm -rf $(BUILD)
