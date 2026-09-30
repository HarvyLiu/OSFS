// main.c -- NOBOOST vs BOOST8 demo. Lesson docs/en.md.
#include <stdio.h>
#include "mlfq.h"

static void show(const char *tag, int boost) {
    mjob_t js[3] = {{1,0,3,0,0,0,0,0},{2,0,10,0,0,0,0,0},{3,0,3,0,0,0,0,0}};
    int order[3], no = 0;
    mlfq_run(js, 3, boost, order, &no);
    printf("%s comp: A=%d B=%d C=%d order %d-%d-%d\n", tag,
           js[0].comp, js[1].comp, js[2].comp, order[0], order[1], order[2]);
}

int main(void) {
    show("NOBOOST", 0);
    show("BOOST8", 8);
    return 0;
}
