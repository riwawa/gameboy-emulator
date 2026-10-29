#ifndef UI_H
#define UI_H
#include <stdint.h>
#include <stdbool.h>

bool ui_init(const char *title);
void ui_present(void);   // mostra o último quadro (vindo do HAL)
bool ui_poll(void);                       // false = usuário quer fechar
void ui_wait_frame(void);                 // segura o emulador em ~59,7 fps
bool ui_screenshot(const char *path);     // salva a tela atual em .bmp
void ui_quit(void);
#endif