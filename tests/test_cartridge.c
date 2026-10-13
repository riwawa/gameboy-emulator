#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "cartridge.h"

#define ROM_SIZE 0x8000u
#define CHECK(cond) do { if (!(cond)) { \
    fprintf(stderr, "    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    return false; \
} } while (0)

static unsigned next_id;
static bool make_rom(char *path, size_t capacity, size_t size,
                     uint8_t type, uint8_t rom_code, uint8_t ram_code,
                     const char *title) {
    int len = snprintf(path, capacity, "test_cartridge_%u.gb", ++next_id);
    if (len < 0 || (size_t)len >= capacity) return false;
    uint8_t *data = calloc(size ? size : 1, 1);
    if (!data) return false;
    if (size > 0) data[0] = 0x31;
    if (size > 2) data[2] = 0xAB;
    if (size > 0x0149) {
        data[0x0147] = type;
        data[0x0148] = rom_code;
        data[0x0149] = ram_code;
        if (title) {
            size_t n = strlen(title);
            if (n > 11) n = 11;
            memcpy(data + 0x0134, title, n);
        }
        data[0x7FFF < size ? 0x7FFF : 0] = 0x42;
    }
    FILE *f = fopen(path, "wb");
    if (!f) { free(data); return false; }
    size_t written = fwrite(data, 1, size, f);
    bool closed = fclose(f) == 0;
    free(data);
    if (written != size || !closed) { remove(path); return false; }
    return true;
}

#define FIXTURE(path, size, type, rom, ram, title) \
    CHECK(make_rom(path, sizeof(path), size, type, rom, ram, title))

static bool test_valid_rom(void) {
    char path[80]; FIXTURE(path, ROM_SIZE, 0, 0, 0, "TESTGAME");
    Cartridge cart = {0};
    bool ok = cartridge_load(&cart, path);
    remove(path);
    CHECK(ok);
    CHECK(cart.rom != NULL);
    CHECK(cart.rom_size == ROM_SIZE);
    cartridge_free(&cart);
    return true;
}

static bool test_header(void) {
    char path[80]; FIXTURE(path, ROM_SIZE, 0, 0, 0, "TESTGAME");
    Cartridge cart = {0};
    bool ok = cartridge_load(&cart, path); remove(path);
    CHECK(ok);
    CHECK(strcmp(cart.header.title, "TESTGAME") == 0);
    CHECK(cart.header.type == 0);
    CHECK(cart.header.rom_size_code == 0);
    CHECK(cart.header.ram_size_code == 0);
    cartridge_free(&cart);
    return true;
}

static bool test_reads(void) {
    char path[80]; FIXTURE(path, ROM_SIZE, 0, 0, 0, "TESTGAME");
    Cartridge cart = {0};
    bool ok = cartridge_load(&cart, path); remove(path);
    CHECK(ok);
    CHECK(cartridge_read(&cart, 0x0000) == 0x31);
    CHECK(cartridge_read(&cart, 0x0002) == 0xAB);
    CHECK(cartridge_read(&cart, 0x7FFF) == 0x42);
    CHECK(cartridge_read(&cart, 0x8000) == 0xFF);
    CHECK(cartridge_read(&cart, 0xFFFF) == 0xFF);
    cartridge_free(&cart);
    return true;
}

static bool test_missing_file(void) {
    Cartridge cart = {0};
    CHECK(!cartridge_load(&cart, "arquivo_que_nao_existe_123.gb"));
    CHECK(cart.rom == NULL);
    return true;
}

static bool test_wrong_size(size_t size) {
    char path[80]; FIXTURE(path, size, 0, 0, 0, "TESTGAME");
    Cartridge cart = {0};
    bool ok = cartridge_load(&cart, path); remove(path);
    CHECK(!ok);
    CHECK(cart.rom == NULL);
    return true;
}

static bool test_bad_header(uint8_t type, uint8_t rom, uint8_t ram) {
    char path[80]; FIXTURE(path, ROM_SIZE, type, rom, ram, "TESTGAME");
    Cartridge cart = {0};
    bool ok = cartridge_load(&cart, path); remove(path);
    CHECK(!ok);
    CHECK(cart.rom == NULL);
    return true;
}

static bool test_null_arguments(void) {
    Cartridge cart = {0};
    CHECK(!cartridge_load(NULL, "anything.gb"));
    CHECK(!cartridge_load(&cart, NULL));
    CHECK(cartridge_read(NULL, 0) == 0xFF);
    CHECK(cartridge_read(&cart, 0) == 0xFF);
    cartridge_free(NULL);
    return true;
}

static bool test_free(void) {
    char path[80]; FIXTURE(path, ROM_SIZE, 0, 0, 0, "TESTGAME");
    Cartridge cart = {0};
    bool ok = cartridge_load(&cart, path); remove(path);
    CHECK(ok);
    cartridge_free(&cart);
    CHECK(cart.rom == NULL);
    CHECK(cart.rom_size == 0);
    CHECK(cart.header.title[0] == '\0');
    return true;
}

static bool test_double_free(void) {
    Cartridge cart = {0};
    cartridge_free(&cart);
    cartridge_free(&cart);
    CHECK(cart.rom == NULL);
    return true;
}

/* Detecta apenas consistencia funcional; vazamento requer LeakSanitizer/Valgrind. */
static bool test_reload(void) {
    char first[80], second[80];
    FIXTURE(first, ROM_SIZE, 0, 0, 0, "FIRST");
    FIXTURE(second, ROM_SIZE, 0, 0, 0, "SECOND");
    Cartridge cart = {0};
    bool loaded1 = cartridge_load(&cart, first);
    bool loaded2 = loaded1 && cartridge_load(&cart, second);
    remove(first); remove(second);
    CHECK(loaded1 && loaded2);
    CHECK(strcmp(cart.header.title, "SECOND") == 0);
    cartridge_free(&cart);
    return true;
}

static bool test_failed_reload_preserves_old(void) {
    char path[80]; FIXTURE(path, ROM_SIZE, 0, 0, 0, "FIRST");
    Cartridge cart = {0};
    bool ok = cartridge_load(&cart, path); remove(path);
    CHECK(ok);
    CHECK(!cartridge_load(&cart, "arquivo_que_nao_existe_123.gb"));
    CHECK(strcmp(cart.header.title, "FIRST") == 0);
    CHECK(cartridge_read(&cart, 0) == 0x31);
    cartridge_free(&cart);
    return true;
}

typedef bool (*Test)(void);
static int passed, failed;
static void run(const char *name, Test test) {
    printf("%-42s", name);
    if (test()) { puts("PASS"); ++passed; }
    else { puts("FAIL"); ++failed; }
}
static bool test_small(void) { return test_wrong_size(0x4000); }
static bool test_large(void) { return test_wrong_size(0x10000); }
static bool test_mbc1(void) { return test_bad_header(0x01, 0, 0); }
static bool test_ram(void) { return test_bad_header(0, 0, 0x02); }
static bool test_rom_code(void) { return test_bad_header(0, 0x01, 0); }

int main(void) {
    run("01 Carregar ROM valida", test_valid_rom);
    run("02 Verificar cabecalho", test_header);
    run("03 Ler enderecos e limites", test_reads);
    run("04 Arquivo inexistente", test_missing_file);
    run("05 ROM menor que 32 KiB", test_small);
    run("06 ROM maior que 32 KiB", test_large);
    run("07 Rejeitar MBC1", test_mbc1);
    run("08 Rejeitar RAM externa", test_ram);
    run("09 Rejeitar codigo ROM nao suportado", test_rom_code);
    run("10 Argumentos nulos", test_null_arguments);
    run("11 Liberar e zerar cartucho", test_free);
    run("12 Liberar duas vezes", test_double_free);
    run("13 Recarregar ROM (checar vazamento)", test_reload);
    run("14 Preservar ROM se recarga falhar", test_failed_reload_preserves_old);
    printf("\nResultado: %d passaram, %d falharam\n", passed, failed);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
