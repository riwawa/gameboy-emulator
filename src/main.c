#include <stdio.h>
#include "../include/cartridge.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        puts("Uso: ./gameboy jogo.gb");
        return 1;
    }

    Cartridge cart = {0};

    if (!cartridge_load(&cart, argv[1])) {
        puts("Erro ao carregar cartucho.");
        return 1;
    }

    printf("Titulo: %s\n", cart.header.title);
    printf("Tipo: 0x%02X\n", cart.header.type);
    printf("ROM: %zu bytes\n", cart.rom_size);
    printf("Byte em 0x0100: 0x%02X\n",
           cartridge_read(&cart, 0x0100));

    cartridge_free(&cart);
    return 0;
}
