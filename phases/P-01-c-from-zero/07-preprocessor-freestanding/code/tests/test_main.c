// test_main.c -- 5 checks: safe MIN, documented BAD trap, stringize, layout.
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define BAD(a, b) a<b?a:b
#define STR_(x) #x
#define STR(x) STR_(x)

int main(void) {
    int x = 3, y = 4;
    assert(MIN(x + 1, y) == 4);   // parenthesized: correct
    assert(MIN(2, 3) * 10 == 20); // parens protect the whole result
    assert(BAD(2, 3) * 10 == 2);  // trap: expands to 2<3?2:3*10 -> 2, not 20
    assert(strcmp(STR(1 + 1), "1 + 1") == 0);  // two-step stringize expands then pastes (spacing as written)
    assert(sizeof(int) >= 2);     // static_assert's runtime echo
    printf("all preprocessor checks pass\n");
    return 0;
}
