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

#include <math.h>
#include <string.h>

#define PROCESSORS 3

#define uchar unsigned char
#define FOPENB 1

static volatile int flag = -1;

static volatile uchar *in, *out;
static volatile int i, x_size, y_size;

#define exit_error(IFB, IFC)                                                                       \
    {                                                                                              \
        fprintf(stderr, IFB, IFC);                                                                 \
        exit(0);                                                                                   \
    }

/* {{{ get_image(filename,in,x_size,y_size) */
/* {{{ int getint(fp) derived from XV */
int getint(fd) FILE *fd;
{
    int c, i;
    // char dummy[10000];
    char *dummy = (char *)malloc(10000 * sizeof(char));
    c = getc(fd);
    while (1) {         /* find next integer */
        if (c == '#') { /* if we're at a comment, read to end of line */
            dummy = fgets(dummy, 9000, fd);
        }
        if (c == EOF) { exit_error("Image %s not binary PGM.\n", "is"); }
        if (c >= '0' && c <= '9') { break; /* found what we were looking for */ }
        c = getc(fd);
    }

    /* we're at the start of a number, continue until we hit a non-number */
    i = 0;
    while (1) {
        i = (i * 10) + (c - '0');
        c = getc(fd);
        if (c == EOF) { return (i); }
        if (c < '0' || c > '9') { break; }
    }
    return (i);
}
/* }}} */

void get_image(filename, in, x_size, y_size) char filename[200];
unsigned char **in;
int *x_size, *y_size;
{
    FILE *fd;
    char header[100];
    int tmp;
#ifdef FOPENB
    if ((fd = fopen(filename, "rb")) == NULL)
#else
    if ((fd = fopen(filename, "r")) == NULL)
#endif
        exit_error("Can't input image %s.\n", filename);
    /* {{{ read header */
    header[0] = fgetc(fd);
    header[1] = fgetc(fd);
    if (!(header[0] == 'P' && header[1] == '5')) {
        exit_error("Image %s does not have binary PGM header.\n", filename);
    }
    *x_size = getint(fd);
    *y_size = getint(fd);
    tmp = getint(fd);
    /* }}} */
    *in = (uchar *)malloc(*x_size * *y_size);
    if (fread(*in, 1, *x_size * *y_size, fd) == 0) {
        exit_error("Image %s is wrong size.\n", filename);
    }
    fclose(fd);
}
/* }}} */

/* {{{ put_image(filename,in,x_size,y_size) */
void put_image(filename, in, x_size, y_size) char filename[100], *in;
int x_size, y_size;
{
    FILE *fd;
#ifdef FOPENB
    if ((fd = fopen(filename, "wb")) == NULL)
#else
    if ((fd = fopen(filename, "w")) == NULL)
#endif
        exit_error("Can't output image%s.\n", filename);
    fprintf(fd, "P5\n");
    fprintf(fd, "%d %d\n", x_size, y_size);
    fprintf(fd, "255\n");
    if (fwrite(in, x_size * y_size, 1, fd) != 1) exit_error("Can't write image %s.\n", filename);
    fclose(fd);
}
/* }}} */

void *convolution(int myID, uchar *in, uchar *out, int x_size, int y_size) {
    int i, j, x;

    const int kernel[3][3] = {{-1, -1, -1}, {-1, 8, -1}, {-1, -1, -1}};

    for (i = 1 + (y_size - 1 - 1) / PROCESSORS * myID;
         i < 1 + (y_size - 1 - 1) / PROCESSORS * (myID + 1) +
                 (myID + 1 == PROCESSORS && (y_size - 1 - 1) % PROCESSORS != 0
                      ? (y_size - 1 - 1) % PROCESSORS
                      : 0);
         i++) {
        for (j = 1; j < x_size - 1; j++) {

            x = (in[((i - 1) * x_size) + (j)] * kernel[0][0]) +
                (in[((i - 1) * x_size) + (j)] * kernel[0][1]) +
                (in[((i - 1) * x_size) + (j)] * kernel[0][2]) +

                (in[((i)*x_size) + (j)] * kernel[1][0]) + (in[((i)*x_size) + (j)] * kernel[1][1]) +
                (in[((i)*x_size) + (j)] * kernel[1][2]) +

                (in[((i + 1) * x_size) + (j)] * kernel[2][0]) +
                (in[((i + 1) * x_size) + (j)] * kernel[2][1]) +
                (in[((i + 1) * x_size) + (j)] * kernel[2][2]);

            if (x > 255) {
                x = 255;
            } else if (x < 0) {
                x = 0;
            }

            out[i * x_size + j] = x;
        }
    }

    return 1;
}

int main0(int myId) {
    if (myId == 0) {
        /* Read image data */
        get_image("input_large2.pgm", &in, &x_size, &y_size);

        out = (uchar *)malloc(x_size * y_size);
        memset(out, 128, x_size * y_size); /* note not set to zero */
        flag = 0;
    }

    while (flag == -1) {}

    flag++;
    convolution(myId, in, out, x_size, y_size);
    flag--;

    while (flag != 0) {}

    if (myId == 0) {
        put_image("output.pgm", out, x_size, y_size);
        free(out);
    }
    return 1;
}

int main(int argc, char **argv) {

    int id = impProcessorId();

    printf("CPU %d starting...\n", id);

    switch (id) {

        case 0:
        case 1:
        case 2: main0(id); break;
    }

    return 1;
}
