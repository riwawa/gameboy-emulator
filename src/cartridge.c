#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/cartridge.h"

#define M_CYCLES_PER_SEC 1048576u   
#define RTC_SAVE_BYTES   48u      

static void parse_header(Cartridge *cart){
    const uint8_t *rom=cart->rom;

    cart->header.type=rom[0x0147];
    cart->header.rom_size_code=rom[0x0148];
    cart->header.ram_size_code=rom[0x0149];

    size_t title_len=16;
    if(rom[0x0143]==0x80 || rom[0x0143]==0xC0){
        title_len=11;
    }

    memcpy(cart->header.title,&rom[0x0134], title_len);
    cart->header.title[title_len]='\0';
}

typedef struct { uint8_t type; MbcKind mbc; bool ram, battery, rtc; } CartType;
static const CartType cart_types[] = {
    {0x00, MBC_NONE, false, false, false},   // ROM
    {0x01, MBC_1,    false, false, false},   // MBC1
    {0x02, MBC_1,    true,  false, false},   // MBC1 + RAM
    {0x03, MBC_1,    true,  true,  false},   // MBC1 + RAM + bateria
    {0x05, MBC_2,    false, false, false},   // MBC2 (a RAM de 512x4 bits é do próprio chip)
    {0x06, MBC_2,    false, true,  false},   // MBC2 + bateria
    {0x08, MBC_NONE, true,  false, false},   // ROM + RAM
    {0x09, MBC_NONE, true,  true,  false},   // ROM + RAM + bateria
    {0x0F, MBC_3,    false, true,  true },   // MBC3 + relógio + bateria
    {0x10, MBC_3,    true,  true,  true },   // MBC3 + relógio + RAM + bateria
    {0x11, MBC_3,    false, false, false},   // MBC3
    {0x12, MBC_3,    true,  false, false},   // MBC3 + RAM
    {0x13, MBC_3,    true,  true,  false},   // MBC3 + RAM + bateria
    {0x19, MBC_5,    false, false, false},   // MBC5
    {0x1A, MBC_5,    true,  false, false},   // MBC5 + RAM
    {0x1B, MBC_5,    true,  true,  false},   // MBC5 + RAM + bateria
    {0x1C, MBC_5,    false, false, false},   // MBC5 + vibração (a vibração é ignorada)
    {0x1D, MBC_5,    true,  false, false},   // MBC5 + vibração + RAM
    {0x1E, MBC_5,    true,  true,  false},   // MBC5 + vibração + RAM + bateria
};

bool cartridge_init(Cartridge *cart, const uint8_t *rom, size_t size) {
    if (!cart || !rom) return false;
    if (size < 0x8000 || (size % 0x4000) != 0) return false;

    *cart = (Cartridge){0};
    cart->rom = rom;
    cart->rom_size = size;
    cart->rom_banks = (uint32_t)(size / 0x4000);
    parse_header(cart);

    const CartType *ct = NULL;
    for (size_t i = 0; i < sizeof cart_types / sizeof cart_types[0]; i++)
        if (cart_types[i].type == cart->header.type) { ct = &cart_types[i]; break; }
    if (!ct) {
        fprintf(stderr, "Tipo de cartucho 0x%02X ainda nao suportado\n", cart->header.type);
        *cart = (Cartridge){0};
        return false;
    }
    cart->mbc = ct->mbc;
    cart->has_battery = ct->battery;
    cart->has_rtc = ct->rtc;
    cart->bank1 = 1;
    cart->rom_bank = 1;

    // Tamanho da RAM (código 0x0149)
    static const uint32_t ram_kb[6] = {0, 2, 8, 32, 128, 64};
    uint8_t rc = cart->header.ram_size_code;
    if (ct->mbc == MBC_2)       cart->ram_size = 512;           // fixa: o cabeçalho diz "sem RAM"
    else if (ct->ram && rc < 6) cart->ram_size = ram_kb[rc] * 1024u;

    size_t total = cart->ram_size + (ct->rtc ? RTC_SAVE_BYTES : 0);
    if (total) {
        cart->save = calloc(1, total);
        if (!cart->save) { *cart = (Cartridge){0}; return false; }
        if (cart->ram_size) cart->ram = cart->save;
        if (ct->battery) cart->save_size = total;
    }
    return true;
}

// Conveniência para o desktop: lê o arquivo para a memória e chama cartridge_init.
bool cartridge_load(Cartridge *cart, const char *path) {
    if (!cart || !path) return false;

    FILE *file = fopen(path, "rb");
    if (!file) return false;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return false; }
    long file_size = ftell(file);
    if (file_size < 0 || fseek(file, 0, SEEK_SET) != 0) { fclose(file); return false; }

    uint8_t *rom = malloc((size_t)file_size);
    if (!rom) { fclose(file); return false; }
    size_t n = fread(rom, 1, (size_t)file_size, file);
    fclose(file);
    if (n != (size_t)file_size || !cartridge_init(cart, rom, (size_t)file_size)) {
        free(rom);
        return false;
    }
    cart->owns_rom = true;          // quem carregou do arquivo é quem libera
    return true;
}

void cartridge_free(Cartridge *cart) {
    if(!cart) return;
    if (cart->owns_rom) free((void *)cart->rom);
    free(cart->save);                // 'ram' aponta para dentro dela
    *cart=(Cartridge){0};
}

// ---------------------------------------------------------------------------
// Leitura da ROM: 0000-7FFF
// ---------------------------------------------------------------------------
uint8_t cartridge_read(const Cartridge *cart, uint16_t address) {
    if (!cart||!cart->rom) return 0xFF;
    if (address >= 0x8000) return 0xFF;

    uint32_t bank;
    switch (cart->mbc) {
    case MBC_1:
        if (address < 0x4000)                              // janela 0000-3FFF
            bank = cart->mode ? ((uint32_t)cart->bank2 << 5) : 0;
        else                                               // janela 4000-7FFF
            bank = ((uint32_t)cart->bank2 << 5) | cart->bank1;
        break;
    case MBC_2: case MBC_3: case MBC_5:
        bank = address < 0x4000 ? 0 : cart->rom_bank;      // 0000-3FFF é sempre o banco 0
        break;
    default:
        return cart->rom[address];                         // sem MBC: ROM direta
    }
    bank &= cart->rom_banks - 1;                           // ROMs menores "dão a volta"
    return cart->rom[(size_t)bank * 0x4000 + (address & 0x3FFF)];
}

static void rtc_second(Rtc *r) {
    // Cada registrador é um contador de N bits. O "vai-um" só sai no valor certo
    // (59, 59, 23); valores inválidos (ex.: segundos = 60..62) só incrementam,
    // e o estouro do contador (63, 63, 31) volta a 0 SEM vai-um.
    bool carry_m = false, carry_h = false, carry_d = false;

    if (r->s == 59) { r->s = 0; carry_m = true; } else r->s = (r->s + 1) & 63;
    if (carry_m) {
        if (r->m == 59) { r->m = 0; carry_h = true; } else r->m = (r->m + 1) & 63;
    }
    if (carry_h) {
        if (r->h == 23) { r->h = 0; carry_d = true; } else r->h = (r->h + 1) & 31;
    }
    if (carry_d) {
        if (r->d == 511) { r->d = 0; r->carry = true; } else r->d++;
    }
}

void cartridge_tick(Cartridge *cart) {
    if (!cart->has_rtc || cart->rtc.halt) return;
    if (++cart->rtc.sub >= M_CYCLES_PER_SEC) {
        cart->rtc.sub = 0;
        rtc_second(&cart->rtc);
    }
}

static void rtc_pack(const Rtc *r, uint8_t out[5]) {
    out[0] = r->s;
    out[1] = r->m;
    out[2] = r->h;
    out[3] = r->d & 0xFF;
    out[4] = (uint8_t)(((r->d >> 8) & 1) | (r->halt ? 0x40 : 0) | (r->carry ? 0x80 : 0));
}

static void rtc_latch(Rtc *r) {
    rtc_pack(r, r->latched);
}

static void rtc_write(Rtc *r, uint8_t reg, uint8_t v) {
    switch (reg) {
    case 0x08: r->s = v & 0x3F; r->sub = 0; break;         // escrever os segundos zera a fração de segundo
    case 0x09: r->m = v & 0x3F; break;
    case 0x0A: r->h = v & 0x1F; break;
    case 0x0B: r->d = (uint16_t)((r->d & 0x100) | v); break;
    case 0x0C: r->d = (uint16_t)((r->d & 0xFF) | ((v & 1) << 8));
               r->halt = v & 0x40;
               r->carry = v & 0x80;
               break;
    }
}

void cartridge_write(Cartridge *cart, uint16_t address, uint8_t value) {
    if(!cart||!cart->rom||cart->mbc==MBC_NONE) return;

    if (cart->mbc == MBC_2) {                              // 0000-3FFF: o bit 8 do ENDEREÇO escolhe o registrador
        if (address < 0x4000) {
            if (address & 0x100) {                         //   bit 8 = 1: banco de ROM (4 bits)
                cart->rom_bank = value & 0x0F;
                if (cart->rom_bank == 0) cart->rom_bank = 1;
            } else {                                       //   bit 8 = 0: liga a RAM
                cart->ram_enable = (value & 0x0F) == 0x0A;
            }
        }
        return;
    }

    if (address < 0x2000) {                                // 0000-1FFF: liga a RAM
        cart->ram_enable = (value & 0x0F) == 0x0A;
        return;
    }

    switch (cart->mbc) {
    case MBC_1:
        switch (address >> 13) {                           // 1..3 = os blocos de 8 KB restantes
        case 1: cart->bank1 = value & 0x1F;
                if (cart->bank1 == 0) cart->bank1 = 1; break;
        case 2: cart->bank2 = value & 0x03; break;
        case 3: cart->mode  = value & 0x01; break;
        }
        break;

    case MBC_3:
        if (address < 0x4000) {                            // 2000-3FFF: banco de ROM (7 bits; 8 no MBC30)
            uint16_t mask = cart->rom_banks > 128 ? 0xFF : 0x7F;
            cart->rom_bank = value & mask;
            if (cart->rom_bank == 0) cart->rom_bank = 1;
        } else if (address < 0x6000) {                     // 4000-5FFF: banco de RAM (0-7) ou relógio (08-0C)
            cart->ram_sel = value & 0x0F;
        } else {                                           // 6000-7FFF: "latch" = escrever 0 e depois 1
            if (cart->has_rtc && cart->rtc.latch_prev == 0 && value == 1) rtc_latch(&cart->rtc);
            cart->rtc.latch_prev = value;
        }
        break;

    case MBC_5:
        if (address < 0x3000)       cart->rom_bank = (cart->rom_bank & 0x100) | value;           // 2000-2FFF: 8 bits baixos
        else if (address < 0x4000)  cart->rom_bank = (cart->rom_bank & 0x0FF) | ((value & 1) << 8); // 3000-3FFF: 9º bit
        else if (address < 0x6000)  cart->ram_sel = value & 0x0F;                                // 4000-5FFF: banco de RAM
        break;

    default: break;
    }
}

static size_t ram_index(const Cartridge *cart, uint16_t address) {
    size_t bank = 0;
    if (cart->mbc == MBC_1)      bank = cart->mode ? cart->bank2 : 0;
    else if (cart->mbc != MBC_NONE) bank = cart->ram_sel;
    return (bank * 0x2000 + (address - 0xA000)) % cart->ram_size;   // RAM de 2 KB espelha
}

uint8_t cartridge_ram_read(const Cartridge *cart, uint16_t address) {
    if (!cart) return 0xFF;
    if (cart->mbc == MBC_NONE) {
        if (!cart->ram) return 0xFF;
        return cart->ram[ram_index(cart, address)];
    }
    if (!cart->ram_enable) return 0xFF;
    if (cart->mbc == MBC_3 && cart->ram_sel >= 0x08) {     // relógio
        if (cart->has_rtc && cart->ram_sel <= 0x0C) return cart->rtc.latched[cart->ram_sel - 8];
        return 0xFF;
    }
    if (!cart->ram) return 0xFF;
    uint8_t v = cart->ram[ram_index(cart, address)];
    return cart->mbc == MBC_2 ? (uint8_t)(v | 0xF0) : v;   // MBC2: só 4 bits; os de cima leem 1
}

void cartridge_ram_write(Cartridge *cart, uint16_t address, uint8_t value) {
    if (!cart) return;
    if (cart->mbc == MBC_NONE) {
        if (cart->ram) {
            uint8_t *p = &cart->ram[ram_index(cart, address)];
            if (*p != value) { *p = value; cart->save_dirty = cart->has_battery; }
        }
        return;
    }
    if (!cart->ram_enable) return;
    if (cart->mbc == MBC_3 && cart->ram_sel >= 0x08) {
        if (cart->has_rtc && cart->ram_sel <= 0x0C) rtc_write(&cart->rtc, cart->ram_sel, value);
        return;
    }
    if (!cart->ram) return;
    if (cart->mbc == MBC_2) value &= 0x0F;                 // só os 4 bits baixos existem
    uint8_t *p = &cart->ram[ram_index(cart, address)];
    if (*p != value) { *p = value; cart->save_dirty = cart->has_battery; }   // só marca se mudou de fato
}

static void put32(uint8_t *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i)); }
static void put64(uint8_t *p, uint64_t v) { for (int i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (8 * i)); }
static uint32_t get32(const uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint64_t get64(const uint8_t *p) { return get32(p) | ((uint64_t)get32(p + 4) << 32); }

static void rtc_advance(Rtc *r, uint64_t secs) {
    if (r->halt || r->s > 59 || r->m > 59 || r->h > 23) return;   // parado ou com valores inválidos: não mexe
    uint64_t total = r->s + 60ull * (r->m + 60ull * (r->h + 24ull * r->d)) + secs;
    uint64_t days  = total / 86400;
    uint32_t rem   = (uint32_t)(total % 86400);
    r->h = (uint8_t)(rem / 3600);
    r->m = (uint8_t)(rem % 3600 / 60);
    r->s = (uint8_t)(rem % 60);
    if (days > 511) r->carry = true;            // o contador de dias estourou: o carry fica ligado
    r->d = (uint16_t)(days & 511);
}

size_t cartridge_save_export(Cartridge *cart, uint64_t now) {
    if (!cart || !cart->save_size) return 0;
    if (cart->has_rtc) {
        uint8_t *p = cart->save + cart->ram_size, cur[5];
        rtc_pack(&cart->rtc, cur);
        for (int i = 0; i < 5; i++) {
            put32(p + 4 * i,      cur[i]);
            put32(p + 20 + 4 * i, cart->rtc.latched[i]);
        }
        put64(p + 40, now);
    }
    cart->save_dirty = false;
    return cart->save_size;
}

bool cartridge_save_import(Cartridge *cart, size_t got, uint64_t now) {
    if (!cart || !cart->save_size || got == 0) return false;
    if (got < cart->ram_size) {
        fprintf(stderr, "Save menor que a RAM do cartucho (%zu < %zu): ignorado\n", got, cart->ram_size);
        memset(cart->save, 0, cart->save_size);
        return false;
    }
    if (cart->has_rtc && got >= cart->ram_size + 44) {       
        const uint8_t *p = cart->save + cart->ram_size;
        Rtc *r = &cart->rtc;
        r->s = get32(p) & 0x3F;       r->m = get32(p + 4) & 0x3F;   r->h = get32(p + 8) & 0x1F;
        uint32_t dh = get32(p + 16);
        r->d = (uint16_t)(((dh & 1) << 8) | (get32(p + 12) & 0xFF));
        r->halt = dh & 0x40;          r->carry = dh & 0x80;
        for (int i = 0; i < 5; i++) r->latched[i] = (uint8_t)get32(p + 20 + 4 * i);
        r->sub = 0;
        uint64_t saved = got >= cart->ram_size + 48 ? get64(p + 40) : get32(p + 40);
        if (now > saved && saved != 0) rtc_advance(r, now - saved);  
    }
    return true;
}
