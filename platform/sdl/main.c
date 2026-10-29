#include <stdio.h>
#include "emu.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file.gb\n", argv[0]);
        return 1;
    }

    return emu_run(argv[1]);
}