// macros.c -- preprocessor toolkit with teeth. Lesson docs/en.md.
#include <assert.h>
#include <stdio.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define STR_(x) #x
#define STR(x) STR_(x)

#ifndef NDEBUG
#define LOG(msg) printf("dbg: %s\n", msg)
#else
#define LOG(msg) ((void)0)
#endif

int main(void) {
    int x = 3, y = 4;
    printf("min=%d line=%d file=%s ver=%s\n",
           MIN(x + 1, y), __LINE__, __FILE__, STR(__STDC_VERSION__));
    LOG("visible in debug build");
    static_assert(sizeof(int) >= 2, "int too small for this course");
    return 0;
}
