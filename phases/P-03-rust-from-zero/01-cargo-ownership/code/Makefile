RUSTC=rustc
EDITION=--edition 2021
BUILD=build
all: $(BUILD)/own
$(BUILD)/own: main.rs
	@mkdir -p $(BUILD)
	$(RUSTC) $(EDITION) -D warnings main.rs -o $(BUILD)/own
run: $(BUILD)/own
	./$(BUILD)/own
test: all
	./$(BUILD)/own | grep -q "still-have=ada lovelace" && echo "PASS ownership-verbs"
	$(RUSTC) $(EDITION) --test main.rs -o $(BUILD)/own-test && ./$(BUILD)/own-test
clean:
	rm -rf $(BUILD)
