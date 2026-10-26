
import subprocess
import re
from pathlib import Path

# Pasta do projeto e executável
ROOT = Path(__file__).resolve().parent
EXECUTABLE = ROOT / "gameboy"
TEST_DIR = ROOT / "tests"

# Valores em hexadecimal, exceto ciclos.
# Nome do arquivo: (instrução, A esperado, F esperado, ciclos)
TESTS = {
    "01_add_basic.gb":     ("ADD", 0x10, 0x20, 12),
    "02_adc_carry.gb":     ("ADC", 0x10, 0x20, 12),
    "03_sub_halfborrow.gb":("SUB", 0x0F, 0x60, 12),
    "04_sbc_carry.gb":     ("SBC", 0x0F, 0x60, 12),
    "05_and_basic.gb":     ("AND", 0x03, 0x20, 12),
    "06_xor_zero.gb":      ("XOR", 0x00, 0x80, 12),
    "07_or_basic.gb":      ("OR",  0xAF, 0x00, 12),
    "08_cp_equal.gb":      ("CP",  0x05, 0xC0, 12),
    "09_add_overflow.gb":  ("ADD", 0x00, 0xB0, 12),
    "10_sbc_borrow.gb":    ("SBC", 0x00, 0xF0, 12),
    "11_cp_borrow.gb":     ("CP",  0x02, 0x70, 12),
    "12_and_immediate.gb": ("AND", 0x00, 0xA0, 12),
    "13_xor_memory.gb":   ("XOR", 0xFF, 0x00, 16),
    "14_or_immediate.gb":  ("OR",  0xAF, 0x00, 12),
    "15_cp_immediate.gb":  ("CP",  0x05, 0xC0, 12),
}

STATE_PATTERN = re.compile(
    r"AFTER \| PC:([0-9A-Fa-f]+) "
    r"A:([0-9A-Fa-f]+) "
    r"B:([0-9A-Fa-f]+) "
    r"C:([0-9A-Fa-f]+) "
    r"F:([0-9A-Fa-f]+)"
)

def compile_emulator():
    print("Compilando emulador...")

    sources = sorted(str(p) for p in (ROOT / "src").glob("*.c"))

    command = [
        "clang", "-std=c11",
        "-Wall", "-Wextra",
        "-I" + str(ROOT / "include"),
        *sources,
        "-o", str(EXECUTABLE)
    ]

    result = subprocess.run(
        command,
        capture_output=True,
        text=True
    )

    if result.returncode != 0:
        print("Erro de compilação:")
        print(result.stderr)
        return False

    print("Compilação concluída.\n")
    return True


def run_test(filename, expected):
    operation, expected_a, expected_f, expected_cycles = expected
    rom_path = TEST_DIR / filename

    if not rom_path.is_file():
        return False, "ROM não encontrada"

    try:
        process = subprocess.run(
            [str(EXECUTABLE), str(rom_path)],
            capture_output=True,
            text=True,
            timeout=5
        )
    except subprocess.TimeoutExpired:
        return False, "Timeout"

    if process.returncode != 0:
        return False, f"Processo terminou com código {process.returncode}"

    output = process.stdout

    # Verificar se a operação esperada foi executada.
    executed = re.search(
        rf"^[0-9A-Fa-f]{{4}}:\s+{operation}\s+\(",
        output,
        re.MULTILINE
    )

    if not executed:
        return False, f"Instrução {operation} não encontrada"

    # Recuperar o último estado.
    # Nas ROMs atuais, após a ALU só existem NOPs,
    # que não modificam registradores nem flags.
    states = STATE_PATTERN.findall(output)

    if not states:
        return False, "Nenhum estado AFTER encontrado"

    pc, a, b, c, f = (
        int(value, 16) for value in states[-1]
    )

    cycles_match = re.search(r"M-cycles:\s*(\d+)", output)
    instructions_match = re.search(r"Instructions:\s*(\d+)", output)

    if not cycles_match or not instructions_match:
        return False, "Contadores não encontrados"

    cycles = int(cycles_match.group(1))
    instructions = int(instructions_match.group(1))

    # Os 15 testes atuais executam 10 instruções.
    expected_pc = 0x010E if filename == "13_xor_memory.gb" else 0x010C

    errors = []

    if a != expected_a:
        errors.append(f"A={a:02X}, esperado {expected_a:02X}")

    if f != expected_f:
        errors.append(f"F={f:02X}, esperado {expected_f:02X}")

    if pc != expected_pc:
        errors.append(f"PC={pc:04X}, esperado {expected_pc:04X}")

    if cycles != expected_cycles:
        errors.append(f"ciclos={cycles}, esperado {expected_cycles}")

    if instructions != 10:
        errors.append(f"instruções={instructions}, esperado 10")

    if errors:
        return False, "; ".join(errors)

    return True, f"A={a:02X} F={f:02X} ciclos={cycles}"


def main():
    if not compile_emulator():
        raise SystemExit(1)

    passed = 0

    for filename, expected in TESTS.items():
        ok, message = run_test(filename, expected)

        if ok:
            passed += 1
            print(f"PASS  {filename}  {message}")
        else:
            print(f"FAIL  {filename}  {message}")

    total = len(TESTS)

    print("\n" + "=" * 40)
    print(f"RESULTADO: {passed}/{total} testes passaram")
    print("=" * 40)

    if passed != total:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
