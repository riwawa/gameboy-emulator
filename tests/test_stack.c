
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "cpu_internal.h"

int main(void) {
    // 1. Criar um Bus e uma CPU para o teste
    Bus bus = {0};
    cpu_context cpu = {0};

    cpu.bus = &bus;
    cpu.regs.sp = 0xD000;

    // 2. Empilhar o valor
    stack_push16(&cpu, 0x1234);

    // 3. Verificar PUSH
    assert(cpu.regs.sp == 0xCFFE);
    assert(bus_read(&bus, 0xCFFE) == 0x34);
    assert(bus_read(&bus, 0xCFFF) == 0x12);

    // 4. Recuperar o valor
    uint16_t value = stack_pop16(&cpu);

    // 5. Verificar POP
    assert(value == 0x1234);
    assert(cpu.regs.sp == 0xD000);

    printf("PASS: stack push/pop 16 bits\n");

    return 0;
}
