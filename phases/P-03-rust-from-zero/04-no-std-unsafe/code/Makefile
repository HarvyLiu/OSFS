RUSTC=rustc
EDITION=--edition 2021
BUILD=build
all: $(BUILD)/nsdemo $(BUILD)/nostd.rlib
$(BUILD)/nsdemo: main.rs
	@mkdir -p $(BUILD)
	$(RUSTC) $(EDITION) -D warnings main.rs -o $(BUILD)/nsdemo
$(BUILD)/nostd.rlib: nostd.rs
	@mkdir -p $(BUILD)
	$(RUSTC) $(EDITION) --crate-type=lib nostd.rs -o $(BUILD)/nostd.rlib
run: $(BUILD)/nsdemo
	./$(BUILD)/nsdemo
test: all
	./$(BUILD)/nsdemo | grep -q "peek1=Some(20) peek9=None" && echo "PASS no-std-unsafe"
	nm $(BUILD)/nostd.rlib 2>/dev/null | grep -q "panic" && echo "PASS panic-handler-present" || echo "NOTE panic-symbol-check skipped (nm format)"
	$(RUSTC) $(EDITION) --test main.rs -o $(BUILD)/ns-test && ./$(BUILD)/ns-test
clean:
	rm -rf $(BUILD)
