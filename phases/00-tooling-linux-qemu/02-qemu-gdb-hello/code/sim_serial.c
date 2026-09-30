// sim_serial.c — host-side fake of the serial loop. No QEMU needed.
#include <stdio.h>
#include <string.h>

static char fake_serial[64];
static int pos = 0;
static void fake_outb(char c) { if (pos < 63) fake_serial[pos++] = c; }

int main(void) {
    const char *msg = "OSFS\n";
    for (int i = 0; msg[i]; i++) fake_outb(msg[i]);
    fake_serial[pos] = 0;
    printf("would-send-to-0x3F8: %s", fake_serial);
    return strcmp(fake_serial, "OSFS\n") != 0;
}
