#include <stdio.h>
#include <stdlib.h>
#include "emu.h"
#include "ui.h"
#include "audio_sdl.h"
#include "save_sdl.h"

static uint8_t *read_file(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = n > 0 ? malloc((size_t)n) : NULL;
    if (buf && fread(buf, 1, (size_t)n, f) != (size_t)n) { free(buf); buf = NULL; }
    fclose(f);
    if (buf) *size = (size_t)n;
    return buf;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file.gb\n", argv[0]);
        return 1;
    }

    size_t size = 0;
    uint8_t *rom = read_file(argv[1], &size);
    if (!rom) {
        fprintf(stderr, "Failed to read ROM: %s\n", argv[1]);
        return 1;
    }
    save_set_path(argv[1]);                 
    if (!emu_init(rom, size)) {
        fprintf(stderr, "Failed to load ROM: %s\n", argv[1]);
        free(rom);
        return 1;
    }
    if (!ui_init(emu_title())) {
        emu_shutdown();
        free(rom);
        return 1;
    }

    if (!audio_init()) fprintf(stderr, "Sem audio: o jogo roda mudo.\n");

    int status = 0;
    while (ui_poll()) {
        if (!emu_run_frame()) {
            fprintf(stderr, "CPU stopped\n");
            status = 1;
            break;
        }
        ui_present();
        ui_wait_frame();
    }

    const emu_context *c = emu_get_context();
    printf("Instructions: %llu\n", (unsigned long long)c->ticks);
    printf("M-cycles: %llu\n", (unsigned long long)c->cycles);

    audio_quit();
    ui_quit();
    emu_shutdown();
    free(rom);
    return status;
}
