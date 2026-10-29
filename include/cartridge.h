#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    char title[17];
    uint8_t type;
    uint8_t rom_size_code;
    uint8_t ram_size_code;
} CartridgeHeader;

typedef enum { MBC_NONE, MBC_1, MBC_2, MBC_3, MBC_5 } MbcKind;

// Relógio em tempo real do MBC3. Conta em tempo de EMULAÇÃO (ciclos), não pelo relógio do PC.
typedef struct {
    uint8_t  s, m, h;       // segundos, minutos, horas (6, 6 e 5 bits)
    uint16_t d;             // contador de dias (9 bits)
    bool     halt;          // DH bit 6: relógio parado
    bool     carry;         // DH bit 7: o contador de dias estourou (fica ligado até o jogo limpar)
    uint32_t sub;           // ciclos-M dentro do segundo atual
    uint8_t  latched[5];    // cópia "congelada" que o jogo lê (S, M, H, DL, DH)
    uint8_t  latch_prev;    // último valor escrito em 6000-7FFF (latch = 0 e depois 1)
} Rtc;

typedef struct {
    const uint8_t *rom;   // só leitura: pode apontar para a flash
    bool owns_rom;        // true se cartridge_load alocou (então cartridge_free libera)
    size_t rom_size;
    uint32_t rom_banks;   // quantos bancos de 16 KB a ROM tem
    CartridgeHeader header;

    // RAM externa (opcional, fica no cartucho)
    uint8_t *ram;
    size_t ram_size;
    bool has_battery;     // a RAM (e o relógio) deveriam ser salvos

    // Imagem de save = RAM + (no MBC3 com relógio) 48 bytes do relógio no final.
    // 'ram' aponta para o início desta mesma área, então a RAM já fica "pronta para gravar".
    uint8_t *save;
    size_t   save_size;   // 0 se o cartucho não tem bateria
    bool     save_dirty;  // a RAM mudou desde o último export

    MbcKind mbc;
    bool ram_enable;      // 0000-1FFF: 0x0A habilita RAM (e o relógio, no MBC3)

    // MBC1
    uint8_t bank1;        // 5 bits baixos do banco de ROM (0 vira 1)
    uint8_t bank2;        // 2 bits: bits altos do banco de ROM ou banco de RAM
    uint8_t mode;         // 0 = simples, 1 = avançado

    // MBC3 / MBC5
    uint16_t rom_bank;    // banco de ROM em 4000-7FFF (7/8 bits no MBC3, 9 bits no MBC5)
    uint8_t  ram_sel;     // banco de RAM (0-15) ou, no MBC3, registrador do relógio (08-0C)
    bool     has_rtc;
    Rtc      rtc;
} Cartridge;

bool cartridge_init(Cartridge *cart, const uint8_t *rom, size_t size);   // ROM já na memória
bool cartridge_load(Cartridge *cart, const char *path);                  // lê de arquivo (desktop)
uint8_t cartridge_read(const Cartridge *cart, uint16_t address);
void cartridge_free(Cartridge *cart);
void cartridge_write(Cartridge *cart, uint16_t address, uint8_t value);
uint8_t cartridge_ram_read(const Cartridge *cart, uint16_t address);              // A000-BFFF
void cartridge_ram_write(Cartridge *cart, uint16_t address, uint8_t value);
void cartridge_tick(Cartridge *cart);
bool cartridge_save_import(Cartridge *cart, size_t got, uint64_t now);   // 'got' bytes já lidos em cart->save
size_t cartridge_save_export(Cartridge *cart, uint64_t now);             // prepara cart->save e devolve o tamanho                                    // 1x por ciclo-M (relógio do MBC3)

#endif
