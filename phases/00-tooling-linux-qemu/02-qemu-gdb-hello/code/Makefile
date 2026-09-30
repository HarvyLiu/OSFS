CC=gcc
CFLAGS=-m32 -ffreestanding -O2 -Wall -Werror -nostdlib -fno-pie -no-pie
BUILD=build
ELF=$(BUILD)/kernel.elf

all: $(ELF)

$(ELF): boot.s kernel.c linker.ld
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -T linker.ld boot.s kernel.c -o $(ELF)

qemu: $(ELF)
	timeout 5 qemu-system-x86_64 -nographic -kernel $(ELF) || test $$? -eq 124

qemu32: $(ELF)
	timeout 5 qemu-system-i386 -nographic -kernel $(ELF) || test $$? -eq 124

debug:
	@echo "terminal 1: qemu-system-x86_64 -nographic -kernel $(ELF) -S -s"
	@echo "terminal 2: gdb -ex 'target remote :1234' -ex 'break kernel_main' -ex continue $(ELF)"

test:
	python3 -m unittest discover -s tests -v

sim:
	cc -Wall -Werror -std=c11 sim_serial.c -o /tmp/sim_serial && /tmp/sim_serial

clean:
	rm -rf $(BUILD)
