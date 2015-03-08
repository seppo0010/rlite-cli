#include <stdio.h>
#include <stdlib.h>
#include "rdb2rldf.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage %s <source.rdb> <target.rld> \n", argv[0]);
        return 1;
    }
    if (rdb2rld(argv[1], argv[2])) {
        return 1;
    }
    return 0;
}
