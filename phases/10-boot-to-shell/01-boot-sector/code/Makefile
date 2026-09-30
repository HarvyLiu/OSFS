AS=as
LD=ld
QEMU=qemu-system-x86_64
BUILD=build
all: $(BUILD)/boot.bin
$(BUILD)/boot.bin: boot.s
	@mkdir -p $(BUILD)
	$(AS) --32 boot.s -o $(BUILD)/boot.o
	$(LD) -m elf_i386 -Ttext 0x7c00 --oformat binary $(BUILD)/boot.o -o $(BUILD)/boot.bin
qemu: $(BUILD)/boot.bin
	timeout 5 $(QEMU) -drive format=raw,file=$(BUILD)/boot.bin -nographic || test $$? -eq 124
test:
	python3 tests/test_boot.py -v
clean:
	rm -rf $(BUILD)
