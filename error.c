#include <stdio.h>
#include <errno.h>

void panic(char *msg) {
    fprintf(stderr, "Error: %s: %s\n", msg, strerror(errno));
    exit(1);
}
