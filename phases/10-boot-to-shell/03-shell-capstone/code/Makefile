AS=as
CC=gcc
LD=ld
QEMU=qemu-system-x86_64
BUILD=build
CFLAGS=-m32 -ffreestanding -O1 -Wall -Werror -Wextra -std=c11
all: $(BUILD)/os.bin
$(BUILD)/kernel.bin: kentry.s irq.s kernel.c link.ld
	@mkdir -p $(BUILD)
	$(AS) --32 kentry.s -o $(BUILD)/kentry.o
	$(AS) --32 irq.s -o $(BUILD)/irq.o
	$(CC) $(CFLAGS) -c kernel.c -o $(BUILD)/kernel.o
	$(LD) -m elf_i386 -T link.ld $(BUILD)/kentry.o $(BUILD)/irq.o $(BUILD)/kernel.o --oformat binary -o $(BUILD)/kernel.bin
$(BUILD)/boot.bin: boot.s $(BUILD)/kernel.bin
	@mkdir -p $(BUILD)
	SECTORS=$$(( ($$(stat -c%s $(BUILD)/kernel.bin) + 511) / 512 )); \
	$(AS) --32 --defsym LOAD_SECTORS=$$SECTORS boot.s -o $(BUILD)/boot.o
	$(LD) -m elf_i386 -Ttext 0x7c00 --oformat binary $(BUILD)/boot.o -o $(BUILD)/boot.bin
$(BUILD)/os.bin: $(BUILD)/boot.bin $(BUILD)/kernel.bin
	cat $(BUILD)/boot.bin $(BUILD)/kernel.bin > $(BUILD)/os.bin
qemu: $(BUILD)/os.bin
	timeout 5 $(QEMU) -drive format=raw,file=$(BUILD)/os.bin -nographic || test $$? -eq 124
test:
	python3 tests/test_shell.py -v
clean:
	rm -rf $(BUILD)
