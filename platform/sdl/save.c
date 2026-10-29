#include <stdio.h>
#include <string.h>
#include <time.h>
#include "hal.h"
#include "save_sdl.h"

static char save_path[4096];

void save_set_path(const char *rom_path) {
    save_path[0] = '\0';
    if (!rom_path) return;
    size_t len = strlen(rom_path);
    if (len + 5 > sizeof save_path) return;              
    memcpy(save_path, rom_path, len + 1);

    char *slash = strrchr(save_path, '/');
    char *dot   = strrchr(save_path, '.');
    if (dot && (!slash || dot > slash)) *dot = '\0';     
    strcat(save_path, ".sav");
}

size_t hal_save_load(uint8_t *buf, size_t cap) {
    if (!save_path[0]) return 0;
    FILE *f = fopen(save_path, "rb");
    if (!f) return 0;                               
    size_t n = fread(buf, 1, cap, f);
    fclose(f);
    fprintf(stderr, "Save carregado: %s (%zu bytes)\n", save_path, n);
    return n;
}

bool hal_save_store(const uint8_t *buf, size_t len) {
    if (!save_path[0]) return false;
    char tmp[sizeof save_path + 8];
    snprintf(tmp, sizeof tmp, "%s.tmp", save_path);

    FILE *f = fopen(tmp, "wb");
    if (!f) return false;
    bool ok = fwrite(buf, 1, len, f) == len;
    ok = (fclose(f) == 0) && ok;
    if (!ok) { remove(tmp); return false; }
    return rename(tmp, save_path) == 0;
}

uint64_t hal_time_unix(void) {
    return (uint64_t)time(NULL);
}
