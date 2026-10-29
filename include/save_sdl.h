#ifndef SAVE_SDL_H
#define SAVE_SDL_H

// Define onde fica o save: o mesmo caminho da ROM, trocando a extensão por .sav.
// Chamar ANTES de emu_init.
void save_set_path(const char *rom_path);

#endif
