#ifndef HAL_H
#define HAL_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Implementado pela PLATAFORMA (SDL, placa...). O núcleo só chama.
void hal_draw_line(uint8_t y, const uint8_t *shades160);  // uma linha pronta (tons 0..3)
void hal_debug_putc(char c);                              // serial/UART de depuração

// Áudio: o núcleo gera amostras estéreo de 16 bits intercaladas (L,R,L,R...)
// nesta taxa fixa, e a plataforma as toca.
#define HAL_AUDIO_RATE 44100
void hal_audio_push(const int16_t *stereo, unsigned frames);

// Save de bateria: o núcleo só entrega/recebe um bloco de bytes (RAM do cartucho + relógio).
// Onde isso mora (arquivo .sav no PC, flash ou cartão SD na placa) é problema da plataforma.
size_t   hal_save_load(uint8_t *buf, size_t cap);        // devolve quantos bytes leu (0 = não há save)
bool     hal_save_store(const uint8_t *buf, size_t len); // true se gravou
uint64_t hal_time_unix(void);                            // segundos desde 1970; 0 = placa sem hora real
#endif
