RUSTC=rustc
EDITION=--edition 2021
BUILD=build
all: $(BUILD)/bump-rs
$(BUILD)/bump-rs: main.rs
	@mkdir -p $(BUILD)
	$(RUSTC) $(EDITION) -D warnings main.rs -o $(BUILD)/bump-rs
run: $(BUILD)/bump-rs
	./$(BUILD)/bump-rs
test: all
	./$(BUILD)/bump-rs | grep -q "aligned16=true" && ./$(BUILD)/bump-rs | grep -q "oom=None" && echo "PASS bump-allocator"
	$(RUSTC) $(EDITION) --test main.rs -o $(BUILD)/bump-test && ./$(BUILD)/bump-test
clean:
	rm -rf $(BUILD)
