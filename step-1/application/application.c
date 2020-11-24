/*
 *
 * Copyright (c) 2005-2013 Imperas Software Ltd., www.imperas.com
 *
 * The contents of this file are provided under the Software License
 * Agreement that you accepted before downloading this file.
 *
 * This source forms part of the Software and can be used for educational,
 * training, and demonstration purposes but cannot be used for derivative
 * works except in cases where the derivative works require OVP technology
 * to run.
 *
 * For open source models released under licenses that you can use for
 * derivative works, please visit www.OVPworld.org or www.imperas.com
 * for the location of the open source models.
 *
 */

#include "simulatorIntercepts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_VALUES 35

static volatile int flag = 1;
static volatile int fibres = 0;

int fib(int i) { return (i > 1) ? fib(i - 1) + fib(i - 2) : i; }

int even_numbers(int id) {
    int i;

    for (i = 0; i < NUM_VALUES; i += 2) {
        int result = fib(i);
        while (flag != id) {}
        printf("CPU %d: fib(%d)\n", id, i);
        fibres = result;
        flag = (i == (NUM_VALUES - 1)) ? -1 : 0;
    }

    while (flag != -2) {}

    printf("Saiu 1\n");
    return 1;
}

int odd_numbers(int id) {
    int i;

    for (i = 1; i < NUM_VALUES; i += 2) {
        int result = fib(i);
        while (flag != id) {}
        printf("CPU %d: fib(%d)\n", id, i);
        fibres = result;
        flag = (i == (NUM_VALUES - 1)) ? -1 : 0;
    }

    while (flag != -2) {}

    return 1;
}

int print_results(int id) {

    int i = 0, done = 0;

    do {
        int result;
        while (flag != id && flag != -1) {}
        result = fibres;
        done = (flag == -1);
        printf("CPU %d: result = %d\n", id, result);
        flag = ((++i % 2) == 0) ? 1 : 2;
    } while (!done);

    flag = -2;

    return 1;
}

int main(int argc, char **argv) {

    int id = impProcessorId();

    printf("CPU %d starting...\n", id);

    switch (id) {

        case 0: print_results(id); break;

        case 1: even_numbers(id); break;

        case 2: odd_numbers(id); break;
    }

    return 1;
}
