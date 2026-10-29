# Game Boy Emulator (DMG-01)

A Game Boy (DMG-01) emulator written in C from scratch, built as a learning project in computer architecture and low-level programming. It is the first stage of a larger goal: running the same emulator core on a **standalone, physical handheld** built on an embedded platform, with its own display, buttons and electronics.

> **Status:** the CPU, interrupts, timer, MBC1 cartridges and the graphics pipeline work and pass the reference test ROMs listed below. There is **no audio yet**, and only ROM-only and MBC1 cartridges are supported.

## Features

- Sharp SM83 CPU: full instruction set, interrupts, HALT, EI delay
- Memory bus with VRAM, WRAM (plus echo), OAM, HRAM and I/O registers
- Timer (DIV / TIMA / TMA / TAC) based on the 16-bit system counter, including falling-edge quirks and delayed reload
- PPU: modes 2/3/0/1 timing, LY/LYC and STAT interrupts, background, window, sprites (8x8 and 8x16), palettes, OAM DMA
- Joypad matrix with interrupt
- Cartridges: ROM-only and MBC1 (ROM banking, external RAM)
- SDL2 front end: scaled window, ~59.7 fps pacing, keyboard input, BMP screenshots

## Validation

The emulator is checked against independent reference suites, not just against itself.

| Suite | Result |
|---|---|
| [SingleStepTests/sm83](https://github.com/SingleStepTests/sm83) (per-instruction CPU states) | 498 / 500 opcodes (HALT and STOP differ by convention) |
| Blargg `cpu_instrs` (all 11 + combined 64 KB ROM) | Passed |
| Blargg `instr_timing`, `mem_timing` | Passed |
| Mooneye `acceptance/timer/*` | 13 / 13 |
| Mooneye `emulator-only/mbc1/*` | 12 / 13 (multicart wiring not implemented) |
| Mooneye `div_timing`, `ei_sequence`, `ei_timing`, `intr_timing`, `halt_*` | Passed |
| [dmg-acid2](https://github.com/mattcurrie/dmg-acid2) | Pixel-exact |

Known gaps: Mooneye `ie_push` and `boot_hwio` fail (no boot ROM state is emulated).

## Build and run

Requirements: a C compiler (clang or gcc), `make` and SDL2 (`brew install sdl2` on macOS).

```bash
make
make run ROM=path/to/game.gb
```

Controls:

| Key | Button |
|---|---|
| Arrow keys | D-pad |
| X | A |
| Z | B |
| Enter | Start |
| Backspace | Select |
| Esc | Quit |

Test ROMs are not included in the repository. Put them under `tests/roms/external/`.

## Architecture

The code is split into a portable **core** and a thin **platform** layer, so the same core can later be linked into firmware.

```
include/, src/        core: CPU, bus, PPU, timer, joypad, cartridge
platform/sdl/         desktop front end (window, input, timing)
tests/                test ROMs and runners
```

The core never touches the screen, keyboard or clock. It talks to the outside world through a small hardware abstraction layer (`include/hal.h`):

- `hal_draw_line(y, shades)` receives each finished scanline of 160 pixels (shades 0 to 3)
- `hal_debug_putc(c)` receives serial output, useful for test ROMs

Porting to a microcontroller means implementing those functions for a real display and serial port, and feeding buttons into the joypad module.

### Design notes

- Per-line rendering: the PPU draws a whole scanline when mode 3 ends, which keeps it fast enough for small targets
- The PPU runs 4 dots per M-cycle and a frame is 70224 dots (17556 M-cycles)
- The cartridge ROM is accessed through a single read function, so it can live in flash on an embedded target

## Roadmap

- [ ] `emu_run_frame()` so the platform owns the main loop
- [ ] Battery-backed save files
- [ ] MBC3 and MBC5
- [ ] Audio (APU)
- [ ] Variable mode 3 length and gradual OAM DMA
- [ ] Port to an embedded board (display driver, buttons, ROM in flash)

## References

- [Pan Docs](https://gbdev.io/pandocs/): Game Boy technical documentation
- [GBDev Opcode Tables](https://gbdev.io/gb-opcodes/optables/): CPU instruction reference
- [Game Boy CPU Manual](https://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf)
- [Gekkio's Mooneye Test Suite](https://github.com/Gekkio/mooneye-test-suite) and [Blargg's test ROMs](https://github.com/retrio/gb-test-roms)
- [Low Level Devel](https://www.youtube.com/playlist?list=PLVxiWMqQvhg_yk4qy2cSC3457wZJga_e5): emulator video course
- [Code](https://www.charlespetzold.com/books/code/) by Charles Petzold

