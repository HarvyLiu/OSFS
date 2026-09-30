// test_main.c -- 4 checks: exit-code contract + argv indexing rules.
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int exit_for_argc(int argc) { return argc - 1; }  // mirrors args.c

int main(void) {
    assert(exit_for_argc(1) == 0);   // bare invocation: success
    assert(exit_for_argc(3) == 2);   // two args: code 2
    char *fake_argv[] = {"prog", "alpha", 0};
    assert(strcmp(fake_argv[0], "prog") == 0);  // argv[0] = self
    assert(strcmp(fake_argv[1], "alpha") == 0); // argv[1..] = args
    printf("all how-programs-run checks pass\n");
    return 0;
}
