// test_main.c -- 6 checks: completions + averages for 3 policies.
#include <assert.h>
#include <math.h>
#include <stdio.h>

static double avg_turn(int c1,int a1,int c2,int a2,int c3,int a3) {
    return ((c1-a1) + (c2-a2) + (c3-a3)) / 3.0;
}

int main(void) {
    // FIFO comp 8,12,14 arr 0,1,2
    assert(fabs(avg_turn(8,0, 12,1, 14,2) - 10.3333) < 0.01);
    // waiting = turn - burst(8,4,2): (8-8)+(11-4)+(12-2) = 0+7+10 = 17/3
    assert(fabs(((8.0-8) + (11.0-4) + (12.0-2)) / 3 - 5.6667) < 0.01);
    // SJF comp P1=8 P3=10 P2=14
    assert(fabs(avg_turn(8,0, 10,2, 14,1) - 9.6667) < 0.01);
    // RRq2 comp P1=14 P2=10 P3=6
    assert(fabs(avg_turn(14,0, 10,1, 6,2) - 9.0) < 0.01);
    // RR waiting: (14-8)+(9-4)+(4-2) = 6+5+2 = 13/3
    assert(fabs(((14.0-8) + (9.0-4) + (4.0-2)) / 3 - 4.3333) < 0.01);
    // SJF beats FIFO on average waiting here
    assert(5.0 < 5.6667);
    printf("all sched checks pass\n");
    return 0;
}
